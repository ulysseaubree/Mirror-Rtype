#pragma once

#include "../core/coordinator.hpp"
#include "components.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>
#include <algorithm>

namespace ecs {

// === Movement System ===
class MovementSystem {
public:
    void Update(Coordinator& coordinator, float dt) {
        for (Entity entity : entities) {
            auto& transform = coordinator.GetComponent<Transform>(entity);
            const auto& velocity = coordinator.GetComponent<Velocity>(entity);
            
            transform.x += velocity.vx * dt;
            transform.y += velocity.vy * dt;
        }
    }
    
    std::vector<Entity> entities{};
};

// === Input System ===
class InputSystem {
public:
    void Update(Coordinator& coordinator, float speed = 200.f) {
        for (Entity entity : entities) {
            const auto& input = coordinator.GetComponent<PlayerInput>(entity);
            auto& velocity = coordinator.GetComponent<Velocity>(entity);
            
            // Convertit direction (1-9) en vecteur de vélocité
            velocity.vx = 0.f;
            velocity.vy = 0.f;
            
            switch (input.direction) {
                case 1: velocity.vx = -speed; velocity.vy = speed; break;   // Bas-gauche
                case 2: velocity.vy = speed; break;                          // Bas
                case 3: velocity.vx = speed; velocity.vy = speed; break;    // Bas-droite
                case 4: velocity.vx = -speed; break;                         // Gauche
                case 5: break;                                               // Neutre
                case 6: velocity.vx = speed; break;                          // Droite
                case 7: velocity.vx = -speed; velocity.vy = -speed; break;  // Haut-gauche
                case 8: velocity.vy = -speed; break;                         // Haut
                case 9: velocity.vx = speed; velocity.vy = -speed; break;   // Haut-droite
            }
            
            // Normalisation pour les diagonales
            if (velocity.vx != 0.f && velocity.vy != 0.f) {
                float magnitude = std::sqrt(velocity.vx * velocity.vx + velocity.vy * velocity.vy);
                velocity.vx = (velocity.vx / magnitude) * speed;
                velocity.vy = (velocity.vy / magnitude) * speed;
            }
        }
    }
    
    std::vector<Entity> entities{};
};

// === AI System ===
class AISystem {
public:
    void Update(Coordinator& coordinator, float dt) {
        for (Entity entity : entities) {
            auto& ai = coordinator.GetComponent<AIController>(entity);
            auto& transform = coordinator.GetComponent<Transform>(entity);
            auto& velocity = coordinator.GetComponent<Velocity>(entity);
            
            ai.decisionTimer += dt;
            
            // Prendre des décisions périodiquement
            if (ai.decisionTimer >= ai.decisionCooldown) {
                ai.decisionTimer = 0.f;
                UpdateAIState(coordinator, entity, ai, transform);
            }
            
            // Exécuter le comportement actuel
            ExecuteBehavior(coordinator, entity, ai, transform, velocity);
        }
    }
    
    std::vector<Entity> entities{};
    
private:
    void UpdateAIState(Coordinator& coordinator, Entity entity, AIController& ai, const Transform& transform) {
        // Vérifier la santé pour fuir si nécessaire
        auto& health = coordinator.GetComponent<Health>(entity);
        float healthRatio = static_cast<float>(health.current) / health.max;
        
        if (healthRatio < ai.fleeHealthThreshold) {
            ai.currentState = AIController::State::Fleeing;
            return;
        }
        
        // Chercher une cible (joueur ou ennemi selon le team)
        Entity closestTarget = FindClosestTarget(coordinator, entity, transform, ai);
        
        if (closestTarget != MAX_ENTITIES) {
            ai.target = closestTarget;
            float distance = GetDistance(coordinator, entity, closestTarget);
            
            if (distance <= ai.attackRange) {
                ai.currentState = AIController::State::Attacking;
            } else if (distance <= ai.detectionRange) {
                ai.currentState = AIController::State::Chasing;
            } else {
                ai.currentState = AIController::State::Patrolling;
            }
        } else {
            ai.currentState = AIController::State::Patrolling;
            ai.target = MAX_ENTITIES;
        }
    }
    
