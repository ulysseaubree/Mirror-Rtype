#include <iostream>
#include "coordinator.hpp"
#include "udpServer.hpp"

int main() {
    try {
        std::cout << "=== R-Type Server Starting ===" << std::endl;

        ecs::Coordinator coordinator;
        coordinator.Init();
        std::cout << "ECS Coordinator Initialized." << std::endl;

        // UdpServer server(4242);
        // std::cout << "Network Server Object Created." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 84;
    }

    std::cout << "Server runs successfully." << std::endl;
    return 0;
}
