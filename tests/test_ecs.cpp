#include <iostream>
#include <stdexcept>
#include <string>

#include "ecs.hpp"

namespace {

struct Position {
    int x{};
};

struct Velocity {
    int speed{};
};

class PositionSystem : public ecs::System {};

void Require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

ecs::Signature MakePositionSignature(ecs::Coordinator& coordinator)
{
    ecs::Signature signature;
    signature.set(coordinator.GetComponentType<Position>());
    return signature;
}

void TestComponentStorageAndSystemSync()
{
    ecs::Coordinator coordinator;
    coordinator.Init();

    coordinator.RegisterComponent<Position>();
    coordinator.RegisterComponent<Velocity>();

    auto system = coordinator.RegisterSystem<PositionSystem>();
    coordinator.SetSystemSignature<PositionSystem>(MakePositionSignature(coordinator));

    ecs::Entity entity = coordinator.CreateEntity();
    coordinator.AddComponent(entity, Position{42});
    coordinator.AddComponent(entity, Velocity{9001});

    Require(system->entities.size() == 1, "Entity should match system signature after component add");
    Require(system->entities.front() == entity, "System should track the correct entity");

    auto& pos = coordinator.GetComponent<Position>(entity);
    Require(pos.x == 42, "Component data should be retrievable");

    coordinator.RemoveComponent<Position>(entity);
    Require(system->entities.empty(), "Entity should be removed once it no longer matches the signature");
}

void TestImmediateDestructionRemovesComponents()
{
    ecs::Coordinator coordinator;
    coordinator.Init();

    coordinator.RegisterComponent<Position>();
    auto system = coordinator.RegisterSystem<PositionSystem>();
    coordinator.SetSystemSignature<PositionSystem>(MakePositionSignature(coordinator));

    ecs::Entity entity = coordinator.CreateEntity();
    coordinator.AddComponent(entity, Position{21});
    Require(system->entities.size() == 1, "Entity should be tracked before destruction");

    coordinator.DestroyEntity(entity);
    Require(system->entities.empty(), "DestroyEntity should remove entity from matching systems");

    ecs::Entity replacement = coordinator.CreateEntity();
    coordinator.AddComponent(replacement, Position{84});
    Require(system->entities.size() == 1, "System should pick up entities created after a destruction");
    Require(coordinator.GetComponent<Position>(replacement).x == 84, "New entity should accept components after destruction");
}

void TestDeferredDestructionCleansUp()
{
    ecs::Coordinator coordinator;
    coordinator.Init();

    coordinator.RegisterComponent<Position>();
    auto system = coordinator.RegisterSystem<PositionSystem>();
    coordinator.SetSystemSignature<PositionSystem>(MakePositionSignature(coordinator));

    ecs::Entity survivor = coordinator.CreateEntity();
    ecs::Entity doomed = coordinator.CreateEntity();

    coordinator.AddComponent(survivor, Position{7});
    coordinator.AddComponent(doomed, Position{13});

    Require(system->entities.size() == 2, "Both entities should be tracked prior to destruction");

    coordinator.RequestDestroyEntity(doomed);
    Require(system->entities.size() == 2, "Deferred destruction should not remove entities immediately");

    coordinator.ProcessDestructions();
    Require(system->entities.size() == 1, "Processing should remove destroyed entity from systems");
    Require(system->entities.front() == survivor, "Surviving entity should still be tracked");

    auto& remaining = coordinator.GetComponent<Position>(survivor);
    Require(remaining.x == 7, "Components on surviving entities should be intact");

    ecs::Entity replacement = coordinator.CreateEntity();
    coordinator.AddComponent(replacement, Position{99});
    Require(system->entities.size() == 2, "System should keep working after processing destructions");
}

} // namespace

int main()
{
    int failed = 0;

    const auto run = [&](const char* name, auto&& fn) {
        try {
            fn();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& e) {
            std::cerr << "[FAIL] " << name << ": " << e.what() << '\n';
            ++failed;
        }
    };

    run("Component storage and system sync", TestComponentStorageAndSystemSync);
    run("Immediate destruction cleanup", TestImmediateDestructionRemovesComponents);
    run("Deferred destruction cleanup", TestDeferredDestructionCleansUp);

    if (failed == 0) {
        std::cout << "All ECS tests passed\n";
        return 0;
    }

    std::cerr << failed << " test(s) failed\n";
    return 1;
}
