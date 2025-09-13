#include "Node.h"
#include <stdexcept>

using namespace zenoh;

namespace zexample {

static Session open_default_session() {
    Config cfg = Config::create_default();
    return Session::open(std::move(cfg));
}

Node::Node() : _session(open_default_session()) {}
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

bool Node::get_publisher(const std::string& key, Publisher*& out) {
    std::lock_guard<std::mutex> lock(_mx);
    auto it = _publishers.find(key);
    if (it == _publishers.end()) { out = nullptr; return false; }
    out = it->second.get();
    return true;
}

void Node::publish(const std::string& key, const std::string& data) {
    Publisher* pub = nullptr;
    if (!get_publisher(key, pub)) {
        create_publisher(key);
        get_publisher(key, pub);
    }
    pub->put(Bytes(data));
}

bool Node::has_subscriber(const std::string& key) const {
    std::lock_guard<std::mutex> lock(_mx);
    return _subscribers.find(key) != _subscribers.end();
}

void Node::create_subscriber(const std::string& key, MessageCallback cb) {
    std::lock_guard<std::mutex> lock(_mx);
    if (_subscribers.count(key)) return;

    auto sub = std::make_shared<Subscriber<void>>(
        _session.declare_subscriber(
            make_keyexpr(key),
            [cb, key](const Sample& s) {
                cb(key, s.get_payload().as_string());
            },
            closures::none
        )
    );
    _subscribers.emplace(key, std::move(sub));
}

bool Node::get_subscriber(const std::string& key, Subscriber<void>*& out) {
    std::lock_guard<std::mutex> lock(_mx);
    auto it = _subscribers.find(key);
    if (it == _subscribers.end()) { out = nullptr; return false; }
    out = it->second.get();
    return true;
}

void Node::subscribe(const std::string& key, MessageCallback cb) {
    if (has_subscriber(key)) return;
    create_subscriber(key, std::move(cb));
}

} // namespace zexample
