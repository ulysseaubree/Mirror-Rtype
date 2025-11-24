#pragma once

#include "ecs.hpp"

namespace ecs {

struct Transform {
    float x{};
    float y{};
};

struct Velocity {
    float vx{};
    float vy{};
};

// Moves entities according to their velocity.
class MovementSystem : public System {
public:
    void Update(float dt)
    {
        for (Entity entity : entities) {
            auto& transform = gCoordinator.GetComponent<Transform>(entity);
            const auto& velocity = gCoordinator.GetComponent<Velocity>(entity);
            transform.x += velocity.vx * dt;
            transform.y += velocity.vy * dt;
        }
    }
};

inline void ExampleEcsUsage()
{
    gCoordinator.Init();

    gCoordinator.RegisterComponent<Transform>();
    gCoordinator.RegisterComponent<Velocity>();

    auto movementSystem = gCoordinator.RegisterSystem<MovementSystem>();
    Signature movementSignature;
    movementSignature.set(gCoordinator.GetComponentType<Transform>(), true);
    movementSignature.set(gCoordinator.GetComponentType<Velocity>(), true);
    gCoordinator.SetSystemSignature<MovementSystem>(movementSignature);

    Entity entity = gCoordinator.CreateEntity();
    gCoordinator.AddComponent<Transform>(entity, Transform{0.f, 0.f});
    gCoordinator.AddComponent<Velocity>(entity, Velocity{10.f, 0.f});

    movementSystem->Update(0.016f);
}

} // namespace ecs
