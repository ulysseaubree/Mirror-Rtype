# Changelog

## Defense 1 Prototype (2025‑12‑15)

- **Refactor**: restructured the project to clarify separation between client, server, ECS and network layers.
- **CMake**: ensured a working build on Linux, generating two binaries `r-type_server` and `r-type_client`.
- **Dependency management**: kept SFML discovery via `find_package` and used header‑only ASIO for networking; no external package manager required for this MVP.
- **ECS enhancements**: added enemy spawning in the server, using the existing ECS to create entities with `Transform`, `Velocity`, `Boundary`, `Health` and `Team` components.
- **Networking protocol**: documented the UDP text protocol in `docs/protocol.md`.
- **Server**: added multithreading‐like behaviour by separating the fixed simulation step from network reception; implemented periodic enemy spawn and world state replication.
- **Client**: replaced the CLI with a graphical SFML prototype featuring a scrolling starfield and a player ship controlled via keyboard; integrated UDP communication (HELLO/INPUT/STATE/ACK) and fallback to offline mode if no server is available.
- **Documentation**: updated `README.md` and `client/README.md` to reflect the new architecture and usage.
- **Tests**: added a minimal test for `UdpClient` under `tests/`.

### Known limitations / TODO (Defense 2)

- The enemy behaviour is simplistic (no shooting, no collision handling on client side).
- No sound effects or sprites are loaded; the player and enemies are simple shapes.
- Multithreading in the server is rudimentary; a real implementation should offload network I/O to a separate thread and use queues to communicate with the game loop.
- The ECS could expose a method to query component existence (e.g., `HasComponent`) to cleanly filter destroyed entities.
- Serialization is naive; a binary protocol would be more efficient for larger games.