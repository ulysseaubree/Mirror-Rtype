/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** simple UDP server using UdpServer
*/

#include "../network/includes/udpServer.hpp"
#include "../ecs/game/systems.hpp"
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

// Déclarations externes des systèmes définis dans game_server.cpp
// Ces pointeurs sont créés et initialisés via la fonction InitEcs().
extern std::shared_ptr<MovementSystem> gMovementSystem;
extern std::shared_ptr<InputSystem>    gInputSystem;
extern std::shared_ptr<BoundarySystem> gBoundarySystem;
extern std::shared_ptr<SpawnerSystem>  gSpawnerSystem;
extern std::shared_ptr<CollisionSystem> gCollisionSystem;
extern std::shared_ptr<HealthSystem>   gHealthSystem;
extern std::shared_ptr<LifetimeSystem> gLifetimeSystem;

// Forward declare the AI system pointer defined in game_server.cpp so that
// the main server loop can invoke enemy behaviour updates.  Without
// declaring this pointer here the symbol would be unknown in this
// translation unit.
extern std::shared_ptr<AISystem> gAISystem;

std::unordered_map<std::string, ecs::Entity> gClientToEntity;

// Liste globale des ennemis générés côté serveur. Chaque ennemi est une entité ECS
// avec un Transform, une Velocity (vers la gauche), un Boundary et une Health.
static std::vector<ecs::Entity> gEnemies;

// Global container for projectiles.  Whenever a projectile is spawned it
// is pushed into this vector so that its state can be synchronised with
// clients.  Destroyed projectiles are pruned regularly.
static std::vector<ecs::Entity> gProjectiles;

// Per-player shoot cooldown timers.  Keys are entity identifiers and
// values represent the remaining time (in seconds) before the entity
// can fire another shot.  Entries are erased when entities are
// destroyed or clients disconnect.
static std::unordered_map<ecs::Entity, float> gPlayerShootCooldowns;

// Per-enemy shoot cooldown timers.  Similar to gPlayerShootCooldowns
// but for AI-controlled enemies.  Each enemy fires when its timer
// reaches zero and is then reset to a predefined interval.
static std::unordered_map<ecs::Entity, float> gEnemyShootCooldowns;

// Crée un ennemi apparaissant à droite de l'écran et se déplaçant vers la gauche.
ecs::Entity CreateEnemyEntity()
{
    using namespace ecs;
    Entity e = gCoordinator.CreateEntity();
    // Position de spawn : x hors écran à droite, y aléatoire
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distY(50.f, 550.f);
    Transform t{};
    t.x = 900.f;
    t.y = distY(rng);
    gCoordinator.AddComponent(e, t);
    // Vitesse horizontale négative
    Velocity v{};
    v.vx = -80.f;
    v.vy = 0.f;
    gCoordinator.AddComponent(e, v);
    // Limites pour détruire lorsque l'ennemi sort de l'écran
    Boundary b{};
    b.minX = -100.f;
    b.maxX = 900.f;
    b.minY = 0.f;
    b.maxY = 600.f;
    b.destroy = true;
    b.wrap = false;
    gCoordinator.AddComponent(e, b);
    // Une petite santé
    Health h{};
    h.current = 1;
    h.max = 1;
    gCoordinator.AddComponent(e, h);
    // Équipe "ennemi"
    Team team{};
    team.teamID = 1;
    gCoordinator.AddComponent(e, team);

    // Collider for collision detection.  A medium radius roughly
    // approximates the size of the enemy sprite.
    Collider col{};
    col.shape = Collider::Shape::Circle;
    col.radius = 20.f;
    gCoordinator.AddComponent(e, col);

    // Damager so that an enemy can inflict damage when colliding with
    // another entity (e.g., ramming the player).  This value can be
    // tuned to adjust difficulty.
    Damager dmg{};
    dmg.damage = 10;
    gCoordinator.AddComponent(e, dmg);

    // AI controller enabling simple state-based behaviours such as
    // patrolling, chasing and fleeing.  Default state is patrolling.
    AIController ai{};
    ai.currentState = AIController::State::Patrolling;
    gCoordinator.AddComponent(e, ai);

    // Initialise the enemy's shoot cooldown.  Each enemy fires a
    // projectile after this timer elapses.  Higher values reduce the
    // firing rate.
    gEnemyShootCooldowns[e] = 2.0f;

    return e;
}

