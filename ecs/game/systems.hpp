#pragma once

#include "ecs.hpp"
#include "components.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <algorithm>

namespace ecs {

// === Movement System ===
class MovementSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& transform = gCoordinator.GetComponent<Transform>(entity);
            const auto& velocity = gCoordinator.GetComponent<Velocity>(entity);
            
            transform.x += velocity.vx * dt;
            transform.y += velocity.vy * dt;
        }
    }
};

// === Input System ===
class InputSystem : public System {
public:
    void Update(float speed = 200.f) {
        for (Entity entity : entities) {
            const auto& input = gCoordinator.GetComponent<PlayerInput>(entity);
            auto& velocity = gCoordinator.GetComponent<Velocity>(entity);
            
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
};

// === AI System ===
class AISystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& ai = gCoordinator.GetComponent<AIController>(entity);
            auto& transform = gCoordinator.GetComponent<Transform>(entity);
            auto& velocity = gCoordinator.GetComponent<Velocity>(entity);
            
            ai.decisionTimer += dt;
            
            // Prendre des décisions périodiquement
            if (ai.decisionTimer >= ai.decisionCooldown) {
                ai.decisionTimer = 0.f;
                UpdateAIState(entity, ai, transform);
            }
            
            // Exécuter le comportement actuel
            ExecuteBehavior(entity, ai, transform, velocity);
        }
    }
    
