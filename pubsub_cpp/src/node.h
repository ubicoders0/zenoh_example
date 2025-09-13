#pragma once
#include "zenoh.hxx"

#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <memory>

namespace zexample {

class Node {
public:
    using MessageCallback = std::function<void(const std::string& key,
                                               const std::string& payload)>;

    Node();
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;
    ~Node();

    // ---- Publisher management ----
    bool has_publisher(const std::string& key) const;
    void create_publisher(const std::string& key);
    bool get_publisher(const std::string& key, zenoh::Publisher*& out);

    void publish(const std::string& key, const std::string& data);

    // ---- Subscriber management ----
    bool has_subscriber(const std::string& key) const;
    void create_subscriber(const std::string& key, MessageCallback cb);
    bool get_subscriber(const std::string& key, zenoh::Subscriber<void>*& out);

    void subscribe(const std::string& key, MessageCallback cb);
    void shutdown();

private:
    zenoh::Session _session;

    // NOTE: callback-based subscribers are Subscriber<void>
    std::unordered_map<std::string, std::shared_ptr<zenoh::Publisher>>       _publishers;
    std::unordered_map<std::string, std::shared_ptr<zenoh::Subscriber<void>>> _subscribers;

    mutable std::mutex _mx;

    static zenoh::KeyExpr make_keyexpr(const std::string& key);
};

} // namespace zexample
