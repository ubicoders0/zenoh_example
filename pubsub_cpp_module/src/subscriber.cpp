// subscriber.cpp
#include "zenoh_unity_wrapper.h"
#include <string>
#include <vector>      
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>

void message_callback(const char* key, const uint8_t* data, int32_t len, void* user_data) {
    std::cout << "Received message: ";
    for (int32_t i = 0; i < len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

int main() {
    ZU_NodeHandle node = ZU_CreateNode("subscriber_node");
    const char* key = "demo/example/bytes";

    ZU_CreateSubscriber(node, key, message_callback, nullptr);

    std::cout << "Subscriber ready." << std::endl;

    // Keep the process alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ZU_DestroyNode(node);
    return 0;
}