ecs::Entity CreatePlayerEntity()
{
    using namespace ecs;
    Entity e = gCoordinator.CreateEntity();

    Transform t{};
    t.x = 400.f;
    t.y = 300.f;
    gCoordinator.AddComponent(e, t);

    Velocity v{};
    gCoordinator.AddComponent(e, v);

    PlayerInput input{};
    gCoordinator.AddComponent(e, input);

    Boundary b{};
    b.minX = 0.f;  b.maxX = 800.f;
    b.minY = 0.f;  b.maxY = 600.f;
    b.destroy = false;
    b.wrap = false;
    gCoordinator.AddComponent(e, b);

    Health h{};
    h.current = 100;
    h.max = 100;
    gCoordinator.AddComponent(e, h);

    Team team{};
    team.teamID = 0; // joueurs
    gCoordinator.AddComponent(e, team);

    PlayerTag tag{};
    tag.clientId = static_cast<int>(e);
    gCoordinator.AddComponent(e, tag);

    // Collider for the player.  This allows the player to take damage
    // when hit by enemy projectiles or enemies themselves.  Radius is
    // chosen to approximate the player ship size.
    Collider col{};
    col.shape = Collider::Shape::Circle;
    col.radius = 18.f;
    gCoordinator.AddComponent(e, col);

    // Initialise the player's shoot cooldown to zero so the player can
    // fire immediately when connecting.  Cooldown will be updated every
    // tick in the game loop.
    gPlayerShootCooldowns[e] = 0.f;

    return e;
}

// -----------------------------------------------------------------------------
// Helper to spawn a projectile from an entity.
//
// This function creates a new entity representing a projectile and
// attaches the necessary components.  The projectile's direction and
// damage depend on whether it is fired by a player or by an enemy.
static void SpawnProjectileFrom(ecs::Entity shooter, bool fromPlayer)
{
    using namespace ecs;
    // Retrieve the shooter's position.  We ensure that this function
    // is only called for living entities, so GetComponent will not
    // assert.
    const auto &shooterTransform = gCoordinator.GetComponent<Transform>(shooter);
    const auto &shooterTeam = gCoordinator.GetComponent<Team>(shooter);

    Entity proj = gCoordinator.CreateEntity();

    // Place the projectile slightly in front of the shooter.
    Transform t{};
    t.x = shooterTransform.x + (fromPlayer ? 25.f : -25.f);
    t.y = shooterTransform.y;
    gCoordinator.AddComponent(proj, t);

    // Assign velocity based on the origin of the shot.
    Velocity v{};
    v.vx = fromPlayer ? 400.f : -200.f;
    v.vy = 0.f;
    gCoordinator.AddComponent(proj, v);

    // Use a small collider to detect impacts.  Bullets are not triggers
    // so they will cause damage on collision.
    Collider c{};
    c.shape = Collider::Shape::Circle;
    c.radius = 5.f;
    c.isTrigger = false;
    gCoordinator.AddComponent(proj, c);

    // Damage differs for player and enemy projectiles to balance gameplay.
    Damager d{};
    d.damage = fromPlayer ? 10 : 15;
    gCoordinator.AddComponent(proj, d);

    // Propagate the team identifier from the shooter to the projectile.
    Team team{};
    team.teamID = shooterTeam.teamID;
    gCoordinator.AddComponent(proj, team);

    // Limit projectile lifetime to prevent stray bullets lingering forever.
    Lifetime lifetime{};
    lifetime.timeLeft = 3.0f;
    gCoordinator.AddComponent(proj, lifetime);

    // Destroy the projectile when it leaves the play area.
    Boundary boundary{};
    boundary.minX = -100.f;
    boundary.maxX = 900.f;
    boundary.minY = -100.f;
    boundary.maxY = 700.f;
    boundary.destroy = true;
    boundary.wrap = false;
    gCoordinator.AddComponent(proj, boundary);

    // Keep track of all projectiles for synchronisation with clients.
    gProjectiles.push_back(proj);
}

