#include "utils.hpp"

namespace ecs {

SystemRefs InitECS() {
    SystemRefs systems;
    gCoordinator.Init();

    gCoordinator.RegisterComponent<Transform>();
    gCoordinator.RegisterComponent<Velocity>();
    gCoordinator.RegisterComponent<Collider>();
    gCoordinator.RegisterComponent<Health>();
    gCoordinator.RegisterComponent<Lifetime>();
    gCoordinator.RegisterComponent<Score>();
    gCoordinator.RegisterComponent<Boundary>();
    gCoordinator.RegisterComponent<PlayerInput>();
    gCoordinator.RegisterComponent<Team>();
    gCoordinator.RegisterComponent<Damager>();
    gCoordinator.RegisterComponent<AIController>();
    gCoordinator.RegisterComponent<Spawner>();

    // Input System
    systems.inputSystem = gCoordinator.RegisterSystem<InputSystem>();
    {
        Signature signature;
        signature.set(gCoordinator.GetComponentType<PlayerInput>());
        signature.set(gCoordinator.GetComponentType<Velocity>());
        gCoordinator.SetSystemSignature<InputSystem>(signature);
    }

    // AI System
    systems.aiSystem = gCoordinator.RegisterSystem<AISystem>();
    {
        Signature signature;
        signature.set(gCoordinator.GetComponentType<AIController>());
        signature.set(gCoordinator.GetComponentType<Transform>());
        signature.set(gCoordinator.GetComponentType<Velocity>());
        signature.set(gCoordinator.GetComponentType<Team>());
        signature.set(gCoordinator.GetComponentType<Health>());
        gCoordinator.SetSystemSignature<AISystem>(signature);
    }

    // Movement System
    systems.movementSystem = gCoordinator.RegisterSystem<MovementSystem>();
    {
        Signature signature;
        signature.set(gCoordinator.GetComponentType<Transform>());
        signature.set(gCoordinator.GetComponentType<Velocity>());
        gCoordinator.SetSystemSignature<MovementSystem>(signature);
    }

    // Collision System (détection + résolution)
    systems.collisionSystem = gCoordinator.RegisterSystem<CollisionSystem>();
    {
        Signature signature;
        signature.set(gCoordinator.GetComponentType<Transform>());
        signature.set(gCoordinator.GetComponentType<Collider>());
        signature.set(gCoordinator.GetComponentType<Team>());
        signature.set(gCoordinator.GetComponentType<Damager>());
        signature.set(gCoordinator.GetComponentType<Health>());
        gCoordinator.SetSystemSignature<CollisionSystem>(signature);
    }

    // Spawner System
    systems.spawnerSystem = gCoordinator.RegisterSystem<SpawnerSystem>();
    {
        Signature signature;
        signature.set(gCoordinator.GetComponentType<Transform>());
        signature.set(gCoordinator.GetComponentType<Spawner>());
        signature.set(gCoordinator.GetComponentType<Team>());
        gCoordinator.SetSystemSignature<SpawnerSystem>(signature);
    }

    // Health System
    systems.healthSystem = gCoordinator.RegisterSystem<HealthSystem>();
    {
        Signature signature;
        signature.set(gCoordinator.GetComponentType<Health>());
        gCoordinator.SetSystemSignature<HealthSystem>(signature);
    }

    // Lifetime System
    systems.lifetimeSystem = gCoordinator.RegisterSystem<LifetimeSystem>();
    {
        Signature signature;
        signature.set(gCoordinator.GetComponentType<Lifetime>());
        gCoordinator.SetSystemSignature<LifetimeSystem>(signature);
    }

    // Boundary System
    systems.boundarySystem = gCoordinator.RegisterSystem<BoundarySystem>();
    {
        Signature signature;
        signature.set(gCoordinator.GetComponentType<Transform>());
        signature.set(gCoordinator.GetComponentType<Boundary>());
        gCoordinator.SetSystemSignature<BoundarySystem>(signature);
    }

    return systems;
}

}