/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** simple UDP server using UdpServer
*/

#include "../network/includes/udpServer.hpp"
#include "../ecs/core/coordinator.hpp"
#include "../ecs/game/systems.hpp"
#include "../network/includes/protocol.hpp"
#include "../network/includes/thread_utils.hpp"
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <random>
#include <memory>
#include <unordered_set>

using namespace ecs;

extern std::shared_ptr<MovementSystem> gMovementSystem;
extern std::shared_ptr<InputSystem>    gInputSystem;
extern std::shared_ptr<BoundarySystem> gBoundarySystem;
extern std::shared_ptr<SpawnerSystem>  gSpawnerSystem;
extern std::shared_ptr<CollisionSystem> gCollisionSystem;
extern std::shared_ptr<HealthSystem>   gHealthSystem;
extern std::shared_ptr<LifetimeSystem> gLifetimeSystem;

extern std::shared_ptr<AISystem> gAISystem;

std::unordered_map<std::string, ecs::Entity> gClientToEntity;

static std::unordered_map<ecs::Entity, std::chrono::steady_clock::time_point> gPlayerStartTimes;
static std::unordered_map<ecs::Entity, int> gPlayerScores;

static std::vector<ecs::Entity> gEnemies;

static std::vector<ecs::Entity> gProjectiles;

// Build a binary STATE packet representing the current world snapshot.  The
// payload layout is as follows:
//   uint32_t msgId         // unique message identifier for reliability
//   uint32_t tick          // simulation tick count
//   uint16_t numPlayers    // number of player entities
//   uint16_t numEnemies    // number of enemy entities
//   uint16_t numProjectiles// number of projectile entities
//   for each player:
//       uint32_t id
//       uint8_t  type=0    // 0 for player
//       float    x, y
//       uint32_t hp
//   for each enemy:
//       uint32_t id
//       uint8_t  type=1    // 1 for enemy
//       float    x, y
//       uint32_t hp
//   for each projectile:
//       uint32_t id
//       uint8_t  type=2    // 2 for projectile
//       float    x, y
// All numeric values are encoded in network byte order.  This function
// assembles the payload and prefixes it with a protocol header.
static std::string BuildStatePacket(uint32_t msgId, uint64_t tick)
{
    using protocol::encodeHeader;
    using protocol::Opcode;
    std::vector<uint8_t> payload;
    // Reserve an initial guess to reduce reallocations
    payload.reserve(32 + (gClientToEntity.size() + gEnemies.size() + gProjectiles.size()) * 24);
    // Append msgId and tick (both 32 bits for simplicity)
    uint32_t msgIdNet = htonl(msgId);
    uint32_t tickLow = htonl(static_cast<uint32_t>(tick & 0xFFFFFFFFu));
    payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&msgIdNet), reinterpret_cast<uint8_t *>(&msgIdNet) + sizeof(uint32_t));
    payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&tickLow), reinterpret_cast<uint8_t *>(&tickLow) + sizeof(uint32_t));
    // Append counts
    uint16_t nPlayers = htons(static_cast<uint16_t>(gClientToEntity.size()));
    uint16_t nEnemies = htons(static_cast<uint16_t>(gEnemies.size()));
    uint16_t nProjs   = htons(static_cast<uint16_t>(gProjectiles.size()));
    payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&nPlayers), reinterpret_cast<uint8_t *>(&nPlayers) + sizeof(uint16_t));
    payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&nEnemies), reinterpret_cast<uint8_t *>(&nEnemies) + sizeof(uint16_t));
    payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&nProjs),   reinterpret_cast<uint8_t *>(&nProjs)   + sizeof(uint16_t));
    // Players
    for (const auto &kv : gClientToEntity) {
        ecs::Entity id = kv.second;
        if (!gCoordinator.GetSignature(id).any()) continue;
        uint32_t idNet = htonl(static_cast<uint32_t>(id));
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&idNet), reinterpret_cast<uint8_t *>(&idNet) + sizeof(uint32_t));
        uint8_t type = 0;
        payload.push_back(type);
        const auto &t = gCoordinator.GetComponent<ecs::Transform>(id);
        uint32_t xBits; std::memcpy(&xBits, &t.x, sizeof(float)); xBits = htonl(xBits);
        uint32_t yBits; std::memcpy(&yBits, &t.y, sizeof(float)); yBits = htonl(yBits);
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&xBits), reinterpret_cast<uint8_t *>(&xBits) + sizeof(uint32_t));
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&yBits), reinterpret_cast<uint8_t *>(&yBits) + sizeof(uint32_t));
        const auto &h = gCoordinator.GetComponent<ecs::Health>(id);
        uint32_t hpNet = htonl(static_cast<uint32_t>(h.current));
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&hpNet), reinterpret_cast<uint8_t *>(&hpNet) + sizeof(uint32_t));
    }
    // Enemies
    for (auto id : gEnemies) {
        if (!gCoordinator.GetSignature(id).any()) continue;
        uint32_t idNet = htonl(static_cast<uint32_t>(id));
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&idNet), reinterpret_cast<uint8_t *>(&idNet) + sizeof(uint32_t));
        uint8_t type = 1;
        payload.push_back(type);
        const auto &t = gCoordinator.GetComponent<ecs::Transform>(id);
        uint32_t xBits; std::memcpy(&xBits, &t.x, sizeof(float)); xBits = htonl(xBits);
        uint32_t yBits; std::memcpy(&yBits, &t.y, sizeof(float)); yBits = htonl(yBits);
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&xBits), reinterpret_cast<uint8_t *>(&xBits) + sizeof(uint32_t));
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&yBits), reinterpret_cast<uint8_t *>(&yBits) + sizeof(uint32_t));
        const auto &h = gCoordinator.GetComponent<ecs::Health>(id);
        uint32_t hpNet = htonl(static_cast<uint32_t>(h.current));
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&hpNet), reinterpret_cast<uint8_t *>(&hpNet) + sizeof(uint32_t));
    }
    // Projectiles
    for (auto id : gProjectiles) {
        if (!gCoordinator.GetSignature(id).any()) continue;
        uint32_t idNet = htonl(static_cast<uint32_t>(id));
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&idNet), reinterpret_cast<uint8_t *>(&idNet) + sizeof(uint32_t));
        uint8_t type = 2;
        payload.push_back(type);
        const auto &t = gCoordinator.GetComponent<ecs::Transform>(id);
        uint32_t xBits; std::memcpy(&xBits, &t.x, sizeof(float)); xBits = htonl(xBits);
        uint32_t yBits; std::memcpy(&yBits, &t.y, sizeof(float)); yBits = htonl(yBits);
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&xBits), reinterpret_cast<uint8_t *>(&xBits) + sizeof(uint32_t));
        payload.insert(payload.end(), reinterpret_cast<uint8_t *>(&yBits), reinterpret_cast<uint8_t *>(&yBits) + sizeof(uint32_t));
    }
    // Build final packet
    auto header = protocol::encodeHeader(Opcode::STATE, static_cast<uint16_t>(payload.size()));
    std::string packet(reinterpret_cast<const char *>(header.data()), header.size());
    packet.append(reinterpret_cast<const char *>(payload.data()), payload.size());
    return packet;
}

