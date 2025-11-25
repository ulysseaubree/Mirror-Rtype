#include <iostream>
#include <thread>
#include <chrono>

#include "ecs.hpp"
#include "components.hpp"
#include "systems.hpp"
#include "componentsN.hpp"
#include "systemsN.hpp"

int main(int argc, char* argv[]) {
    std::cout << "=== R-Type Server ===" << std::endl;
    std::cout << "Starting server..." << std::endl;
    
    // Initialiser l'ECS
    ecs::gCoordinator.Init();
    
    std::cout << "ECS initialized successfully!" << std::endl;
    
    // TODO: Initialiser les composants
    // TODO: Initialiser les systèmes
    // TODO: Démarrer la boucle de jeu
    
    std::cout << "Server ready. Press Ctrl+C to stop." << std::endl;
    
    // Boucle simple pour le moment
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "." << std::flush;
    }
    
    return 0;
}