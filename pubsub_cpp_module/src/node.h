#pragma once
#include "zenoh.hxx"

#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <memory>
#include <vector>

namespace ubicoders_zenoh {

class Node {
public:
    // Callback now delivers raw bytes
    using MessageCallback = std::function<void(const std::string& key,
                                               const std::vector<uint8_t>& payload)>;

    explicit Node(const std::string& name);
    Node();
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    ~Node();

    const std::string& name() const { return _name; }  // NEW

    // ---- Query server (queryable) management ----
    // Synchronous handler: return the reply bytes for this query
    using QueryHandler = std::function<std::vector<uint8_t>(
        const std::string& key,
        const std::string& parameters,
        const std::vector<uint8_t>& payload)>;

    // Declare a queryable "server" at `key`. Each incoming query calls `handler`,
    // and we reply with its returned bytes.
    void create_server(const std::string& key, QueryHandler handler);

    // Undeclare a server for `key`.
    void remove_server(const std::string& key);

    // ---- Publisher management ----
    bool has_publisher(const std::string& key) const;
    void create_publisher(const std::string& key);
    void publish(const std::string& key, const std::vector<uint8_t>& data);
    void remove_publisher(const std::string& key);   // NEW

    // ---- Subscriber management ----
    bool has_subscriber(const std::string& key) const;
    void create_subscriber(const std::string& key, MessageCallback cb);
    void remove_subscriber(const std::string& key);  // NEW

    void shutdown();

private:
    zenoh::Session _session;
    std::string _name;  // NEW

    std::unordered_map<std::string, std::shared_ptr<zenoh::Publisher>>        _publishers;
    std::unordered_map<std::string, std::shared_ptr<zenoh::Subscriber<void>>> _subscribers;
    std::unordered_map<std::string, std::shared_ptr<zenoh::Queryable<void>>>  _servers;

    mutable std::mutex _mx;

    static zenoh::KeyExpr make_keyexpr(const std::string& key);
};

} // namespace ubicoders_zenoh
