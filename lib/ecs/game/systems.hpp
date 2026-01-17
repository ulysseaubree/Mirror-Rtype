#pragma once

#include "../core/coordinator.hpp"
#include "components.hpp"
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

namespace ecs {

// ==========================================
// Input System
// ==========================================
class InputSystem {
public:
    // "entities" est passé en paramètre, le système est purement logique
    void Update(Coordinator& coordinator, const std::vector<Entity>& entities, float speed = 200.f) {
        for (Entity entity : entities) {
            const auto& input = coordinator.GetComponent<PlayerInput>(entity);
            auto& velocity = coordinator.GetComponent<Velocity>(entity);
            
            velocity.vx = 0.f;
            velocity.vy = 0.f;
            
            switch (input.direction) {
                case 1: velocity.vx = -speed; velocity.vy = speed; break;
                case 2: velocity.vy = speed; break;
                case 3: velocity.vx = speed; velocity.vy = speed; break;
                case 4: velocity.vx = -speed; break;
                case 5: break; // Neutre
                case 6: velocity.vx = speed; break;
                case 7: velocity.vx = -speed; velocity.vy = -speed; break;
                case 8: velocity.vy = -speed; break;
                case 9: velocity.vx = speed; velocity.vy = -speed; break;
            }
            
            // Normalisation diagonale
            if (velocity.vx != 0.f && velocity.vy != 0.f) {
                float magnitude = std::sqrt(velocity.vx * velocity.vx + velocity.vy * velocity.vy);
                velocity.vx = (velocity.vx / magnitude) * speed;
                velocity.vy = (velocity.vy / magnitude) * speed;
            }
        }
    }
};

// ==========================================
// Movement System
// ==========================================
class MovementSystem {
public:
    void Update(Coordinator& coordinator, const std::vector<Entity>& entities, float dt) {
        for (Entity entity : entities) {
            auto& velocity = coordinator.GetComponent<Velocity>(entity);
            auto& transform = coordinator.GetComponent<Transform>(entity);
            
            transform.x += velocity.vx * dt;
            transform.y += velocity.vy * dt;
        }
    }
};

// ==========================================
// AI System
// ==========================================
class AISystem {
public:
    void Update(Coordinator& coordinator, const std::vector<Entity>& entities, float dt) {
        for (Entity entity : entities) {
            auto& controller = coordinator.GetComponent<AIController>(entity);
            auto& transform = coordinator.GetComponent<Transform>(entity);
             
            controller.decisionTimer += dt;
            if (controller.decisionTimer >= controller.decisionCooldown) {
                controller.decisionTimer = 0.f;
                
                if (controller.currentState == AIController::State::Idle) {
                     controller.target = FindClosestTarget(coordinator, entity, transform, controller);
                     if (controller.target != MAX_ENTITIES) {
                         controller.currentState = AIController::State::Chasing;
                     }
                }
            }
            
            if (controller.currentState == AIController::State::Chasing && controller.target != MAX_ENTITIES) {
                // Logique simplifiée pour l'exemple
                try {
                    // Check if target still maintains Component
                     coordinator.GetComponent<Transform>(controller.target);
                } catch (...) {
                    controller.target = MAX_ENTITIES;
                    controller.currentState = AIController::State::Idle;
                }
            }
        }
    }

