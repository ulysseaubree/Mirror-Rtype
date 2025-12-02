#pragma once

#include "ecs.hpp"
#include "components.hpp"
#include "systems.hpp"
#include <memory>

namespace ecs {

// Structure pour stocker les références aux systèmes
struct SystemRefs {
    std::shared_ptr<InputSystem> inputSystem;
    std::shared_ptr<AISystem> aiSystem;
    std::shared_ptr<MovementSystem> movementSystem;
    std::shared_ptr<CollisionSystem> collisionSystem;
    std::shared_ptr<SpawnerSystem> spawnerSystem;
    std::shared_ptr<HealthSystem> healthSystem;
    std::shared_ptr<LifetimeSystem> lifetimeSystem;
    std::shared_ptr<BoundarySystem> boundarySystem;
};

// Initialise tout l'ECS : Coordinator, Composants et Systèmes
SystemRefs InitECS();

} // namespace ecs