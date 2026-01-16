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

struct GameSystems {
    std::shared_ptr<MovementSystem> movement;
    std::shared_ptr<InputSystem> input;
    std::shared_ptr<BoundarySystem> boundary;
    std::shared_ptr<HealthSystem> health;
    std::shared_ptr<LifetimeSystem> lifetime;
    std::shared_ptr<SpawnerSystem> spawner;
    std::shared_ptr<CollisionSystem> collision;
    std::shared_ptr<AISystem> ai;
};

void InitEcs(Coordinator& coordinator, GameSystems& systems)
{
    coordinator.Init();

    coordinator.RegisterComponent<Transform>();
    coordinator.RegisterComponent<Velocity>();
    coordinator.RegisterComponent<PlayerInput>();
    coordinator.RegisterComponent<Boundary>();
    coordinator.RegisterComponent<Health>();
    coordinator.RegisterComponent<Team>();
    coordinator.RegisterComponent<Damager>();
    coordinator.RegisterComponent<AIController>();
    coordinator.RegisterComponent<Lifetime>();
    coordinator.RegisterComponent<Collider>();
    coordinator.RegisterComponent<Spawner>();
    coordinator.RegisterComponent<PlayerTag>();

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

    systems.collision = coordinator.RegisterSystem<CollisionSystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<Transform>());
    sig.set(coordinator.GetComponentType<Collider>());
    sig.set(coordinator.GetComponentType<Team>());
    sig.set(coordinator.GetComponentType<Damager>());
    sig.set(coordinator.GetComponentType<Health>());
    coordinator.SetSystemSignature<CollisionSystem>(sig);

    systems.ai = coordinator.RegisterSystem<AISystem>();
    sig.reset();
    sig.set(coordinator.GetComponentType<Transform>());
    sig.set(coordinator.GetComponentType<Velocity>());
    sig.set(coordinator.GetComponentType<AIController>());
    sig.set(coordinator.GetComponentType<Health>());
    sig.set(coordinator.GetComponentType<Team>());
    coordinator.SetSystemSignature<AISystem>(sig);
}