private:
    void UpdateAIState(Entity entity, AIController& ai, const Transform& transform) {
        // Vérifier la santé pour fuir si nécessaire
        auto& health = gCoordinator.GetComponent<Health>(entity);
        float healthRatio = static_cast<float>(health.current) / health.max;
        
        if (healthRatio < ai.fleeHealthThreshold) {
            ai.currentState = AIController::State::Fleeing;
            return;
        }
        
        // Chercher une cible (joueur ou ennemi selon le team)
        Entity closestTarget = FindClosestTarget(entity, transform, ai);
        
        if (closestTarget != MAX_ENTITIES) {
            ai.target = closestTarget;
            float distance = GetDistance(entity, closestTarget);
            
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
    
    void ExecuteBehavior(Entity entity, const AIController& ai, 
                        const Transform& transform, Velocity& velocity) {
        switch (ai.currentState) {
            case AIController::State::Idle:
                velocity.vx = 0.f;
                velocity.vy = 0.f;
                break;
                
            case AIController::State::Patrolling:
                // Simple patrol: déplacement lent aléatoire
                velocity.vx = 50.f;
                velocity.vy = 0.f;
                break;
                
            case AIController::State::Chasing:
                if (ai.target != MAX_ENTITIES) {
                    MoveTowardsTarget(entity, ai.target, velocity, 150.f);
                }
                break;
                
            case AIController::State::Fleeing:
                if (ai.target != MAX_ENTITIES) {
                    MoveAwayFromTarget(entity, ai.target, velocity, 200.f);
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
    
    Entity FindClosestTarget(Entity self, const Transform& selfTransform, 
                            const AIController& ai) {
        auto& selfTeam = gCoordinator.GetComponent<Team>(self);
        Entity closest = MAX_ENTITIES;
        float closestDist = ai.detectionRange;
        
        // Parcourir toutes les entités avec Team et Transform
        // (Dans une vraie implémentation, vous voudriez un système de requête plus efficace)
        for (Entity other : entities) {
            if (other == self) continue;
            
            auto& otherTeam = gCoordinator.GetComponent<Team>(other);
            if (otherTeam.teamID == selfTeam.teamID) continue; // Même équipe
            
            float dist = GetDistance(self, other);
            if (dist < closestDist) {
                closestDist = dist;
                closest = other;
            }
        }
        
        return closest;
    }
    
    float GetDistance(Entity e1, Entity e2) {
        const auto& t1 = gCoordinator.GetComponent<Transform>(e1);
        const auto& t2 = gCoordinator.GetComponent<Transform>(e2);
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    void MoveTowardsTarget(Entity self, Entity target, Velocity& velocity, float speed) {
        const auto& selfTransform = gCoordinator.GetComponent<Transform>(self);
        const auto& targetTransform = gCoordinator.GetComponent<Transform>(target);
        
        float dx = targetTransform.x - selfTransform.x;
        float dy = targetTransform.y - selfTransform.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        if (dist > 0.f) {
            velocity.vx = (dx / dist) * speed;
            velocity.vy = (dy / dist) * speed;
        }
    }
    
    void MoveAwayFromTarget(Entity self, Entity target, Velocity& velocity, float speed) {
        const auto& selfTransform = gCoordinator.GetComponent<Transform>(self);
        const auto& targetTransform = gCoordinator.GetComponent<Transform>(target);
        
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
class SpawnerSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& spawner = gCoordinator.GetComponent<Spawner>(entity);
            
            spawner.spawnTimer += dt;
            
            if (spawner.spawnTimer >= spawner.spawnCooldown) {
                spawner.spawnTimer = 0.f;
                
                // Vérifier si on peut encore spawn
                if (spawner.maxSpawns == -1 || spawner.spawnCount < spawner.maxSpawns) {
                    SpawnEntity(entity, spawner);
                    spawner.spawnCount++;
                }
            }
        }
    }
    
private:
    void SpawnEntity(Entity spawner, const Spawner& spawnerComp) {
        const auto& spawnerTransform = gCoordinator.GetComponent<Transform>(spawner);
        
        Entity newEntity = gCoordinator.CreateEntity();
        
        // Position du spawn
        Transform transform;
        transform.x = spawnerTransform.x + spawnerComp.spawnOffsetX;
        transform.y = spawnerTransform.y + spawnerComp.spawnOffsetY;
        gCoordinator.AddComponent(newEntity, transform);
        
        // Vélocité du spawn
        Velocity velocity;
        velocity.vx = spawnerComp.spawnVelocityX;
        velocity.vy = spawnerComp.spawnVelocityY;
        gCoordinator.AddComponent(newEntity, velocity);
        
        // Configuration selon le type
        switch (spawnerComp.typeToSpawn) {
            case Spawner::SpawnType::Projectile:
                SetupProjectile(newEntity, spawner);
                break;
            case Spawner::SpawnType::Enemy:
                SetupEnemy(newEntity);
                break;
            case Spawner::SpawnType::Powerup:
                SetupPowerup(newEntity);
                break;
        }
    }
    
    void SetupProjectile(Entity entity, Entity owner) {
        // Collider
        Collider collider;
        collider.shape = Collider::Shape::Circle;
        collider.radius = 5.f;
        gCoordinator.AddComponent(entity, collider);
        
        // Damager
        Damager damager;
        damager.damage = 10;
        gCoordinator.AddComponent(entity, damager);
        
        // Team (hérite du spawner)
        auto& ownerTeam = gCoordinator.GetComponent<Team>(owner);
        Team team;
        team.teamID = ownerTeam.teamID;
        gCoordinator.AddComponent(entity, team);
        
        // Lifetime
        Lifetime lifetime;
        lifetime.timeLeft = 3.f;
        gCoordinator.AddComponent(entity, lifetime);
        
        // Boundary (détruit hors écran)
        Boundary boundary;
        boundary.destroy = true;
        gCoordinator.AddComponent(entity, boundary);
    }
    
    void SetupEnemy(Entity entity) {
        // Health
        Health health;
        health.current = 50;
        health.max = 50;
        gCoordinator.AddComponent(entity, health);
        
        // Collider
        Collider collider;
        collider.shape = Collider::Shape::Circle;
        collider.radius = 20.f;
        gCoordinator.AddComponent(entity, collider);
        
        // Team
        Team team;
        team.teamID = 1; // Ennemi
        gCoordinator.AddComponent(entity, team);
        
        // AI
        AIController ai;
        ai.currentState = AIController::State::Patrolling;
        gCoordinator.AddComponent(entity, ai);
        
        // Damager
        Damager damager;
        damager.damage = 20;
        gCoordinator.AddComponent(entity, damager);
    }
    
    void SetupPowerup(Entity entity) {
        // Collider (trigger)
        Collider collider;
        collider.shape = Collider::Shape::Circle;
        collider.radius = 15.f;
        collider.isTrigger = true;
        gCoordinator.AddComponent(entity, collider);
        
        // Team (neutre)
        Team team;
        team.teamID = 2;
        gCoordinator.AddComponent(entity, team);
        
        // Lifetime
        Lifetime lifetime;
        lifetime.timeLeft = 10.f;
        gCoordinator.AddComponent(entity, lifetime);
    }
};

// === Collision System ===
class CollisionSystem : public System {
public:
    void Update() {
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                Entity e1 = entities[i];
                Entity e2 = entities[j];
                
                const auto& t1 = gCoordinator.GetComponent<Transform>(e1);
                const auto& t2 = gCoordinator.GetComponent<Transform>(e2);
                const auto& c1 = gCoordinator.GetComponent<Collider>(e1);
                const auto& c2 = gCoordinator.GetComponent<Collider>(e2);
                
                if (CheckCollision(t1, c1, t2, c2)) {
                    ResolveCollision(e1, e2);
                }
            }
        }
    }
    
private:
    bool CheckCollision(const Transform& t1, const Collider& c1,
                       const Transform& t2, const Collider& c2) {
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // Simple circle collision
        if (c1.shape == Collider::Shape::Circle && c2.shape == Collider::Shape::Circle) {
            return distance < (c1.radius + c2.radius);
        }
        
        // Box collision approximée par cercle
        float r1 = (c1.shape == Collider::Shape::Circle) ? c1.radius : 
                   std::sqrt(c1.width * c1.width + c1.height * c1.height) / 2.f;
        float r2 = (c2.shape == Collider::Shape::Circle) ? c2.radius : 
                   std::sqrt(c2.width * c2.width + c2.height * c2.height) / 2.f;
        
        return distance < (r1 + r2);
    }
    
    void ResolveCollision(Entity e1, Entity e2) {
        // Récupérer les teams
        auto& team1 = gCoordinator.GetComponent<Team>(e1);
        auto& team2 = gCoordinator.GetComponent<Team>(e2);
        
        // Ignorer les collisions entre même team (tir ami)
        if (team1.teamID == team2.teamID) return;
        
        // Vérifier si e1 peut endommager e2
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
        
        // Vérifier si e2 peut endommager e1 (collision bidirectionnelle)
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
};

// === Lifetime System ===
class LifetimeSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& lifetime = gCoordinator.GetComponent<Lifetime>(entity);
            lifetime.timeLeft -= dt;
            
            if (lifetime.timeLeft <= 0.f) {
                gCoordinator.RequestDestroyEntity(entity);
            }
        }
    }
};