    void ExecuteBehavior(Coordinator& coordinator, Entity entity, const AIController& ai, 
                        const Transform& transform, Velocity& velocity) {
        switch (ai.currentState) {
            case AIController::State::Idle:
                velocity.vx = 0.f;
                velocity.vy = 0.f;
                break;
                
            case AIController::State::Patrolling:
                velocity.vx = -80.f;
                {
                    float phase = transform.x * 0.05f;
                    velocity.vy = std::sin(phase) * 40.f;
                }
                break;
                
            case AIController::State::Chasing:
                if (ai.target != MAX_ENTITIES) {
                    MoveTowardsTarget(coordinator, entity, ai.target, velocity, 150.f);
                }
                break;
                
            case AIController::State::Fleeing:
                if (ai.target != MAX_ENTITIES) {
                    MoveAwayFromTarget(coordinator, entity, ai.target, velocity, 200.f);
                } else {
                    velocity.vx = -100.f;
                    velocity.vy = 0.f;
                }
                break;
                
            case AIController::State::Attacking:
                // Ralentir pour attaquer
                velocity.vx = 0.f;
                velocity.vy = 0.f;
                break;
        }
    }
    
    Entity FindClosestTarget(Coordinator& coordinator, Entity self, const Transform& selfTransform, 
                            const AIController& ai) {
        auto& selfTeam = coordinator.GetComponent<Team>(self);
        Entity closest = MAX_ENTITIES;
        float closestDist = ai.detectionRange;
        
        for (Entity other : entities) {
            if (other == self) continue;
            
            auto& otherTeam = coordinator.GetComponent<Team>(other);
            if (otherTeam.teamID == selfTeam.teamID) continue;
            
            float dist = GetDistance(coordinator, self, other);
            if (dist < closestDist) {
                closestDist = dist;
                closest = other;
            }
        }
        
        return closest;
    }
    
    float GetDistance(Coordinator& coordinator, Entity e1, Entity e2) {
        const auto& t1 = coordinator.GetComponent<Transform>(e1);
        const auto& t2 = coordinator.GetComponent<Transform>(e2);
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    void MoveTowardsTarget(Coordinator& coordinator, Entity self, Entity target, Velocity& velocity, float speed) {
        const auto& selfTransform = coordinator.GetComponent<Transform>(self);
        const auto& targetTransform = coordinator.GetComponent<Transform>(target);
        
        float dx = targetTransform.x - selfTransform.x;
        float dy = targetTransform.y - selfTransform.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        if (dist > 0.f) {
            velocity.vx = (dx / dist) * speed;
            velocity.vy = (dy / dist) * speed;
        }
    }
    
    void MoveAwayFromTarget(Coordinator& coordinator, Entity self, Entity target, Velocity& velocity, float speed) {
        const auto& selfTransform = coordinator.GetComponent<Transform>(self);
        const auto& targetTransform = coordinator.GetComponent<Transform>(target);
        
        float dx = selfTransform.x - targetTransform.x;
        float dy = selfTransform.y - targetTransform.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        if (dist > 0.f) {
            velocity.vx = (dx / dist) * speed;
            velocity.vy = (dy / dist) * speed;
        }
    }
};

// === Spawner System ===
class SpawnerSystem {
public:
    void Update(Coordinator& coordinator, float dt) {
        for (Entity entity : entities) {
            auto& spawner = coordinator.GetComponent<Spawner>(entity);
            
            spawner.spawnTimer += dt;
            
            if (spawner.spawnTimer >= spawner.spawnCooldown) {
                spawner.spawnTimer = 0.f;
                
                if (spawner.maxSpawns == -1 || spawner.spawnCount < spawner.maxSpawns) {
                    SpawnEntity(coordinator, entity, spawner);
                    spawner.spawnCount++;
                }
            }
        }
    }
    
