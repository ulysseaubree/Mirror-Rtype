#pragma once

#include <SFML/Graphics.hpp>
#include "types.hpp"

namespace ecs {

struct Transform {
    float x{};
    float y{};
    float rotation{};
};

struct Velocity {
    float vx{};
    float vy{};
};

struct Collider {
    enum class Shape {
        Circle,
        Box
    };
    
    Shape shape{Shape::Circle};
    float radius{10.f};
    float width{20.f};
    float height{20.f};
    bool isTrigger{false};
};

struct Health {
    int current{100};
    int max{100};
    bool invincible{false};
    float invincibilityTimer{0.f};
};

struct Lifetime {
    float timeLeft{5.f};
};

struct Score {
    int points{0};
};

struct Boundary {
    float minX{0.f};
    float maxX{800.f};
    float minY{0.f};
    float maxY{600.f};
    bool wrap{false}; 
    bool destroy{true};
};

// PlayerInput: Stocke la direction d'input (1-9, numpad style)
// 7 8 9
// 4 5 6  (5 = pas de mouvement)
// 1 2 3
struct PlayerInput {
    int direction{5};
    bool firePressed{false};
};

struct Team {
    int teamID{0};
};

struct Damager {
    int damage{10};
};

struct AIController {
    enum class State {
        Idle,
        Patrolling,
        Chasing,
        Fleeing,
        Attacking
    };
    
    State currentState{State::Idle};
    Entity target{MAX_ENTITIES};
    float decisionTimer{0.f};
    float decisionCooldown{1.0f}; // Temps entre les décisions
    
    float detectionRange{200.f};
    float attackRange{50.f};
    float fleeHealthThreshold{0.3f}; // Fuit si santé < 30%
};

struct Spawner {
    enum class SpawnType {
        Projectile,
        Enemy,
        Powerup
    };
    
    SpawnType typeToSpawn{SpawnType::Projectile};
    float spawnCooldown{1.0f};
    float spawnTimer{0.f};
    int maxSpawns{-1};
    int spawnCount{0};
    
    float spawnOffsetX{0.f};
    float spawnOffsetY{0.f};
    float spawnVelocityX{0.f};
    float spawnVelocityY{100.f};
};

}