    Entity FindClosestTarget(Coordinator& coordinator, Entity self, const Transform& selfTransform, const AIController& controller) {
        (void)coordinator; (void)self; (void)selfTransform; (void)controller;
        // Implémentation placeholder
        return MAX_ENTITIES;
    }
};

// ==========================================
// Spawner System
// ==========================================
class SpawnerSystem {
public:
    void Update(Coordinator& coordinator, const std::vector<Entity>& entities, float dt) {
        for (Entity entity : entities) {
            auto& spawner = coordinator.GetComponent<Spawner>(entity);
            
            spawner.spawnTimer += dt;
            if (spawner.spawnTimer >= spawner.spawnCooldown) {
                if (spawner.maxSpawns == -1 || spawner.spawnCount < spawner.maxSpawns) {
                    SpawnEntity(coordinator, entity, spawner);
                    spawner.spawnTimer = 0.f;
                    spawner.spawnCount++;
                }
            }
        }
    }

private:
    void SpawnEntity(Coordinator& coordinator, Entity spawnerEntity, const Spawner& spawnerComp) {
        Entity newEntity = coordinator.CreateEntity();
        
        auto& spawnerTransform = coordinator.GetComponent<Transform>(spawnerEntity);
        Transform transform;
        transform.x = spawnerTransform.x + spawnerComp.spawnOffsetX;
        transform.y = spawnerTransform.y + spawnerComp.spawnOffsetY;
        coordinator.AddComponent(newEntity, transform);
        
        Velocity velocity;
        velocity.vx = spawnerComp.spawnVelocityX;
        velocity.vy = spawnerComp.spawnVelocityY;
        coordinator.AddComponent(newEntity, velocity);
        
        switch (spawnerComp.typeToSpawn) {
            case Spawner::SpawnType::Projectile:
                SetupProjectile(coordinator, newEntity, spawnerEntity);
                break;
            case Spawner::SpawnType::Enemy:
                SetupEnemy(coordinator, newEntity);
                break;
            case Spawner::SpawnType::Powerup:
                SetupPowerup(coordinator, newEntity);
                break;
        }
    }
    
    void SetupProjectile(Coordinator& coordinator, Entity entity, Entity owner) {
        Collider collider;
        collider.shape = Collider::Shape::Circle;
        collider.radius = 5.f;
        coordinator.AddComponent(entity, collider);
        
        Damager damager;
        damager.damage = 10;
        coordinator.AddComponent(entity, damager);
        
        // Try/Catch pour Team car owner peut être mort ou ne pas avoir de team
        try {
            auto& ownerTeam = coordinator.GetComponent<Team>(owner);
            Team team;
            team.teamID = ownerTeam.teamID;
            coordinator.AddComponent(entity, team);
        } catch (...) {
            Team team; team.teamID = 0;
            coordinator.AddComponent(entity, team);
        }
        
        Lifetime lifetime;
        lifetime.timeLeft = 3.f;
        coordinator.AddComponent(entity, lifetime);
        
        Boundary boundary;
        boundary.destroy = true;
        coordinator.AddComponent(entity, boundary);
    }
    
    void SetupEnemy(Coordinator& coordinator, Entity entity) {
        Health health; health.current = 50; health.max = 50;
        coordinator.AddComponent(entity, health);
        
        Collider collider; collider.shape = Collider::Shape::Circle; collider.radius = 20.f;
        coordinator.AddComponent(entity, collider);

        Team team; team.teamID = 2; 
        coordinator.AddComponent(entity, team);
    }

    void SetupPowerup(Coordinator& coordinator, Entity entity) {
        (void)coordinator; (void)entity;
    }
};

// ==========================================
// Collision System
// ==========================================
class CollisionSystem {
public:
    void Update(Coordinator& gCoordinator, const std::vector<Entity>& entities) {
        // Optimisation: On part du principe que "entities" contient les entités avec Collider+Transform
        int size = static_cast<int>(entities.size());
        
        for (int i = 0; i < size; ++i) {
            for (int j = i + 1; j < size; ++j) {
                Entity e1 = entities[i];
                Entity e2 = entities[j];
                
                if (CheckCollision(gCoordinator, e1, e2)) {
                    ResolveCollision(gCoordinator, e1, e2);
                }
            }
        }
    }
    
    bool CheckCollision(Coordinator& gCoordinator, Entity e1, Entity e2) {
        auto& t1 = gCoordinator.GetComponent<Transform>(e1);
        auto& c1 = gCoordinator.GetComponent<Collider>(e1);
        auto& t2 = gCoordinator.GetComponent<Transform>(e2);
        auto& c2 = gCoordinator.GetComponent<Collider>(e2);
        
        float dx = t1.x - t2.x;
        float dy = t1.y - t2.y;
        float distance = std::sqrt(dx*dx + dy*dy);
        
        float r1 = (c1.shape == Collider::Shape::Circle) ? c1.radius : 
                   std::sqrt(c1.width * c1.width + c1.height * c1.height) / 2.f;
        float r2 = (c2.shape == Collider::Shape::Circle) ? c2.radius : 
                   std::sqrt(c2.width * c2.width + c2.height * c2.height) / 2.f;
        
        return distance < (r1 + r2);
    }
    
