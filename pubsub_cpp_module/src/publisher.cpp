// publisher.cpp
#include "zenoh_unity_wrapper.h"
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>

int main() {
    ZU_NodeHandle node = ZU_CreateNode("publisher_node");

    const char* key = "demo/example/bytes";
    const std::vector<uint8_t> payload = {
        'h','e','l','l','o','w','o','r','l','d','\0'
    };

    while (true) {
        ZU_Publish(node, key, payload.data(), static_cast<int32_t>(payload.size()));

        std::cout << "Published message: ";
        for (auto byte : payload) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ZU_DestroyNode(node);
    return 0;
}