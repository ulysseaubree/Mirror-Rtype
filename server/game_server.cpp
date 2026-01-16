#include "../network/includes/udpServer.hpp"
#include "../ecs/core/coordinator.hpp"
#include "../ecs/game/systems.hpp"
#include "../ecs/game/components.hpp"
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <memory>

using namespace ecs;

std::shared_ptr<MovementSystem> gMovementSystem;
std::shared_ptr<InputSystem>    gInputSystem;
std::shared_ptr<BoundarySystem> gBoundarySystem;
std::shared_ptr<HealthSystem>   gHealthSystem;
std::shared_ptr<LifetimeSystem> gLifetimeSystem;
std::shared_ptr<SpawnerSystem>  gSpawnerSystem;
std::shared_ptr<CollisionSystem> gCollisionSystem;

std::shared_ptr<AISystem> gAISystem;

void InitEcs()
{
    gCoordinator.Init();

    gCoordinator.RegisterComponent<Transform>();
    gCoordinator.RegisterComponent<Velocity>();
    gCoordinator.RegisterComponent<PlayerInput>();
    gCoordinator.RegisterComponent<Boundary>();
    gCoordinator.RegisterComponent<Health>();
    gCoordinator.RegisterComponent<Team>();
    gCoordinator.RegisterComponent<Damager>();
    gCoordinator.RegisterComponent<AIController>();
    gCoordinator.RegisterComponent<Lifetime>();
    gCoordinator.RegisterComponent<Collider>();
    gCoordinator.RegisterComponent<Spawner>();
    gCoordinator.RegisterComponent<PlayerTag>();

    Signature sig;

    systems.movement = coordinator.RegisterSystem<MovementSystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<Transform>());
    sig.set(coordinator.GetComponentType<Velocity>());
    coordinator.SetSystemSignature<MovementSystem>(sig);

    systems.input = coordinator.RegisterSystem<InputSystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<PlayerInput>());
    sig.set(coordinator.GetComponentType<Velocity>());
    coordinator.SetSystemSignature<InputSystem>(sig);

    systems.boundary = coordinator.RegisterSystem<BoundarySystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<Transform>());
    sig.set(coordinator.GetComponentType<Boundary>());
    coordinator.SetSystemSignature<BoundarySystem>(sig);

    systems.health = coordinator.RegisterSystem<HealthSystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<Health>());
    coordinator.SetSystemSignature<HealthSystem>(sig);

    systems.lifetime = coordinator.RegisterSystem<LifetimeSystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<Lifetime>());
    coordinator.SetSystemSignature<LifetimeSystem>(sig);

    systems.spawner = coordinator.RegisterSystem<SpawnerSystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<Spawner>());
    sig.set(coordinator.GetComponentType<Transform>());
    coordinator.SetSystemSignature<SpawnerSystem>(sig);

   gCollisionSystem = gCoordinator.RegisterSystem<CollisionSystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<Transform>());
    sig.set(gCoordinator.GetComponentType<Collider>());
    sig.set(gCoordinator.GetComponentType<Team>());
    gCoordinator.SetSystemSignature<CollisionSystem>(sig);

    gAISystem = gCoordinator.RegisterSystem<AISystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<Transform>());
    sig.set(coordinator.GetComponentType<Velocity>());
    sig.set(coordinator.GetComponentType<AIController>());
    sig.set(coordinator.GetComponentType<Health>());
    sig.set(coordinator.GetComponentType<Team>());
    coordinator.SetSystemSignature<AISystem>(sig);
}