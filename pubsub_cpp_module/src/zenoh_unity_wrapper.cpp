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

} // extern "C"