    std::vector<Entity> entities{};
    
private:
    void SpawnEntity(Coordinator& coordinator, Entity spawner, const Spawner& spawnerComp) {
        const auto& spawnerTransform = coordinator.GetComponent<Transform>(spawner);
        
        Entity newEntity = coordinator.CreateEntity();
        
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
                SetupProjectile(coordinator, newEntity, spawner);
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
        
        auto& ownerTeam = coordinator.GetComponent<Team>(owner);
        Team team;
        team.teamID = ownerTeam.teamID;
        coordinator.AddComponent(entity, team);
        
        Lifetime lifetime;
        lifetime.timeLeft = 3.f;
        coordinator.AddComponent(entity, lifetime);
        
        Boundary boundary;
        boundary.destroy = true;
        coordinator.AddComponent(entity, boundary);
    }
    
    void SetupEnemy(Coordinator& coordinator, Entity entity) {
        Health health;
        health.current = 50;
        health.max = 50;
        coordinator.AddComponent(entity, health);
        
        Collider collider;
        collider.shape = Collider::Shape::Circle;
        collider.radius = 20.f;
        coordinator.AddComponent(entity, collider);
        
        Team team;
        team.teamID = 1;
        coordinator.AddComponent(entity, team);
        
        AIController ai;
        ai.currentState = AIController::State::Patrolling;
        coordinator.AddComponent(entity, ai);
        
        Damager damager;
        damager.damage = 20;
        coordinator.AddComponent(entity, damager);
    }
    
    void SetupPowerup(Coordinator& coordinator, Entity entity) {
        Collider collider;
        collider.shape = Collider::Shape::Circle;
        collider.radius = 15.f;
        collider.isTrigger = true;
        coordinator.AddComponent(entity, collider);
        
        Team team;
        team.teamID = 2;
        coordinator.AddComponent(entity, team);
        
        Lifetime lifetime;
        lifetime.timeLeft = 10.f;
        coordinator.AddComponent(entity, lifetime);
    }
};

// === Collision System ===
class CollisionSystem {
public:
    void Update(Coordinator& coordinator) {
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                Entity e1 = entities[i];
                Entity e2 = entities[j];
                
                const auto& t1 = coordinator.GetComponent<Transform>(e1);
                const auto& t2 = coordinator.GetComponent<Transform>(e2);
                const auto& c1 = coordinator.GetComponent<Collider>(e1);
                const auto& c2 = coordinator.GetComponent<Collider>(e2);
                
                if (CheckCollision(t1, c1, t2, c2)) {
                    ResolveCollision(coordinator, e1, e2);
                }
            }
        }
    }
    
    std::vector<Entity> entities{};
    
private:
    bool CheckCollision(const Transform& t1, const Collider& c1,
                       const Transform& t2, const Collider& c2) {
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        if (c1.shape == Collider::Shape::Circle && c2.shape == Collider::Shape::Circle) {
            return distance < (c1.radius + c2.radius);
        }
        
        float r1 = (c1.shape == Collider::Shape::Circle) ? c1.radius : 
                   std::sqrt(c1.width * c1.width + c1.height * c1.height) / 2.f;
        float r2 = (c2.shape == Collider::Shape::Circle) ? c2.radius : 
                   std::sqrt(c2.width * c2.width + c2.height * c2.height) / 2.f;
        
        return distance < (r1 + r2);
    }
    
