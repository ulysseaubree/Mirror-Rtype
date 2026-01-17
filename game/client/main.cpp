#include <iostream>
#include "coordinator.hpp"  // Test de l'ECS
#include "udpClient.hpp"    // Test du Réseau

int main() {
    try {
        std::cout << "=== R-Type Client Starting ===" << std::endl;

        // Test d'instanciation basique pour vérifier le link
        ecs::Coordinator coordinator;
        coordinator.Init();
        std::cout << "ECS Coordinator Initialized." << std::endl;

        // UdpClient client("127.0.0.1", 4242); 
        // std::cout << "Network Client Object Created." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 84;
    }

    std::cout << "Client runs successfully." << std::endl;
    return 0;
}
