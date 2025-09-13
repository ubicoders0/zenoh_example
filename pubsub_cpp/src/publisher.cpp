#include "zenoh.hxx"
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>

using namespace zenoh;

int main() {
    auto session = Session::open(Config::create_default());

    auto pub = session.declare_publisher(KeyExpr("demo/example/bytes"));

    // Prepare "helloworld" as raw bytes (includes a '\0' to print nicely)
    const std::vector<uint8_t> payload = {
        'h','e','l','l','o','w','o','r','l','d','\0'
    };

    while (true) {
        pub.put(Bytes(payload));  // now matches Bytes(const std::vector<uint8_t>&)
        std::cout << "Published message: "
                  << reinterpret_cast<const char*>(payload.data()) << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