// Forward declaration of the new authoritative game loop.  The legacy
// loop remains below wrapped in a preprocessor block for reference but
// is no longer executed.  The new loop handles multiple clients,
// fixed-timestep simulation, projectile spawning, enemy AI and
// broadcasting world snapshots to all connected clients.
static void RunServerLoop(UdpServer& server);

static std::atomic_bool g_running{true};

static void signal_handler(int)
{
    g_running = false;
}

std::string HandleMessageFromClient(
    const std::string& msg,
    const std::string& clientKey,
    int& nextMsgId,
    std::vector<std::pair<int, std::string>>& pendingMessages)
{
    size_t pos = msg.find(' ');
    std::string type = (pos == std::string::npos) ? msg : msg.substr(0, pos);

    if (type == "HELLO") {
        // Créer un joueur si nécessaire
        if (gClientToEntity.find(clientKey) == gClientToEntity.end()) {
            ecs::Entity e = CreatePlayerEntity();
            gClientToEntity[clientKey] = e;

            // Réponse immédiate WELCOME
            return "WELCOME " + std::to_string(e) + "\r\n";
        }
        // Si déjà connu, renvoyer quand même un WELCOME
        ecs::Entity e = gClientToEntity[clientKey];
        return "WELCOME " + std::to_string(e) + "\r\n";
    }

    if (type == "INPUT") {
        if (gClientToEntity.find(clientKey) == gClientToEntity.end())
            return ""; // ignore si pas encore HELLO

        ecs::Entity e = gClientToEntity[clientKey];

        int direction = 5;
        int fire = 0;

        if (pos != std::string::npos) {
            std::string rest = msg.substr(pos + 1);
            std::stringstream ss(rest);
            ss >> direction >> fire;
        }
        auto& input = gCoordinator.GetComponent<ecs::PlayerInput>(e);
        input.direction = direction;
        input.firePressed = (fire != 0);
        input.firePressed = (fire != 0);

        // Pas de réponse directe
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

    // PING / ECHO pour debug
    if (type == "PING") {
        return "PONG\r\n";
    }

    return "ECHO: " + msg + "\r\n";
}

// -----------------------------------------------------------------------------
// New server loop implementation
//
// This function encapsulates the main simulation and networking loop for the
// authoritative server.  It processes incoming messages, updates the ECS at
// a fixed timestep, spawns enemies and projectiles, prunes destroyed
// entities and broadcasts the complete game state to every active client at
// the end of each frame.
static void RunServerLoop(UdpServer& server)
{
    using namespace ecs;
    // Message storage required by HandleMessageFromClient but unused for
    // reliability in this implementation.
    std::vector<std::pair<int, std::string>> pendingMessages;
    int nextMsgId = 0;
    uint64_t tick = 0;
    float enemySpawnTimer = 0.f;
    const float enemySpawnInterval = 8.f;
    auto lastTime = std::chrono::high_resolution_clock::now();
    const float fixedDt = 1.f / 60.f;
    float accumulator = 0.f;

    while (g_running) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = now - lastTime;
        lastTime = now;
        accumulator += delta.count();

        // Drain the receive buffer and handle each message.  Messages may
        // contain multiple lines separated by newlines.
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
                std::string response = HandleMessageFromClient(line, clientKey,
                                                             nextMsgId, pendingMessages);
                if (!response.empty()) {
                    server.sendData(response, *client);
                }
            }
        }

        // Remove clients that have not sent anything for a while
        auto activeClients = server.getActiveClients(10);
        std::unordered_set<std::string> activeKeys;
        for (const auto &ci : activeClients) {
            activeKeys.insert(ci.getKey());
        }
        for (auto it = gClientToEntity.begin(); it != gClientToEntity.end();) {
            if (activeKeys.find(it->first) == activeKeys.end()) {
                Entity ent = it->second;
                gCoordinator.RequestDestroyEntity(ent);
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

            // Systems
            gInputSystem->Update();
            if (gAISystem) gAISystem->Update(fixedDt);
            gMovementSystem->Update(fixedDt);
            gBoundarySystem->Update();
            gSpawnerSystem->Update(fixedDt);
            gCollisionSystem->Update();
            gHealthSystem->Update(fixedDt);
            gLifetimeSystem->Update(fixedDt);

            // Player shooting
            for (const auto &kv : gClientToEntity) {
                Entity player = kv.second;
                Signature sig = gCoordinator.GetSignature(player);
                if (!sig.any()) continue;
                float &cd = gPlayerShootCooldowns[player];
                auto &inp = gCoordinator.GetComponent<PlayerInput>(player);
                if (inp.firePressed && cd <= 0.f) {
                    SpawnProjectileFrom(player, true);
                    cd = 0.3f;
                }
            }

            // Enemy shooting
            for (auto enemy : gEnemies) {
                Signature sig = gCoordinator.GetSignature(enemy);
                if (!sig.any()) continue;
                float &cd = gEnemyShootCooldowns[enemy];
                if (cd <= 0.f) {
                    SpawnProjectileFrom(enemy, false);
                    cd = 2.0f;
                }
            }

            // Enemy spawning
            enemySpawnTimer += fixedDt;
            if (enemySpawnTimer >= enemySpawnInterval) {
                Entity en = CreateEnemyEntity();
                gEnemies.push_back(en);
                enemySpawnTimer = 0.f;
            }

            // Process destructions and prune lists
            gCoordinator.ProcessDestructions();
            // Remove destroyed enemies from gEnemies and clear their shoot cooldown
            gEnemies.erase(std::remove_if(gEnemies.begin(), gEnemies.end(), [&](Entity e){
                bool alive = gCoordinator.GetSignature(e).any();
                if (!alive) {
                    gEnemyShootCooldowns.erase(e);
                }
                return !alive;
            }), gEnemies.end());
            // Remove destroyed projectiles from gProjectiles
            gProjectiles.erase(std::remove_if(gProjectiles.begin(), gProjectiles.end(), [&](Entity e){ return !gCoordinator.GetSignature(e).any(); }), gProjectiles.end());
            // Remove destroyed players from gClientToEntity and their cooldowns
            for (auto it = gClientToEntity.begin(); it != gClientToEntity.end(); ) {
                Entity ent = it->second;
                if (!gCoordinator.GetSignature(ent).any()) {
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

        // Broadcast snapshot to all active clients
        for (const auto &ci : activeClients) {
            std::ostringstream oss;
            int msgId = ++nextMsgId;
            oss << "STATE " << msgId << " " << tick << "\n";
            // Players
            for (const auto &kv : gClientToEntity) {
                Entity id = kv.second;
                Signature sig = gCoordinator.GetSignature(id);
                if (!sig.any()) continue;
                const auto &t = gCoordinator.GetComponent<Transform>(id);
                const auto &h = gCoordinator.GetComponent<Health>(id);
                oss << id << " P " << t.x << " " << t.y << " " << h.current << "\n";
            }
            // Enemies
            for (auto id : gEnemies) {
                Signature sig = gCoordinator.GetSignature(id);
                if (!sig.any()) continue;
                const auto &t = gCoordinator.GetComponent<Transform>(id);
                const auto &h = gCoordinator.GetComponent<Health>(id);
                oss << id << " E " << t.x << " " << t.y << " " << h.current << "\n";
            }
            // Projectiles
            for (auto id : gProjectiles) {
                Signature sig = gCoordinator.GetSignature(id);
                if (!sig.any()) continue;
                const auto &t = gCoordinator.GetComponent<Transform>(id);
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

    InitEcs();

    // std::cout << "[Server] Ready on port " << server.getPort()
    //           << " (press Ctrl+C to quit)" << std::endl;

    // Invoke the new authoritative server loop.  This will run until
    // g_running becomes false (e.g., when SIGINT is received).
    RunServerLoop(server);

    // std::cout << "\n[Server] Shutting down..." << std::endl;
    server.disconnect();
    return 0;
}