// zenoh_unity.cpp
#define ZENOH_UNITY_BUILD
#include "zenoh_unity_wrapper.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include "node.h"
#include <condition_variable>
#include <chrono>

using ubicoders_zenoh::Node;

namespace {
std::atomic<uint64_t> g_next_id{1};

struct NodeEntry {
    std::unique_ptr<Node> node;
};

std::mutex g_mx;
std::unordered_map<uint64_t, NodeEntry> g_nodes;

Node* get_node(ZU_NodeHandle h) {
    if (h == 0) return nullptr;
    std::lock_guard<std::mutex> lk(g_mx);
    auto it = g_nodes.find(h);
    return (it == g_nodes.end()) ? nullptr : it->second.node.get();
}

// Per-server registration (per node + key)
struct ServerCfg {
    ZU_QueryCallback cb = nullptr;
    void* user = nullptr;
    int timeout_ms = 3000;
};

// One latch per request (Unity will complete/fail it)
struct Pending {
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    bool ok = false;
    std::vector<uint8_t> reply;
    std::string err;
};

// node -> (key -> cfg)
static std::unordered_map<uint64_t, std::unordered_map<std::string, ServerCfg>> g_servers;
// node -> (request_id -> pending)
static std::unordered_map<uint64_t, std::unordered_map<uint64_t, std::shared_ptr<Pending>>> g_pending;



} // namespace

