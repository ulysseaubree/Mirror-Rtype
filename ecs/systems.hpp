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

// === Player Input System ===
class PlayerInputSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& velocity = gCoordinator.GetComponent<Velocity>(entity);
            auto& transform = gCoordinator.GetComponent<Transform>(entity);
            const auto& control = gCoordinator.GetComponent<PlayerControl>(entity);
            
            velocity.vx = 0.f;
            velocity.vy = 0.f;
            
            // Déplacement WASD ou flèches
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || 
                sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                velocity.vy = -control.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || 
                sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                velocity.vy = control.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || 
                sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                velocity.vx = -control.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || 
                sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                velocity.vx = control.speed;
            }
            
            // Normaliser la vitesse en diagonale
            if (velocity.vx != 0.f && velocity.vy != 0.f) {
                float factor = control.speed / std::sqrt(velocity.vx * velocity.vx + velocity.vy * velocity.vy);
                velocity.vx *= factor;
                velocity.vy *= factor;
            }
            
            // Rotation vers la souris (optionnel)
            if (pWindow) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(*pWindow);
                float dx = static_cast<float>(mousePos.x) - transform.x;
                float dy = static_cast<float>(mousePos.y) - transform.y;
                transform.rotation = std::atan2(dy, dx) * 180.f / 3.14159f;
            }
        }
    }
    
    sf::RenderWindow* pWindow{nullptr};
};

// === Render System ===
class RenderSystem : public System {
public:
    void Render(sf::RenderWindow& window) {
        for (Entity entity : entities) {
            const auto& transform = gCoordinator.GetComponent<Transform>(entity);
            const auto& shape = gCoordinator.GetComponent<RenderShape>(entity);
            
            switch (shape.type) {
                case RenderShape::Type::Circle: {
                    sf::CircleShape circle(shape.radius);
                    circle.setFillColor(shape.color);
                    circle.setOrigin(shape.radius, shape.radius);
                    circle.setPosition(transform.x, transform.y);
                    circle.setRotation(transform.rotation);
                    window.draw(circle);
                    break;
                }
                case RenderShape::Type::Rectangle: {
                    sf::RectangleShape rect(sf::Vector2f(shape.width, shape.height));
                    rect.setFillColor(shape.color);
                    rect.setOrigin(shape.width / 2.f, shape.height / 2.f);
                    rect.setPosition(transform.x, transform.y);
                    rect.setRotation(transform.rotation);
                    window.draw(rect);
                    break;
                }
                case RenderShape::Type::Triangle: {
                    sf::CircleShape triangle(shape.radius, 3);
                    triangle.setFillColor(shape.color);
                    triangle.setOrigin(shape.radius, shape.radius);
                    triangle.setPosition(transform.x, transform.y);
                    triangle.setRotation(transform.rotation);
                    window.draw(triangle);
                    break;
                }
            }
        }
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
                    collisions.push_back({e1, e2});
                }
            }
        }
    }
    
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
    
    std::vector<std::pair<Entity, Entity>> collisions;
};

// === Lifetime System ===
class LifetimeSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& lifetime = gCoordinator.GetComponent<Lifetime>(entity);
            lifetime.timeLeft -= dt;
            
            if (lifetime.timeLeft <= 0.f) {
                toDestroy.push_back(entity);
            }
        }
    }
    
    std::vector<Entity> toDestroy;
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
                    toDestroy.push_back(entity);
                }
            } else {
                // Clamp aux limites
                transform.x = std::clamp(transform.x, boundary.minX, boundary.maxX);
                transform.y = std::clamp(transform.y, boundary.minY, boundary.maxY);
            }
        }
    }
    
    std::vector<Entity> toDestroy;
};

// === Rotation System ===
class RotationSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& transform = gCoordinator.GetComponent<Transform>(entity);
            const auto& rotation = gCoordinator.GetComponent<RotationSpeed>(entity);
            
            transform.rotation += rotation.speed * dt;
            
            // Normaliser l'angle
            while (transform.rotation >= 360.f) transform.rotation -= 360.f;
            while (transform.rotation < 0.f) transform.rotation += 360.f;
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
                toDestroy.push_back(entity);
            }
        }
    }
    
    std::vector<Entity> toDestroy;
};

} // namespace ecs