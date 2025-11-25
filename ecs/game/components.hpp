#pragma once

#include <SFML/Graphics.hpp>
#include "types.hpp"

namespace ecs {

// === Core Components ===

struct Transform {
    float x{};
    float y{};
    float rotation{}; // en degrés
};

struct Velocity {
    float vx{};
    float vy{};
};

struct RenderShape {
    enum class Type {
        Circle,
        Rectangle,
        Triangle
    };
    
    Type type{Type::Circle};
    sf::Color color{sf::Color::White};
    float width{20.f};
    float height{20.f};
    float radius{10.f};
};

struct PlayerControl {
    float speed{200.f};
    float rotationSpeed{180.f};
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

struct Enemy {
    float damage{10.f};
};

struct Projectile {
    float damage{25.f};
    Entity owner{};
};

struct Score {
    int points{0};
};

struct Boundary {
    float minX{0.f};
    float maxX{800.f};
    float minY{0.f};
    float maxY{600.f};
    bool wrap{false}; // wraparound style Asteroids
    bool destroy{true}; // détruire si sort des limites
};

struct RotationSpeed {
    float speed{90.f}; // degrés par seconde
};

} // namespace ecs