static std::unordered_map<ecs::Entity, float> gPlayerShootCooldowns;

static std::unordered_map<ecs::Entity, float> gEnemyShootCooldowns;

ecs::Entity CreateEnemyEntity()
{
    using namespace ecs;
    Entity e = gCoordinator.CreateEntity();
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distY(50.f, 550.f);
    
    Transform t{};
    t.x = 900.f;
    t.y = distY(rng);
    gCoordinator.AddComponent(e, t);
    Velocity v{};
    v.vx = -80.f;
    v.vy = 0.f;
    gCoordinator.AddComponent(e, v);
    Boundary b{};
    b.minX = -100.f;
    b.maxX = 900.f;
    b.minY = 0.f;
    b.maxY = 600.f;
    b.destroy = true;
    b.wrap = false;
    gCoordinator.AddComponent(e, b);
    Health h{};
    h.current = 1;
    h.max = 1;
    gCoordinator.AddComponent(e, h);

    Team team{};
    team.teamID = 1;
    coordinator.AddComponent(e, team);

    Collider col{};
    col.shape = Collider::Shape::Circle;
    col.radius = 20.f;
    coordinator.AddComponent(e, col);

    Damager dmg{};
    dmg.damage = 10;
    coordinator.AddComponent(e, dmg);

    AIController ai{};
    ai.currentState = AIController::State::Patrolling;
    coordinator.AddComponent(e, ai);

    gEnemyShootCooldowns[e] = 2.0f;

    return e;
}