    void ResolveCollision(Coordinator& gCoordinator, Entity e1, Entity e2) {
        Signature sig1 = gCoordinator.GetSignature(e1);
        Signature sig2 = gCoordinator.GetSignature(e2);
        
        ComponentType damagerType = gCoordinator.GetComponentType<Damager>();
        ComponentType healthType = gCoordinator.GetComponentType<Health>();
        
        auto& team1 = gCoordinator.GetComponent<Team>(e1);
        auto& team2 = gCoordinator.GetComponent<Team>(e2);
        
        if (team1.teamID == team2.teamID) return;
        
        ApplyDamage(gCoordinator, e1, e2, sig1, sig2, damagerType, healthType);
        ApplyDamage(gCoordinator, e2, e1, sig2, sig1, damagerType, healthType);
    }

private:
    void ApplyDamage(Coordinator& coordinator, Entity attacker, Entity victim, 
                     Signature sigAttacker, Signature sigVictim, 
                     ComponentType damagerType, ComponentType healthType) {
        
        if (sigAttacker.test(damagerType) && sigVictim.test(healthType)) {
            auto& damager = coordinator.GetComponent<Damager>(attacker);
            auto& health = coordinator.GetComponent<Health>(victim);
            
            if (damager.damage > 0 && !health.invincible && health.invincibilityTimer <= 0.f) {
                health.current -= damager.damage;
                
                if (coordinator.GetSignature(attacker).test(coordinator.GetComponentType<Boundary>())) {
                    auto& bound = coordinator.GetComponent<Boundary>(attacker);
                    if (bound.destroy) {
                        coordinator.DestroyEntity(attacker);
                    }
                }
            }
        }
    }
};

// ==========================================
// Other Systems
// ==========================================

class LifetimeSystem {
public:
    void Update(Coordinator& coordinator, const std::vector<Entity>& entities, float dt) {
        std::vector<Entity> toDelete;
        for (Entity entity : entities) {
            auto& life = coordinator.GetComponent<Lifetime>(entity);
            life.timeLeft -= dt;
            if (life.timeLeft <= 0.f) {
                toDelete.push_back(entity);
            }
        }
        for (Entity e : toDelete) {
            coordinator.DestroyEntity(e);
        }
    }
};

class HealthSystem {
public:
    void Update(Coordinator& coordinator, const std::vector<Entity>& entities, float dt) {
        std::vector<Entity> toDelete;
        for (Entity entity : entities) {
            auto& health = coordinator.GetComponent<Health>(entity);
            if (health.invincibilityTimer > 0.f) {
                health.invincibilityTimer -= dt;
            }
            if (health.current <= 0) {
                toDelete.push_back(entity);
            }
        }
        for (Entity e : toDelete) {
            coordinator.DestroyEntity(e);
        }
    }
};

class BoundarySystem {
public:
    float minX = -100.f, maxX = 2020.f;
    float minY = -100.f, maxY = 1180.f;

    void Update(Coordinator& coordinator, const std::vector<Entity>& entities) {
        std::vector<Entity> toDelete;
        for (Entity entity : entities) {
            auto& transform = coordinator.GetComponent<Transform>(entity);
            auto& bound = coordinator.GetComponent<Boundary>(entity);
            
            bool outside = transform.x < minX || transform.x > maxX || transform.y < minY || transform.y > maxY;
            
            if (outside) {
                if (bound.destroy) {
                   toDelete.push_back(entity);
                } else {
                    if (transform.x < 0) transform.x = 0;
                    if (transform.x > 1920) transform.x = 1920;
                }
            }
        }
        for (Entity e : toDelete) {
            coordinator.DestroyEntity(e);
        }
    }
};

} // namespace ecs</document>
