/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** simple UDP server using UdpServer
*/

#include "../network/includes/udpServer.hpp"
#include "../ecs/core/coordinator.hpp"
#include "../ecs/game/systems.hpp"
#include "../ecs/game/components.hpp"
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <random>
#include <memory>
#include <unordered_set>

using namespace ecs;

// Structure pour regrouper tous les systèmes (définie dans game_server.cpp)
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

// Fonction définie dans game_server.cpp
void InitEcs(Coordinator& coordinator, GameSystems& systems);

// Mapping client -> entity
std::unordered_map<std::string, ecs::Entity> gClientToEntity;

// Listes d'entités
static std::vector<ecs::Entity> gEnemies;
static std::vector<ecs::Entity> gProjectiles;

// Cooldowns de tir
static std::unordered_map<ecs::Entity, float> gPlayerShootCooldowns;
static std::unordered_map<ecs::Entity, float> gEnemyShootCooldowns;

// Créer un ennemi
ecs::Entity CreateEnemyEntity(Coordinator& coordinator)
{
    Entity e = coordinator.CreateEntity();
    
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distY(50.f, 550.f);
    
    Transform t{};
    t.x = 900.f;
    t.y = distY(rng);
    coordinator.AddComponent(e, t);
    
    Velocity v{};
    v.vx = -80.f;
    v.vy = 0.f;
    coordinator.AddComponent(e, v);
    
    Boundary b{};
    b.minX = -100.f;
    b.maxX = 900.f;
    b.minY = 0.f;
    b.maxY = 600.f;
    b.destroy = true;
    b.wrap = false;
    coordinator.AddComponent(e, b);
    
    Health h{};
    h.current = 1;
    h.max = 1;
    coordinator.AddComponent(e, h);
    
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

    Collider col{};
    col.shape = Collider::Shape::Circle;
    col.radius = 18.f;
    coordinator.AddComponent(e, col);

    gPlayerShootCooldowns[e] = 0.f;

    return e;
}

// Spawn un projectile
static void SpawnProjectileFrom(Coordinator& coordinator, ecs::Entity shooter, bool fromPlayer)
{
    const auto &shooterTransform = coordinator.GetComponent<Transform>(shooter);
    const auto &shooterTeam = coordinator.GetComponent<Team>(shooter);

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

// Forward declaration de la boucle serveur
static void RunServerLoop(UdpServer& server, Coordinator& coordinator, GameSystems& systems);

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
            std::cout << "[Server] ACK received for message ID " << ackId << std::endl;
        }
        return "";
    }

    if (type == "PING") {
        return "PONG\r\n";
    }

    return "ECHO: " + msg + "\r\n";
}

// Boucle serveur principale
static void RunServerLoop(UdpServer& server, Coordinator& coordinator, GameSystems& systems)
{
    std::vector<std::pair<int, std::string>> pendingMessages;
    int nextMsgId = 0;
    uint64_t tick = 0;
    float enemySpawnTimer = 0.f;
    const float enemySpawnInterval = 2.f;
    auto lastTime = std::chrono::high_resolution_clock::now();
    const float fixedDt = 1.f / 60.f;
    float accumulator = 0.f;

    while (g_running) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = now - lastTime;
        lastTime = now;
        accumulator += delta.count();

        // Recevoir les messages
        std::string raw;
        while (!(raw = server.receiveData(1024)).empty()) {
            const ClientInfo *client = server.getLastClient();
            if (!client) {
                continue;
            }
            std::string clientKey = client->getKey();
            std::stringstream ss(raw);
            std::string line;
            while (std::getline(ss, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (line.empty()) continue;
                std::string response = HandleMessageFromClient(coordinator, line, clientKey,
                                                             nextMsgId, pendingMessages);
                if (!response.empty()) {
                    server.sendData(response, *client);
                }
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

            // Process destructions
            coordinator.ProcessDestructions();
            
            // Nettoyer les listes
            gEnemies.erase(std::remove_if(gEnemies.begin(), gEnemies.end(), [&](Entity e){
                bool alive = coordinator.GetSignature(e).any();
                if (!alive) {
                    gEnemyShootCooldowns.erase(e);
                }
                return !alive;
            }), gEnemies.end());
            
            gProjectiles.erase(std::remove_if(gProjectiles.begin(), gProjectiles.end(), [&](Entity e){ 
                return !coordinator.GetSignature(e).any(); 
            }), gProjectiles.end());
            
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

        // Broadcast snapshot
        for (const auto &ci : activeClients) {
            std::ostringstream oss;
            int msgId = ++nextMsgId;
            oss << "STATE " << msgId << " " << tick << "\n";
            
            // Players
            for (const auto &kv : gClientToEntity) {
                Entity id = kv.second;
                Signature sig = coordinator.GetSignature(id);
                if (!sig.any()) continue;
                const auto &t = coordinator.GetComponent<Transform>(id);
                const auto &h = coordinator.GetComponent<Health>(id);
                oss << id << " P " << t.x << " " << t.y << " " << h.current << "\n";
            }
            
            // Enemies
            for (auto id : gEnemies) {
                Signature sig = coordinator.GetSignature(id);
                if (!sig.any()) continue;
                const auto &t = coordinator.GetComponent<Transform>(id);
                const auto &h = coordinator.GetComponent<Health>(id);
                oss << id << " E " << t.x << " " << t.y << " " << h.current << "\n";
            }
            
            // Projectiles
            for (auto id : gProjectiles) {
                Signature sig = coordinator.GetSignature(id);
                if (!sig.any()) continue;
                const auto &t = coordinator.GetComponent<Transform>(id);
                oss << id << " B " << t.x << " " << t.y << "\n";
            }
            
            std::string payload = oss.str();
            payload += "\r\n";
            server.sendData(payload, ci);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

    std::cout << "[Server] Ready on port " << server.getPort()
              << " (press Ctrl+C to quit)" << std::endl;

    // Lancer la boucle serveur en passant le coordinator
    RunServerLoop(server, coordinator, systems);

    std::cout << "\n[Server] Shutting down..." << std::endl;
    server.disconnect();
    return 0;
}