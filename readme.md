## R‑Type Defense 1 Prototype

This repository contains a minimal yet functional R‑Type prototype designed for the first defense.  
It demonstrates a clear separation between the **engine** (ECS, systems), **networking** (UDP authoritative server) and **rendering** (SFML). The goal is to provide a playable game loop over the network with clean architecture and documentation.

### Binaries

The project builds two executables via CMake :

- `r-type_server` : authoritative UDP server handling physics, collisions, spawns and world state replication.  
- `r-type_client` : graphical client using SFML. It displays a scrolling starfield, allows the player to move and shoot, and communicates with the server over UDP.

### Build instructions (Linux)

1. Install the dependencies (SFML ≥ 2.5, a C++17‑capable compiler, CMake ≥ 3.16).  
2. Clone this repository and create a build directory:

   ```bash
   mkdir build && cd build
   cmake ..
   make -j
   ```

The binaries will be placed in `build/bin/`.

### Running

Start the server in one terminal:

```bash
./bin/r-type_server
```

Then start one or more clients in other terminals:

```bash
./bin/r-type_client    # optionally add IP and port arguments
```

Use the arrow keys to move your ship and spacebar to shoot. The server will replicate the position of your ship and spawn simple enemies that scroll from right to left.

### Project Structure

- `client/` : client application sources and headers. Contains a README with usage details.
- `server/` : server logic (game loop, messaging, spawning). The main loop illustrates a clean separation between network I/O and game simulation.
- `ecs/` : header‑only ECS framework with components and systems used by both client and server.
- `network/` : UDP client/server wrappers and a simple thread‑safe queue.
- `docs/` : additional documentation, including the description of the network protocol.
- `tests/` : minimal tests validating parts of the networking layer.

The original ECS skeleton documentation follows below.

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