ecs::Entity CreatePlayerEntity(Coordinator& coordinator)
{
    Entity e = coordinator.CreateEntity();

    Transform t{};
    t.x = 400.f;
    t.y = 300.f;
    coordinator.AddComponent(e, t);

    Velocity v{};
    coordinator.AddComponent(e, v);

    PlayerInput input{};
    coordinator.AddComponent(e, input);

    Boundary b{};
    b.minX = 0.f;  b.maxX = 800.f;
    b.minY = 0.f;  b.maxY = 600.f;
    b.destroy = false;
    b.wrap = false;
    coordinator.AddComponent(e, b);

    Health h{};
    h.current = 100;
    h.max = 100;
    coordinator.AddComponent(e, h);

    Team team{};
    team.teamID = 0;
    coordinator.AddComponent(e, team);

    PlayerTag tag{};
    tag.clientId = static_cast<int>(e);
    coordinator.AddComponent(e, tag);

    // Record creation time and initialise score.  These maps are used
    // when computing the scoreboard at the end of the game.
    gPlayerStartTimes[e] = std::chrono::steady_clock::now();
    gPlayerScores[e] = 0;

    Collider col{};
    col.shape = Collider::Shape::Circle;
    col.radius = 18.f;
    coordinator.AddComponent(e, col);

    gPlayerShootCooldowns[e] = 0.f;

    return e;
}

static void SpawnProjectileFrom(ecs::Entity shooter, bool fromPlayer)
{
    using namespace ecs;
    const auto &shooterTransform = gCoordinator.GetComponent<Transform>(shooter);
    const auto &shooterTeam = gCoordinator.GetComponent<Team>(shooter);

    Entity proj = coordinator.CreateEntity();

    Transform t{};
    t.x = shooterTransform.x + (fromPlayer ? 25.f : -25.f);
    t.y = shooterTransform.y;
    coordinator.AddComponent(proj, t);

    Velocity v{};
    v.vx = fromPlayer ? 400.f : -200.f;
    v.vy = 0.f;
    coordinator.AddComponent(proj, v);

    Collider c{};
    c.shape = Collider::Shape::Circle;
    c.radius = 5.f;
    c.isTrigger = false;
    coordinator.AddComponent(proj, c);

    Damager d{};
    d.damage = fromPlayer ? 10 : 15;
    coordinator.AddComponent(proj, d);

    Team team{};
    team.teamID = shooterTeam.teamID;
    coordinator.AddComponent(proj, team);

    Lifetime lifetime{};
    lifetime.timeLeft = 3.0f;
    coordinator.AddComponent(proj, lifetime);

    Boundary boundary{};
    boundary.minX = -100.f;
    boundary.maxX = 900.f;
    boundary.minY = -100.f;
    boundary.maxY = 700.f;
    boundary.destroy = true;
    boundary.wrap = false;
    coordinator.AddComponent(proj, boundary);

    gProjectiles.push_back(proj);
}

static void RunServerLoop(UdpServer& server);

static std::atomic_bool g_running{true};

static void signal_handler(int)
{
    g_running = false;
}

std::string HandleMessageFromClient(
    Coordinator& coordinator,
    const std::string& msg,
    const std::string& clientKey,
    int& nextMsgId,
    std::vector<std::pair<int, std::string>>& pendingMessages)
{
    size_t pos = msg.find(' ');
    std::string type = (pos == std::string::npos) ? msg : msg.substr(0, pos);

    if (type == "HELLO") {
        if (gClientToEntity.find(clientKey) == gClientToEntity.end()) {
            ecs::Entity e = CreatePlayerEntity(coordinator);
            gClientToEntity[clientKey] = e;
            return "WELCOME " + std::to_string(e) + "\r\n";
        }
        ecs::Entity e = gClientToEntity[clientKey];
        return "WELCOME " + std::to_string(e) + "\r\n";
    }

    if (type == "INPUT") {
        if (gClientToEntity.find(clientKey) == gClientToEntity.end())
            return "";

        ecs::Entity e = gClientToEntity[clientKey];

        int direction = 5;
        int fire = 0;

        if (pos != std::string::npos) {
            std::string rest = msg.substr(pos + 1);
            std::stringstream ss(rest);
            ss >> direction >> fire;
        }
        auto& input = coordinator.GetComponent<ecs::PlayerInput>(e);
        input.direction = direction;
        input.firePressed = (fire != 0);

        return "";
    }

    if (type == "ACK") {
        if (pos != std::string::npos) {
            std::string idStr = msg.substr(pos + 1);
            int ackId = std::atoi(idStr.c_str());
            pendingMessages.erase(
                std::remove_if(
                    pendingMessages.begin(),
                    pendingMessages.end(),
                    [ackId](const std::pair<int, std::string>& p) {
                        return p.first == ackId;
                    }),
                pendingMessages.end()
            );
            // std::cout << "[Server] ACK received for message ID " << ackId << std::endl;
        }
        return "";
    }

    if (type == "PING") {
        return "PONG\r\n";
    }

    return "ECHO: " + msg + "\r\n";
}