    void ResolveCollision(Entity e1, Entity e2) {
        // Vérifier que les deux entités ont le composant Team
        Signature sig1 = gCoordinator.GetSignature(e1);
        Signature sig2 = gCoordinator.GetSignature(e2);
        
        ComponentType teamType = gCoordinator.GetComponentType<Team>();
        ComponentType damagerType = gCoordinator.GetComponentType<Damager>();
        ComponentType healthType = gCoordinator.GetComponentType<Health>();
        ComponentType colliderType = gCoordinator.GetComponentType<Collider>();
        
        // Les deux doivent avoir Team (déjà garanti par la signature du système)
        auto& team1 = gCoordinator.GetComponent<Team>(e1);
        auto& team2 = gCoordinator.GetComponent<Team>(e2);
        
        if (team1.teamID == team2.teamID) return;
        
        // Vérifier si e1 peut endommager e2
        bool e1HasDamager = sig1.test(damagerType);
        bool e2HasHealth = sig2.test(healthType);
        
        if (e1HasDamager && e2HasHealth) {
            auto& damager1 = gCoordinator.GetComponent<Damager>(e1);
            auto& health2 = gCoordinator.GetComponent<Health>(e2);
            
            if (damager1.damage > 0 && !health2.invincible && health2.invincibilityTimer <= 0.f) {
                health2.current -= damager1.damage;
                health2.invincibilityTimer = 0.5f; // 0.5s d'invincibilité
                
                // Détruire le projectile après impact
                auto& collider1 = gCoordinator.GetComponent<Collider>(e1);
                if (!collider1.isTrigger) {
                    gCoordinator.RequestDestroyEntity(e1);
                }
            }
        }
        
        // Vérifier si e2 peut endommager e1 (collision bidirectionnelle)
        bool e2HasDamager = sig2.test(damagerType);
        bool e1HasHealth = sig1.test(healthType);
        
        if (e2HasDamager && e1HasHealth) {
            auto& damager2 = gCoordinator.GetComponent<Damager>(e2);
            auto& health1 = gCoordinator.GetComponent<Health>(e1);
            
            if (damager2.damage > 0 && !health1.invincible && health1.invincibilityTimer <= 0.f) {
                health1.current -= damager2.damage;
                health1.invincibilityTimer = 0.5f;
                
                auto& collider2 = gCoordinator.GetComponent<Collider>(e2);
                if (!collider2.isTrigger) {
                    gCoordinator.RequestDestroyEntity(e2);
                }
            }
        }
    }
};

// === Lifetime System ===
class LifetimeSystem {
public:
    void Update(Coordinator& coordinator, float dt) {
        for (Entity entity : entities) {
            auto& lifetime = coordinator.GetComponent<Lifetime>(entity);
            lifetime.timeLeft -= dt;
            
            if (lifetime.timeLeft <= 0.f) {
                coordinator.RequestDestroyEntity(entity);
            }
        }
    }
    
    std::vector<Entity> entities{};
};

// === Boundary System ===
class BoundarySystem {
public:
    void Update(Coordinator& coordinator) {
        for (Entity entity : entities) {
            auto& transform = coordinator.GetComponent<Transform>(entity);
            const auto& boundary = coordinator.GetComponent<Boundary>(entity);
            
            if (boundary.wrap) {
                if (transform.x < boundary.minX) transform.x = boundary.maxX;
                if (transform.x > boundary.maxX) transform.x = boundary.minX;
                if (transform.y < boundary.minY) transform.y = boundary.maxY;
                if (transform.y > boundary.maxY) transform.y = boundary.minY;
            } else if (boundary.destroy) {
                if (transform.x < boundary.minX || transform.x > boundary.maxX ||
                    transform.y < boundary.minY || transform.y > boundary.maxY) {
                    coordinator.RequestDestroyEntity(entity);
                }
            } else {
                transform.x = std::min(std::max(transform.x, boundary.minX), boundary.maxX);
                transform.y = std::min(std::max(transform.y, boundary.minY), boundary.maxY);
            }
        }
    }
    
    std::vector<Entity> entities{};
};

// === Health System ===
class HealthSystem {
public:
    void Update(Coordinator& coordinator, float dt) {
        for (Entity entity : entities) {
            auto& health = coordinator.GetComponent<Health>(entity);
            
            if (health.invincibilityTimer > 0.f) {
                health.invincibilityTimer -= dt;
            }
            
            if (health.current <= 0) {
                coordinator.RequestDestroyEntity(entity);
            }
        }
    }
    
    std::vector<Entity> entities{};
};

}