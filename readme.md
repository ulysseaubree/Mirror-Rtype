## ECS Skeleton Overview

This project contains a compact, header-only Entity Component System (ECS) skeleton designed for a 2D networked shoot'em up (R-Type-like) developed in C++20 using only the standard library. The ECS layer is fully decoupled from rendering, networking, threading, and OS specifics so it can be shared between clients and servers.

### Files

- `ecs/ecs.hpp` – complete ECS implementation (entity, component, system managers, coordinator, and global instance).
- `ecs/example.hpp` – minimal example showing how to define components, systems, and run a simple update.

### Core Ideas

- **Entities** are lightweight IDs (`ecs::Entity`) managed by `ecs::EntityManager`, which reuses freed IDs and stores signatures describing which components each entity owns.
- **Components** are plain structs stored densely per type inside `ecs::ComponentArray<T>`. `ecs::ComponentManager` registers component types, stores them, and exposes `Add/Remove/Get`.
- **Systems** derive from `ecs::System`, hold a `Signature`, and the set of entities matching that signature. `ecs::SystemManager` registers systems and keeps entity membership in sync whenever signatures change.
- **Coordinator** (`ecs::Coordinator`) is the façade that client code uses to create/destroy entities, register components/systems, and add/remove component instances. A single global instance `ecs::gCoordinator` is provided for convenience in header-only form.

### Basic Usage

1. `#include "ecs/ecs.hpp"` (and `ecs/example.hpp` if you want the sample).
2. Call `ecs::gCoordinator.Init();` once during startup.
3. Register every component type you plan to use via `RegisterComponent<T>()`.
4. Register each system (e.g., `auto move = gCoordinator.RegisterSystem<MovementSystem>();`), then build a signature describing which components the system needs and call `SetSystemSignature<System>(signature);`.
5. Create entities with `CreateEntity()` and attach components using `AddComponent(entity, Component{...});`.
6. Run your systems by iterating over `system->mEntities` and accessing components through `gCoordinator.GetComponent<T>(entity);`. The example movement system demonstrates this pattern.
7. Destroy entities through `DestroyEntity(entity);` when they are no longer needed; this automatically cleans up components and removes the entity from every system.

See `ecs/example.hpp` for a concise demonstration that registers a `Transform` and `Velocity` component, creates a movement system, spawns an entity, and performs one update step.

Because the ECS is header-only, simply drop the `ecs` folder into your project, enable C++20, and include the headers wherever needed. Add your own components and systems to extend the behavior for your game logic.
