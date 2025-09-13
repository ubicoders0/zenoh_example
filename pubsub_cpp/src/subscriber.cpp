#include "zenoh.hxx"
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>


using namespace zenoh;

int main(int argc, char** argv) {
    Config config = Config::create_default();
    auto session = Session::open(std::move(config));

    std::vector<uint8_t> received_payload;

    auto subscriber = session.declare_subscriber(
        KeyExpr("demo/example/bytes"),
        [&received_payload](const Sample& sample) {
            received_payload = sample.get_payload().as_vector();
        },
        closures::none
    );

    std::cout << "Subscriber ready." << std::endl;

    while(true){
        if (!received_payload.empty()) {
            std::cout << "Received message: "
                      << reinterpret_cast<const char*>(received_payload.data())
                      << std::endl;
            received_payload.clear();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    

    return 0;
}