// === Boundary System ===
class BoundarySystem : public System {
public:
    void Update() {
        for (Entity entity : entities) {
            auto& transform = gCoordinator.GetComponent<Transform>(entity);
            const auto& boundary = gCoordinator.GetComponent<Boundary>(entity);
            
            if (boundary.wrap) {
                // Wraparound
                if (transform.x < boundary.minX) transform.x = boundary.maxX;
                if (transform.x > boundary.maxX) transform.x = boundary.minX;
                if (transform.y < boundary.minY) transform.y = boundary.maxY;
                if (transform.y > boundary.maxY) transform.y = boundary.minY;
            } else if (boundary.destroy) {
                // Détruire si hors limites
                if (transform.x < boundary.minX || transform.x > boundary.maxX ||
                    transform.y < boundary.minY || transform.y > boundary.maxY) {
                    gCoordinator.RequestDestroyEntity(entity);
                }
            } else {
                // Clamp aux limites
                transform.x = std::clamp(transform.x, boundary.minX, boundary.maxX);
                transform.y = std::clamp(transform.y, boundary.minY, boundary.maxY);
            }
        }
    }
};

// === Health System ===
class HealthSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& health = gCoordinator.GetComponent<Health>(entity);
            
            if (health.invincibilityTimer > 0.f) {
                health.invincibilityTimer -= dt;
            }
            
            if (health.current <= 0) {
                gCoordinator.RequestDestroyEntity(entity);
            }
        }
    }
};

}