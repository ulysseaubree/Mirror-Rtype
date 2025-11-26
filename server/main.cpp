#include <iostream>
#include <thread>
#include <chrono>
#include <random>

#include "../ecs/utils/utils.hpp"

using namespace ecs;

std::random_device rd;
std::mt19937 gen(rd());

// Spawn d'ennemis
void SpawnEnemyWave() {
    std::uniform_real_distribution<float> disY(50.0f, 550.0f);
    std::uniform_int_distribution<int> disType(0, 2);
    
    float y = disY(gen);
    int typeInt = disType(gen);
    
    Utils::EnemyType type;
    switch (typeInt) {
        case 0: type = Utils::EnemyType::Basic; break;
        case 1: type = Utils::EnemyType::Zigzag; break;
        case 2: type = Utils::EnemyType::Shooter; break;
        default: type = Utils::EnemyType::Basic; break;
    }
    
    Entity enemy = Utils::CreateEnemy(800.0f, y, type);
    std::cout << "Spawned enemy at y=" << y << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== R-Type Server ===" << std::endl;
    std::cout << "Starting server..." << std::endl;
    
    // Initialiser l'ECS avec Utils
    Utils::InitECS();
    Utils::RegisterSystems(nullptr); // nullptr = mode serveur (pas de rendu)
    
    std::cout << "ECS initialized!" << std::endl;
    
    // TODO: Initialiser le réseau
    // NetworkManager::Instance().InitializeServer(12345);
    
    std::cout << "Server ready on port 12345" << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;
    
    // Timer pour spawn d'ennemis
    auto lastSpawn = std::chrono::steady_clock::now();
    const float spawnInterval = 2.0f; // Spawn toutes les 2 secondes
    
    // Boucle du serveur (60 ticks/sec)
    auto lastTick = std::chrono::steady_clock::now();
    const float tickRate = 1.0f / 60.0f;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTick).count();
        
        if (dt >= tickRate) {
            lastTick = now;
            
            // TODO: Update systems
            // movementSystem->Update(dt);
            // collisionSystem->Update();
            // healthSystem->Update(dt);
            
            // Spawn d'ennemis
            float timeSinceSpawn = std::chrono::duration<float>(now - lastSpawn).count();
            if (timeSinceSpawn >= spawnInterval) {
                SpawnEnemyWave();
                lastSpawn = now;
            }
            
            // TODO: Network updates
            // replicationSystem->Update(dt);
        }
        
        // Sleep pour ne pas surcharger le CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    return 0;
}