#pragma once

#include <SFML/Graphics.hpp>
#include "types.hpp"
#include <string>
#include <unordered_map>
#include <variant>

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

// ========================================
// Scripting Component (Lua)
// ========================================

/**
 * @brief Component that attaches a Lua script to an entity.
 * 
 * Scripts can define callbacks that are invoked by the ScriptingSystem:
 * - on_init(entity)      : Called once when the entity is created
 * - on_update(entity, dt): Called every frame with delta time
 * - on_collision(entity, other): Called when colliding with another entity
 * - on_damage(entity, amount): Called when the entity takes damage
 * - on_death(entity)     : Called when the entity is destroyed
 * 
 * Scripts can also store custom variables in the 'variables' map
 * that persist across frames and can be modified from C++.
 */
struct ScriptComponent {
    // Path to the Lua script file (relative to scripts/ folder)
    std::string scriptPath{};
    
    // Has the script been initialized (on_init called)?
    bool initialized{false};
    
    // Is the script currently enabled?
    bool enabled{true};
    
    // Custom variables that can be set from C++ and accessed in Lua
    // Supports: float, int, bool, string
    using ScriptVar = std::variant<float, int, bool, std::string>;
    std::unordered_map<std::string, ScriptVar> variables{};
    
    // Internal: Lua state reference (managed by ScriptingSystem)
    // Using void* to avoid Lua header dependency in components
    void* luaStateRef{nullptr};
    
    // Helper methods to set/get variables
    void setVar(const std::string& name, float value) { variables[name] = value; }
    void setVar(const std::string& name, int value) { variables[name] = value; }
    void setVar(const std::string& name, bool value) { variables[name] = value; }
    void setVar(const std::string& name, const std::string& value) { variables[name] = value; }
    
    template<typename T>
    T getVar(const std::string& name, T defaultValue = T{}) const {
        auto it = variables.find(name);
        if (it != variables.end()) {
            if (auto* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        return defaultValue;
    }
};

}