extern "C" {

ZU_NodeHandle ZU_CreateNode(const char* name) {
    try {
        auto nid = g_next_id.fetch_add(1, std::memory_order_relaxed);
        auto n = std::make_unique<Node>(name ? std::string(name) : std::string());
        std::lock_guard<std::mutex> lk(g_mx);
        g_nodes.emplace(nid, NodeEntry{std::move(n)});
        return nid;
    } catch (...) { return 0; }
}

void ZU_DestroyNode(ZU_NodeHandle node) {
    std::unique_ptr<Node> owned;
    {
        std::lock_guard<std::mutex> lk(g_mx);
        auto it = g_nodes.find(node);
        if (it != g_nodes.end()) {
            owned = std::move(it->second.node);
            g_nodes.erase(it);
        }
    }
    // dtor runs outside lock
}

void ZU_ShutdownNode(ZU_NodeHandle node) {
    if (auto* n = get_node(node)) {
        n->shutdown();
    }
}

// ---- Publisher API ----
int32_t ZU_HasPublisher(ZU_NodeHandle node, const char* key) {
    if (auto* n = get_node(node)) return n->has_publisher(key ? key : "");
    return 0;
}

int32_t ZU_CreatePublisher(ZU_NodeHandle node, const char* key) {
    if (auto* n = get_node(node)) {
        try { n->create_publisher(key ? key : ""); return 1; }
        catch (...) { }
    }
    return 0;
}

int32_t ZU_RemovePublisher(ZU_NodeHandle node, const char* key) {
    if (auto* n = get_node(node)) {
        try { n->remove_publisher(key ? key : ""); return 1; }
        catch (...) { }
    }
    return 0;
}

int32_t ZU_Publish(ZU_NodeHandle node, const char* key,
                   const uint8_t* data, int32_t len) {
    if (!data || len < 0) return 0;
    if (auto* n = get_node(node)) {
        try {
            std::vector<uint8_t> buf(data, data + len);
            n->publish(key ? key : "", buf);
            return 1;
        } catch (...) { }
    }
    return 0;
}

// ---- Subscriber API ----
int32_t ZU_HasSubscriber(ZU_NodeHandle node, const char* key) {
    if (auto* n = get_node(node)) return n->has_subscriber(key ? key : "");
    return 0;
}

int32_t ZU_CreateSubscriber(ZU_NodeHandle node, const char* key,
                            ZU_MessageCallback cb, void* user_data) {
    if (!cb) return 0;
    if (auto* n = get_node(node)) {
        try {
            n->create_subscriber(key ? key : "",
                [cb, user_data](const std::string& k,
                                const std::vector<uint8_t>& payload) {
                    // Call out to C callback (background thread).
                    cb(k.c_str(),
                       payload.empty() ? nullptr : payload.data(),
                       static_cast<int32_t>(payload.size()),
                       user_data);
                });
            return 1;
        } catch (...) { }
    }
    return 0;
}

int32_t ZU_RemoveSubscriber(ZU_NodeHandle node, const char* key) {
    if (auto* n = get_node(node)) {
        try { n->remove_subscriber(key ? key : ""); return 1; }
        catch (...) { }
    }
    return 0;
}

// ---- Query Server (Queryable) ----------------------------------------------
int32_t ZU_CreateServer(ZU_NodeHandle node,
                        const char* key_expr,
                        ZU_QueryCallback cb,
                        void* user_data,
                        int32_t timeout_ms)
{
    auto* n = get_node(node);
    if (!n || !key_expr || !cb) return 0;

    // Save callback config
    {
        std::lock_guard<std::mutex> lk(g_mx);
        g_servers[node][key_expr] = ServerCfg{cb, user_data, timeout_ms > 0 ? timeout_ms : 3000};
    }

    try {
        // Bridge Node::create_server to Unity async complete/fail
        n->create_server(key_expr,
            [node, key = std::string(key_expr)](const std::string& key_in,
                                                const std::string& params,
                                                const std::vector<uint8_t>& payload) -> std::vector<uint8_t>
            {
                // Load cfg
                ServerCfg cfg{};
                {
                    std::lock_guard<std::mutex> lk(g_mx);
                    auto nit = g_servers.find(node);
                    if (nit == g_servers.end()) throw std::runtime_error("server-not-found");
                    auto sit = nit->second.find(key);
                    if (sit == nit->second.end()) throw std::runtime_error("server-not-found");
                    cfg = sit->second;
                }

                // Create pending entry
                uint64_t id = g_next_id.fetch_add(1, std::memory_order_relaxed);
                auto pend = std::make_shared<Pending>();
                {
                    std::lock_guard<std::mutex> lk(g_mx);
                    g_pending[node][id] = pend;
                }

                // Fire Unity callback (background thread!)
                const uint8_t* data = payload.empty() ? nullptr : payload.data();
                cfg.cb(id, key.c_str(), data, static_cast<int32_t>(payload.size()),
                       params.c_str(), cfg.user);

                // Wait for Unity to complete or timeout
                std::unique_lock<std::mutex> ul(pend->mtx);
                if (!pend->cv.wait_for(
                        ul,
                        std::chrono::milliseconds(cfg.timeout_ms),
                        [&]{ return pend->done; }))
                {
                    // Timeout → clean up and error
                    {
                        std::lock_guard<std::mutex> lk(g_mx);
                        auto nit = g_pending.find(node);
                        if (nit != g_pending.end()) nit->second.erase(id);
                    }
                    throw std::runtime_error("timeout");
                }

                // Completed → remove
                {
                    std::lock_guard<std::mutex> lk(g_mx);
                    auto nit = g_pending.find(node);
                    if (nit != g_pending.end()) nit->second.erase(id);
                }

                if (!pend->ok) throw std::runtime_error(pend->err.empty() ? "error" : pend->err);
                return pend->reply; // Node::create_server will reply with these bytes
            });

        return 1;
    } catch (...) {
        std::lock_guard<std::mutex> lk(g_mx);
        auto it = g_servers.find(node);
        if (it != g_servers.end()) it->second.erase(key_expr);
        return 0;
    }
}

int32_t ZU_RemoveServer(ZU_NodeHandle node, const char* key_expr)
{
    auto* n = get_node(node);
    if (!n || !key_expr) return 0;
    try {
        n->remove_server(key_expr);
        std::lock_guard<std::mutex> lk(g_mx);
        auto it = g_servers.find(node);
        if (it != g_servers.end()) it->second.erase(key_expr);
        return 1;
    } catch (...) { return 0; }
}

int32_t ZU_CompleteRequest(ZU_NodeHandle node, uint64_t request_id,
                           const uint8_t* bytes, int32_t len)
{
    std::shared_ptr<Pending> p;
    {
        std::lock_guard<std::mutex> lk(g_mx);
        auto nit = g_pending.find(node);
        if (nit == g_pending.end()) return 0;
        auto pit = nit->second.find(request_id);
        if (pit == nit->second.end()) return 0;
        p = pit->second;
    }
    {
        std::lock_guard<std::mutex> lk(p->mtx);
        p->ok = true;
        p->reply.assign(bytes ? bytes : nullptr,
                        (bytes && len > 0) ? (bytes + len) : bytes);
        p->done = true;
    }
    p->cv.notify_all();
    return 1;
}

int32_t ZU_FailRequest(ZU_NodeHandle node, uint64_t request_id, const char* message)
{
    std::shared_ptr<Pending> p;
    {
        std::lock_guard<std::mutex> lk(g_mx);
        auto nit = g_pending.find(node);
        if (nit == g_pending.end()) return 0;
        auto pit = nit->second.find(request_id);
        if (pit == nit->second.end()) return 0;
        p = pit->second;
    }
    {
        std::lock_guard<std::mutex> lk(p->mtx);
        p->ok = false;
        p->err = message ? message : "error";
        p->done = true;
    }
    p->cv.notify_all();
    return 1;
}

} // extern "C"
