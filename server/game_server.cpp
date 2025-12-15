#include "../network/includes/udpServer.hpp"

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

using namespace ecs;

//
// Note: use the global coordinator provided in ecs/ecs.hpp (gCoordinator) rather than
// creating a separate instance.  Having multiple coordinators would result in
// systems being registered on one coordinator and entities created on another,
// causing updates to never run.  See server/main.cpp for details.
//

// Systèmes globalement accessibles. Ils seront initialisés dans InitEcs().
std::shared_ptr<MovementSystem> gMovementSystem;
std::shared_ptr<InputSystem>    gInputSystem;
std::shared_ptr<BoundarySystem> gBoundarySystem;
std::shared_ptr<HealthSystem>   gHealthSystem;
std::shared_ptr<LifetimeSystem> gLifetimeSystem;
std::shared_ptr<SpawnerSystem>  gSpawnerSystem;
std::shared_ptr<CollisionSystem> gCollisionSystem;

void InitEcs()
{
    // Initialise the global coordinator. This sets up entity and component managers.
    gCoordinator.Init();

    // === Register Components ===
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

    // === Register Systems + signatures ===
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
    sig.set(gCoordinator.GetComponentType<Damager>());
    sig.set(gCoordinator.GetComponentType<Health>());
    gCoordinator.SetSystemSignature<CollisionSystem>(sig);
}
