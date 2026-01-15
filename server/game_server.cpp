#include "../network/includes/udpServer.hpp"
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

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

    gMovementSystem = gCoordinator.RegisterSystem<MovementSystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<Transform>());
    sig.set(gCoordinator.GetComponentType<Velocity>());
    gCoordinator.SetSystemSignature<MovementSystem>(sig);

    gInputSystem = gCoordinator.RegisterSystem<InputSystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<PlayerInput>());
    sig.set(gCoordinator.GetComponentType<Velocity>());
    gCoordinator.SetSystemSignature<InputSystem>(sig);

    gBoundarySystem = gCoordinator.RegisterSystem<BoundarySystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<Transform>());
    sig.set(gCoordinator.GetComponentType<Boundary>());
    gCoordinator.SetSystemSignature<BoundarySystem>(sig);

    gHealthSystem = gCoordinator.RegisterSystem<HealthSystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<Health>());
    gCoordinator.SetSystemSignature<HealthSystem>(sig);

    gLifetimeSystem = gCoordinator.RegisterSystem<LifetimeSystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<Lifetime>());
    gCoordinator.SetSystemSignature<LifetimeSystem>(sig);

    gSpawnerSystem = gCoordinator.RegisterSystem<SpawnerSystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<Spawner>());
    sig.set(gCoordinator.GetComponentType<Transform>());
    gCoordinator.SetSystemSignature<SpawnerSystem>(sig);

   gCollisionSystem = gCoordinator.RegisterSystem<CollisionSystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<Transform>());
    sig.set(gCoordinator.GetComponentType<Collider>());
    sig.set(gCoordinator.GetComponentType<Team>());
    gCoordinator.SetSystemSignature<CollisionSystem>(sig);

    gAISystem = gCoordinator.RegisterSystem<AISystem>();
    sig.reset();
    sig.set(gCoordinator.GetComponentType<Transform>());
    sig.set(gCoordinator.GetComponentType<Velocity>());
    sig.set(gCoordinator.GetComponentType<AIController>());
    sig.set(gCoordinator.GetComponentType<Health>());
    sig.set(gCoordinator.GetComponentType<Team>());
    gCoordinator.SetSystemSignature<AISystem>(sig);
}