static void RunServerLoop(UdpServer& server)
{
    using namespace ecs;
    std::vector<std::pair<int, std::string>> pendingMessages;
    int nextMsgId = 0;
    uint64_t tick = 0;
    float enemySpawnTimer = 0.f;
    const float enemySpawnInterval = 4.f;
    auto lastTime = std::chrono::high_resolution_clock::now();
    const float fixedDt = 1.f / 60.f;
    float accumulator = 0.f;

    while (g_running) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = now - lastTime;
        lastTime = now;
        accumulator += delta.count();

        std::string raw;
        while (!(raw = server.receiveData(2048)).empty()) {
            const ClientInfo *client = server.getLastClient();
            if (!client) {
                continue;
            }
            if (raw.size() < sizeof(protocol::PacketHeader)) {
                continue;
            }
            protocol::PacketHeader hdr;
            if (!protocol::decodeHeader(reinterpret_cast<const uint8_t*>(raw.data()), raw.size(), hdr)) {
                continue;
            }
            if (hdr.version != protocol::PROTOCOL_VERSION) {
                continue;
            }
            if (hdr.length + sizeof(protocol::PacketHeader) > raw.size()) {
                continue;
            }
            const uint8_t *payload = reinterpret_cast<const uint8_t*>(raw.data()) + sizeof(protocol::PacketHeader);
            size_t payloadLen = hdr.length;
            switch (static_cast<protocol::Opcode>(hdr.opcode)) {
                case protocol::Opcode::HELLO: {
                    // Assign or retrieve existing player entity and respond with WELCOME
                    std::string clientKey = client->getKey();
                    if (gClientToEntity.find(clientKey) == gClientToEntity.end()) {
                        ecs::Entity e = CreatePlayerEntity();
                        gClientToEntity[clientKey] = e;
                    }
                    ecs::Entity e = gClientToEntity[clientKey];
                    std::string welcome = protocol::encodeWelcome(static_cast<uint32_t>(e));
                    server.sendData(welcome, *client);
                    break;
                }
                case protocol::Opcode::INPUT: {
                    uint8_t dir = 5;
                    bool fire = false;
                    if (protocol::decodeInput(payload, payloadLen, dir, fire)) {
                        std::string clientKey = client->getKey();
                        auto it = gClientToEntity.find(clientKey);
                        if (it != gClientToEntity.end()) {
                            ecs::Entity ent = it->second;
                            auto &input = gCoordinator.GetComponent<ecs::PlayerInput>(ent);
                            input.direction = dir;
                            input.firePressed = fire;
                        }
                    }
                    break;
                }
                case protocol::Opcode::ACK: {
                    uint32_t ackId;
                    if (protocol::decodeAck(payload, payloadLen, ackId)) {
                        pendingMessages.erase(
                            std::remove_if(pendingMessages.begin(), pendingMessages.end(),
                                           [ackId](const std::pair<int, std::string> &p) {
                                               return static_cast<uint32_t>(p.first) == ackId;
                                           }),
                            pendingMessages.end());
                    }
                    break;
                }
                case protocol::Opcode::LIST_LOBBIES:
                case protocol::Opcode::CREATE_LOBBY:
                case protocol::Opcode::JOIN_LOBBY:
                case protocol::Opcode::START_GAME:
                case protocol::Opcode::LOBBY_UPDATE:
                    // Lobby handling is not yet implemented in the server
                    break;
                default:
                    // Unknown or unsupported opcode
                    break;
            }
        }

        // Nettoyer les clients inactifs
        auto activeClients = server.getActiveClients(10);
        std::unordered_set<std::string> activeKeys;
        for (const auto &ci : activeClients) {
            activeKeys.insert(ci.getKey());
        }
        for (auto it = gClientToEntity.begin(); it != gClientToEntity.end();) {
            if (activeKeys.find(it->first) == activeKeys.end()) {
                Entity ent = it->second;
                coordinator.RequestDestroyEntity(ent);
                gPlayerShootCooldowns.erase(ent);
                gEnemyShootCooldowns.erase(ent);
                it = gClientToEntity.erase(it);
            } else {
                ++it;
            }
        }

        // Fixed timestep simulation
        while (accumulator >= fixedDt) {
            // Update cooldowns
            for (auto &kv : gPlayerShootCooldowns) kv.second -= fixedDt;
            for (auto &kv : gEnemyShootCooldowns) kv.second -= fixedDt;

            // IMPORTANT: Passer le coordinator à TOUS les systèmes
            systems.input->Update(coordinator);
            if (systems.ai) systems.ai->Update(coordinator, fixedDt);
            systems.movement->Update(coordinator, fixedDt);
            systems.boundary->Update(coordinator);
            systems.spawner->Update(coordinator, fixedDt);
            systems.collision->Update(coordinator);
            systems.health->Update(coordinator, fixedDt);
            systems.lifetime->Update(coordinator, fixedDt);

            // Player shooting
            for (const auto &kv : gClientToEntity) {
                Entity player = kv.second;
                Signature sig = coordinator.GetSignature(player);
                if (!sig.any()) continue;
                float &cd = gPlayerShootCooldowns[player];
                auto &inp = coordinator.GetComponent<PlayerInput>(player);
                if (inp.firePressed && cd <= 0.f) {
                    SpawnProjectileFrom(coordinator, player, true);
                    cd = 0.3f;
                }
            }

            // Enemy shooting
            for (auto enemy : gEnemies) {
                Signature sig = coordinator.GetSignature(enemy);
                if (!sig.any()) continue;
                float &cd = gEnemyShootCooldowns[enemy];
                if (cd <= 0.f) {
                    SpawnProjectileFrom(coordinator, enemy, false);
                    cd = 2.0f;
                }
            }

            // Enemy spawning
            enemySpawnTimer += fixedDt;
            if (enemySpawnTimer >= enemySpawnInterval) {
                Entity en = CreateEnemyEntity(coordinator);
                gEnemies.push_back(en);
                enemySpawnTimer = 0.f;
            }

            gCoordinator.ProcessDestructions();
            gEnemies.erase(std::remove_if(gEnemies.begin(), gEnemies.end(), [&](Entity e){
                bool alive = coordinator.GetSignature(e).any();
                if (!alive) {
                    gEnemyShootCooldowns.erase(e);
                }
                return !alive;
            }), gEnemies.end());
            gProjectiles.erase(std::remove_if(gProjectiles.begin(), gProjectiles.end(), [&](Entity e){ return !gCoordinator.GetSignature(e).any(); }), gProjectiles.end());
            for (auto it = gClientToEntity.begin(); it != gClientToEntity.end(); ) {
                Entity ent = it->second;
                if (!coordinator.GetSignature(ent).any()) {
                    gPlayerShootCooldowns.erase(ent);
                    gEnemyShootCooldowns.erase(ent);
                    it = gClientToEntity.erase(it);
                } else {
                    ++it;
                }
            }

            tick++;
            accumulator -= fixedDt;
        }

        for (const auto &ci : activeClients) {
            uint32_t msgId = static_cast<uint32_t>(++nextMsgId);
            std::string packet = BuildStatePacket(msgId, tick);
            server.sendData(packet, ci);
            pendingMessages.emplace_back(static_cast<int>(msgId), std::string{});
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::vector<ClientInfo> remaining = server.getActiveClients(10);
    if (!remaining.empty()) {
        std::vector<protocol::ScoreEntry> entries;
        auto nowScore = std::chrono::steady_clock::now();
        for (const auto &kv : gClientToEntity) {
            ecs::Entity ent = kv.second;
            auto itStart = gPlayerStartTimes.find(ent);
            float timeSurv = 0.f;
            if (itStart != gPlayerStartTimes.end()) {
                auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(nowScore - itStart->second).count();
                timeSurv = static_cast<float>(dur) / 1000.f;
            }
            uint32_t score = 0;
            auto itScore = gPlayerScores.find(ent);
            if (itScore != gPlayerScores.end()) {
                score = static_cast<uint32_t>(itScore->second);
            }
            entries.push_back({ static_cast<uint32_t>(ent), score, timeSurv });
        }
        std::string scoreboardPacket = protocol::encodeScoreboard(entries);
        for (const auto &ci : remaining) {
            server.sendData(scoreboardPacket, ci);
        }
    }
}

int main()
{
    std::signal(SIGINT, signal_handler);
    int port = 4242;
    UdpServer server(port, true);
    if (!server.initSocket()) {
        std::cerr << "Failed to init UDP server on port " << port << std::endl;
        return 84;
    }

    // Créer une instance LOCALE du coordinator (pas de global!)
    Coordinator coordinator;
    GameSystems systems;
    
    // Initialiser l'ECS
    InitEcs(coordinator, systems);

    RunServerLoop(server);

    server.disconnect();
    return 0;
}