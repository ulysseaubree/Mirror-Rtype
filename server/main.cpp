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

std::unordered_map<std::string, ecs::Entity> gClientToEntity;

// Liste globale des ennemis générés côté serveur. Chaque ennemi est une entité ECS
// avec un Transform, une Velocity (vers la gauche), un Boundary et une Health.
static std::vector<ecs::Entity> gEnemies;

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

    return e;
}

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
            std::cout << "[Server] ACK received for message ID " << ackId << std::endl;
        }
        return "";
    }

    // PING / ECHO pour debug
    if (type == "PING") {
        return "PONG\r\n";
    }

    return "ECHO: " + msg + "\r\n";
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

    std::cout << "[Server] Ready on port " << server.getPort()
              << " (press Ctrl+C to quit)" << std::endl;

    std::vector<std::pair<int, std::string>> pendingMessages;
    int nextMsgId = 0;
    uint64_t tick = 0;
    // Minuteur de génération des ennemis
    float enemySpawnTimer = 0.f;
    const float enemySpawnInterval = 2.f; // en secondes

    auto lastTime = std::chrono::high_resolution_clock::now();
    const float fixedDt = 1.f / 60.f; // 60 FPS logique

    while (g_running) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - lastTime;

        // === Réception messages ===
        std::string msg = server.receiveData(1024);
        if (!msg.empty()) {
            const ClientInfo* client = server.getLastClient();
            if (client != nullptr) {
                std::string clientKey = client->getKey();
                std::cout << "[Server] Received from " << clientKey
                          << " : " << msg << std::endl;

                std::string resp = HandleMessageFromClient(
                    msg, clientKey, nextMsgId, pendingMessages);

                if (!resp.empty()) {
                    std::cout << "[Server] Sending to " << clientKey
                              << " : " << resp;
                    if (!server.sendData(resp, *client)) {
                        std::cerr << "[Server] Error while sending data" << std::endl;
                    }
                }
            }
        }

        // === Simulation fixe ===
        static float accumulator = 0.f;
        accumulator += elapsed.count();
        lastTime = now;

        while (accumulator >= fixedDt) {
            // Update ECS
            gInputSystem->Update();          // utilise PlayerInput -> Velocity
            gMovementSystem->Update(fixedDt);
            gBoundarySystem->Update();
            gSpawnerSystem->Update(fixedDt);
            gCollisionSystem->Update();
            gHealthSystem->Update(fixedDt);
            gLifetimeSystem->Update(fixedDt);

            gCoordinator.ProcessDestructions();

            // Spawn d'un ennemi à intervalles réguliers
            enemySpawnTimer += fixedDt;
            if (enemySpawnTimer >= enemySpawnInterval) {
                auto enemy = CreateEnemyEntity();
                gEnemies.push_back(enemy);
                enemySpawnTimer = 0.f;
            }

            tick++;
            accumulator -= fixedDt;

            // === Construire messages STATE ===
            for (const auto& [clientKey, entity] : gClientToEntity) {
                auto& t = gCoordinator.GetComponent<ecs::Transform>(entity);
                auto& h = gCoordinator.GetComponent<ecs::Health>(entity);

                int msgId = ++nextMsgId;
                std::ostringstream oss;
                oss << "STATE " << msgId << " " << tick << " "
                    << entity << " " << t.x << " " << t.y << " "
                    << h.current << "\r\n";

                pendingMessages.emplace_back(msgId, oss.str());
            }

            // Ajouter l'état de tous les ennemis
            for (auto entity : gEnemies) {
                // Récupérer la position et la santé de l'entité.  
                // Si l'entité a été détruite, ces appels peuvent échouer (assert).  
                // Dans un MVP, nous supposons que les ennemis restent valides durant la frame.
                auto& t2 = gCoordinator.GetComponent<ecs::Transform>(entity);
                auto& h2 = gCoordinator.GetComponent<ecs::Health>(entity);
                int msgId2 = ++nextMsgId;
                std::ostringstream oss2;
                oss2 << "STATE " << msgId2 << " " << tick << " "
                     << entity << " " << t2.x << " " << t2.y << " "
                     << h2.current << "\r\n";
                pendingMessages.emplace_back(msgId2, oss2.str());
            }
        }

        // === Envoi (répétitif) de tous les pendingMessages au client récent ===
        // (MVP : réutilise ton getLastClient, plus tard tu feras map clientKey -> ClientInfo)
        const ClientInfo* lastClient = server.getLastClient();
        if (lastClient != nullptr) {
            for (const auto& p : pendingMessages) {
                std::cout << "[Server] Sending to "
                          << lastClient->getKey()
                          << " : " << p.second;
                if (!server.sendData(p.second, *lastClient)) {
                    std::cerr << "[Server] Error while sending data" << std::endl;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "\n[Server] Shutting down..." << std::endl;
    server.disconnect();
    return 0;
}