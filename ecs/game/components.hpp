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

// Composants pour le gameplay serveur

struct PlayerInput {
    int direction{5};  // 1-9 (numpad layout), 5 = neutre
    bool firePressed{false};
};

struct Team {
    int teamID{0};  // 0 = joueurs, 1 = ennemis, 2 = neutre
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
    Entity target{MAX_ENTITIES};  // Cible actuelle (MAX_ENTITIES = pas de cible)
    
    float detectionRange{300.f};   // Distance de détection
    float attackRange{50.f};        // Distance d'attaque
    float fleeHealthThreshold{0.3f}; // Fuit si HP < 30%
    
    float decisionTimer{0.f};       // Timer pour les décisions
    float decisionCooldown{1.f};    // Intervalle entre décisions
};

struct Spawner {
    enum class SpawnType {
        Projectile,
        Enemy,
        Powerup
    };
    
    SpawnType typeToSpawn{SpawnType::Projectile};
    
    float spawnTimer{0.f};
    float spawnCooldown{2.f};
    
    int spawnCount{0};
    int maxSpawns{-1};  // -1 = infini
    
    // Offset de spawn par rapport au spawner
    float spawnOffsetX{0.f};
    float spawnOffsetY{0.f};
    
    // Vélocité initiale des entités spawnées
    float spawnVelocityX{0.f};
    float spawnVelocityY{0.f};
};

struct PlayerTag {
    int clientId{0};
};

}