#include "Node.h"
#include <stdexcept>

using namespace zenoh;

namespace ubicoders_zenoh {

static Session open_default_session() {
    Config cfg = Config::create_default();
    return Session::open(std::move(cfg));
}

Node::Node() 
    : _name(""), _session(open_default_session()) {}

Node::Node(const std::string& name) 
    : _name(name), _session(open_default_session()) {}
    
Node::~Node() { shutdown(); }

void Node::shutdown() {
    std::lock_guard<std::mutex> lock(_mx);
    _subscribers.clear(); // undeclare before session dies
    _publishers.clear();
}

KeyExpr Node::make_keyexpr(const std::string& key) {
    return KeyExpr(key.c_str());
}

bool Node::has_publisher(const std::string& key) const {
    std::lock_guard<std::mutex> lock(_mx);
    return _publishers.find(key) != _publishers.end();
}

void Node::create_publisher(const std::string& key) {
    std::lock_guard<std::mutex> lock(_mx);
    if (_publishers.count(key)) return;
    auto pub = std::make_shared<Publisher>(_session.declare_publisher(make_keyexpr(key)));
    _publishers.emplace(key, std::move(pub));
}

void Node::publish(const std::string& key, const std::vector<uint8_t>& data) {
    Publisher* pub = nullptr;
    if (!has_publisher(key)) {
        create_publisher(key);        
    }
    auto it = _publishers.find(key);
    it->second->put(zenoh::Bytes(data));
}

void Node::remove_publisher(const std::string& key) {
    std::lock_guard<std::mutex> lock(_mx);
    auto it = _publishers.find(key);
    if (it != _publishers.end()) {
        it->second.reset();  // ensure undeclare before erase
        _publishers.erase(it);
    }
}

bool Node::has_subscriber(const std::string& key) const {
    std::lock_guard<std::mutex> lock(_mx);
    return _subscribers.find(key) != _subscribers.end();
}

void Node::create_subscriber(const std::string& key, MessageCallback cb) {
    std::lock_guard<std::mutex> lock(_mx);
    if (_subscribers.count(key)) return;

    // We extract as string (which is binary-safe in C++) and hand off a byte vector.
    auto sub = std::make_shared<Subscriber<void>>(
        _session.declare_subscriber(
            make_keyexpr(key),
            [cb, key](const Sample& s) {
                const std::string bin = s.get_payload().as_string();   // preserves embedded '\0'
                std::vector<uint8_t> bytes(bin.begin(), bin.end());
                cb(key, bytes);
            },
            closures::none
        )
    );
    _subscribers.emplace(key, std::move(sub));
}


void Node::remove_subscriber(const std::string& key) {
    std::lock_guard<std::mutex> lock(_mx);
    auto it = _subscribers.find(key);
    if (it != _subscribers.end()) {
        it->second.reset();
        _subscribers.erase(it);
    }
}

} // namespace ubicoders_zenoh
