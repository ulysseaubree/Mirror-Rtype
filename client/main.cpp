/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** Client prototype for Defense 1.
**
*/

#include "client.hpp"

static void signal_handler(int)
{
    g_running = false;
}

// Structure représentant une étoile dans le fond
struct Star {
    sf::Vector2f position;
    float speed;
};

// Générer un vecteur d'étoiles aléatoires
std::vector<Star> generateStarfield(unsigned count, const sf::Vector2u& windowSize)
{
    std::vector<Star> stars;
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distX(0.f, static_cast<float>(windowSize.x));
    std::uniform_real_distribution<float> distY(0.f, static_cast<float>(windowSize.y));
    std::uniform_real_distribution<float> distSpeed(30.f, 120.f);
    stars.reserve(count);
    for (unsigned i = 0; i < count; ++i) {
        Star s;
        s.position = {distX(rng), distY(rng)};
        s.speed = distSpeed(rng);
        stars.push_back(s);
    }
    return stars;
}

int main(int ac, char **av)
{
    // Gérer Ctrl+C pour quitter proprement
    std::signal(SIGINT, signal_handler);

    // Configuration IP/port via environnement ou arguments
    std::string ip = "127.0.0.1";
    int port = 4242;
    if (const char* envIp = std::getenv("RTYPE_SERVER_IP")) ip = envIp;
    if (const char* envPort = std::getenv("RTYPE_SERVER_PORT")) port = std::atoi(envPort);
    if (ac >= 2) ip = av[1];
    if (ac >= 3) port = std::atoi(av[2]);

    // Créer la fenêtre
    sf::RenderWindow window(
        sf::VideoMode({800, 600}),
        "R-Type Prototype",
        sf::Style::Close
    );
    window.setVerticalSyncEnabled(true);

    // Générer un fond étoilé
    auto stars = generateStarfield(200, window.getSize());
    sf::CircleShape starShape(1.f);
    starShape.setFillColor(sf::Color::White);

    std::unordered_map<unsigned, std::unique_ptr<sf::Shape>> entities;
    std::unordered_map<unsigned, char> entityTypes;
    std::unordered_map<unsigned, int> entityHealth;
    std::unordered_map<unsigned, sf::Vector2f> serverPositions;
    std::unordered_map<unsigned, sf::Vector2f> renderPositions;
    unsigned localNetId = std::numeric_limits<unsigned>::max();
    const float playerSpeed = 200.f;

    // Gestion réseau
    bool debug = false;
    UdpClient client(ip, port, debug);
    if (!client.initSocket()) {
        std::cerr << "[Client] Failed to initialize UDP client" << std::endl;
        // Mode hors ligne possible
    } else {
        // Binary handshake: send HELLO and wait for WELCOME packet
        client.sendData(protocol::encodeHello());
        bool gotWelcome = false;
        auto start = std::chrono::steady_clock::now();
        while (!gotWelcome) {
            std::string resp = client.receiveData(1024);
            if (!resp.empty()) {
                if (resp.size() >= sizeof(protocol::PacketHeader)) {
                    protocol::PacketHeader hdr;
                    if (protocol::decodeHeader(reinterpret_cast<const uint8_t*>(resp.data()), resp.size(), hdr)) {
                        if (hdr.version == protocol::PROTOCOL_VERSION && static_cast<protocol::Opcode>(hdr.opcode) == protocol::Opcode::WELCOME) {
                            uint32_t pid;
                            if (protocol::decodeWelcome(reinterpret_cast<const uint8_t*>(resp.data()) + sizeof(protocol::PacketHeader), hdr.length, pid)) {
                                localNetId = pid;
                                // Create the local player's shape
                                auto shape = std::make_unique<sf::RectangleShape>(sf::Vector2f{30.f, 20.f});
                                shape->setOrigin(shape->getSize() / 2.f);
                                shape->setFillColor(sf::Color::Cyan);
                                shape->setPosition({100.f, window.getSize().y / 2.f});
                                entities[pid] = std::move(shape);
                                entityTypes[pid] = 'P';
                                entityHealth[pid] = 1;
                                sf::Vector2f pos = entities[pid]->getPosition();
                                serverPositions[pid] = pos;
                                renderPositions[pid] = pos;
                                gotWelcome = true;
                            }
                        }
                    }
                }
            }
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > std::chrono::seconds(2)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!gotWelcome)
            return 84;
    }

    // boucle lobby

    if (wait_to_join(&window) == 84) {
        g_running = false;
        client.disconnect();
        return 0;
    }
    // Boucle principale
    sf::Clock clock;
    while (window.isOpen() && g_running) {
        const float dt = clock.restart().asSeconds();
        // Gestion des événements
        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                window.close();
                break;
            }

            if (const auto* key = ev->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) {
                    window.close();
                    break;
                }
            }
        }
        // Entrée clavier (pavé 1-9)
        int dir = 5;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) dir = 8;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) dir = 2;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) dir = 4;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) dir = 6;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) dir = 7;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) dir = 9;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) dir = 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) dir = 3;
        const int fire = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ? 1 : 0;
        // Appliquer un mouvement local pour le joueur
        if (localNetId != std::numeric_limits<unsigned>::max()) {
            sf::Vector2f vel(0.f,0.f);
            switch (dir) {
                case 8: vel.y = -playerSpeed; break;
                case 2: vel.y = playerSpeed; break;
                case 4: vel.x = -playerSpeed; break;
                case 6: vel.x = playerSpeed; break;
                case 7: vel.x = -playerSpeed; vel.y = -playerSpeed; break;
                case 9: vel.x = playerSpeed; vel.y = -playerSpeed; break;
                case 1: vel.x = -playerSpeed; vel.y = playerSpeed; break;
                case 3: vel.x = playerSpeed; vel.y = playerSpeed; break;
            }
            if (vel.x != 0.f && vel.y != 0.f) {
                float mag = std::sqrt(vel.x*vel.x + vel.y*vel.y);
                vel.x = (vel.x/mag) * playerSpeed;
                vel.y = (vel.y/mag) * playerSpeed;
            }
            auto itR = renderPositions.find(localNetId);
            if (itR != renderPositions.end()) {
                itR->second += vel * dt;
                const auto win = window.getSize();
                itR->second.x = std::clamp(itR->second.x, 0.f, static_cast<float>(win.x));
                itR->second.y = std::clamp(itR->second.y, 0.f, static_cast<float>(win.y));
            }
        }
        // Envoyer l'entrée au serveur via la version binaire du protocole
        if (client.isConnected()) {
            client.sendData(protocol::encodeInput(static_cast<uint8_t>(dir), fire != 0));
        }
        // Recevoir les paquets réseau.  Chaque datagramme correspond à un
        // paquet binaire.  On parcourt tant qu'il y a des données.
        if (client.isConnected()) {
            std::string data;
            while (!(data = client.receiveData(4096)).empty()) {
                if (data.size() < sizeof(protocol::PacketHeader)) {
                    continue;
                }
                protocol::PacketHeader hdr;
                if (!protocol::decodeHeader(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hdr)) {
                    continue;
                }
                if (hdr.version != protocol::PROTOCOL_VERSION) {
                    continue;
                }
                const uint8_t *payload = reinterpret_cast<const uint8_t*>(data.data()) + sizeof(protocol::PacketHeader);
                size_t payloadLen = hdr.length;
                switch (static_cast<protocol::Opcode>(hdr.opcode)) {
                    case protocol::Opcode::STATE: {
                        // Decode the state snapshot.  Payload structure is
                        // described in BuildStatePacket().
                        if (payloadLen < sizeof(uint32_t) * 2 + sizeof(uint16_t) * 3) {
                            break;
                        }
                        const uint8_t *ptr = payload;
                        uint32_t msgIdNet;
                        std::memcpy(&msgIdNet, ptr, sizeof(uint32_t));
                        ptr += sizeof(uint32_t);
                        uint32_t tickNet;
                        std::memcpy(&tickNet, ptr, sizeof(uint32_t));
                        ptr += sizeof(uint32_t);
                        uint32_t msgId = ntohl(msgIdNet);
                        (void)tickNet; // tick currently unused on client
                        uint16_t nPlayersNet, nEnemiesNet, nProjsNet;
                        std::memcpy(&nPlayersNet, ptr, sizeof(uint16_t)); ptr += sizeof(uint16_t);
                        std::memcpy(&nEnemiesNet, ptr, sizeof(uint16_t)); ptr += sizeof(uint16_t);
                        std::memcpy(&nProjsNet,   ptr, sizeof(uint16_t)); ptr += sizeof(uint16_t);
                        size_t nPlayers = ntohs(nPlayersNet);
                        size_t nEnemies = ntohs(nEnemiesNet);
                        size_t nProjs   = ntohs(nProjsNet);
                        std::unordered_set<unsigned> seen;
                        // Decode players
                        for (size_t i = 0; i < nPlayers; ++i) {
                            if (ptr + sizeof(uint32_t) + 1 + sizeof(uint32_t) * 3 > payload + payloadLen) {
                                break;
                            }
                            uint32_t idNet;
                            std::memcpy(&idNet, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            unsigned eid = ntohl(idNet);
                            uint8_t typeByte = *ptr++;
                            (void)typeByte; // should be 0
                            uint32_t xBits, yBits, hpNet;
                            std::memcpy(&xBits, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            std::memcpy(&yBits, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            std::memcpy(&hpNet, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            float x, y;
                            xBits = ntohl(xBits);
                            yBits = ntohl(yBits);
                            std::memcpy(&x, &xBits, sizeof(float));
                            std::memcpy(&y, &yBits, sizeof(float));
                            int hp = static_cast<int>(ntohl(hpNet));
                            seen.insert(eid);
                            if (entities.find(eid) == entities.end()) {
                                auto shape = std::make_unique<sf::RectangleShape>(sf::Vector2f{30.f,20.f});
                                shape->setOrigin(shape->getSize()/2.f);
                                shape->setFillColor(eid==localNetId?sf::Color::Cyan:sf::Color::Green);
                                entities[eid] = std::move(shape);
                            }
                            entityTypes[eid] = 'P';
                            entityHealth[eid] = hp;
                            serverPositions[eid] = {x,y};
                            if (renderPositions.find(eid) == renderPositions.end()) {
                                renderPositions[eid] = {x,y};
                            }
                        }
                        // Decode enemies
                        for (size_t i = 0; i < nEnemies; ++i) {
                            if (ptr + sizeof(uint32_t) + 1 + sizeof(uint32_t) * 3 > payload + payloadLen) {
                                break;
                            }
                            uint32_t idNet;
                            std::memcpy(&idNet, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            unsigned eid = ntohl(idNet);
                            uint8_t typeByte = *ptr++;
                            (void)typeByte; // should be 1
                            uint32_t xBits, yBits, hpNet;
                            std::memcpy(&xBits, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            std::memcpy(&yBits, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            std::memcpy(&hpNet, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            float x, y;
                            xBits = ntohl(xBits);
                            yBits = ntohl(yBits);
                            std::memcpy(&x, &xBits, sizeof(float));
                            std::memcpy(&y, &yBits, sizeof(float));
                            int hp = static_cast<int>(ntohl(hpNet));
                            seen.insert(eid);
                            if (entities.find(eid) == entities.end()) {
                                auto shape = std::make_unique<sf::CircleShape>(20.f);
                                shape->setOrigin(sf::Vector2f{20.f,20.f});
                                shape->setFillColor(sf::Color::Red);
                                entities[eid] = std::move(shape);
                            }
                            entityTypes[eid] = 'E';
                            entityHealth[eid] = hp;
                            serverPositions[eid] = {x,y};
                            if (renderPositions.find(eid) == renderPositions.end()) {
                                renderPositions[eid] = {x,y};
                            }
                        }
                        // Decode projectiles
                        for (size_t i = 0; i < nProjs; ++i) {
                            if (ptr + sizeof(uint32_t) + 1 + sizeof(uint32_t) * 2 > payload + payloadLen) {
                                break;
                            }
                            uint32_t idNet;
                            std::memcpy(&idNet, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            unsigned eid = ntohl(idNet);
                            uint8_t typeByte = *ptr++;
                            (void)typeByte; // should be 2
                            uint32_t xBits, yBits;
                            std::memcpy(&xBits, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            std::memcpy(&yBits, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
                            float x, y;
                            xBits = ntohl(xBits);
                            yBits = ntohl(yBits);
                            std::memcpy(&x, &xBits, sizeof(float));
                            std::memcpy(&y, &yBits, sizeof(float));
                            seen.insert(eid);
                            if (entities.find(eid) == entities.end()) {
                                auto shape = std::make_unique<sf::CircleShape>(5.f);
                                shape->setOrigin(sf::Vector2f{5.f,5.f});
                                shape->setFillColor(sf::Color::Yellow);
                                entities[eid] = std::move(shape);
                            }
                            entityTypes[eid] = 'B';
                            entityHealth[eid] = 1; // bullets have implicit hp
                            serverPositions[eid] = {x,y};
                            if (renderPositions.find(eid) == renderPositions.end()) {
                                renderPositions[eid] = {x,y};
                            }
                        }
                        // Remove entities not present in snapshot (except local)
                        for (auto it = entities.begin(); it != entities.end(); ) {
                            unsigned eid = it->first;
                            if (seen.find(eid) == seen.end() && eid != localNetId) {
                                entityTypes.erase(eid);
                                entityHealth.erase(eid);
                                serverPositions.erase(eid);
                                renderPositions.erase(eid);
                                it = entities.erase(it);
                            } else {
                                ++it;
                            }
                        }
                        // Send ACK
                        client.sendData(protocol::encodeAck(msgId));
                        break;
                    }
                    case protocol::Opcode::SCOREBOARD: {
                        // Decode and display the scoreboard, then end the game
                        std::vector<protocol::ScoreEntry> scores;
                        if (protocol::decodeScoreboard(payload, payloadLen, scores)) {
                            std::cout << "\n=== Scoreboard ===\n";
                            for (const auto &entry : scores) {
                                std::cout << "Player " << entry.playerId << " - Score: " << entry.score
                                          << ", Time: " << entry.timeSurvived << "s" << std::endl;
                            }
                            std::cout << "===================\n";
                        }
                        g_running = false;
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        // Interpolation simple
        const float smoothing=10.f;
        for (auto &kv : entities) {
            unsigned eid = kv.first;
            auto itS = serverPositions.find(eid);
            auto itR = renderPositions.find(eid);
            if (itS == serverPositions.end() || itR == renderPositions.end()) continue;
            sf::Vector2f &srv = itS->second;
            sf::Vector2f &ren = itR->second;
            ren.x += (srv.x - ren.x) * smoothing * dt;
            ren.y += (srv.y - ren.y) * smoothing * dt;
        }
        // Mettre à jour le starfield
        for (auto &star : stars) {
            star.position.x -= star.speed * dt;
            if (star.position.x < 0.f) star.position.x = static_cast<float>(window.getSize().x);
        }
        // Rendu
        window.clear(sf::Color::Black);
        for (const auto &star : stars) {
            starShape.setPosition(star.position);
            window.draw(starShape);
        }
        for (const auto &kv : entities) {
            unsigned eid = kv.first;
            auto itR = renderPositions.find(eid);
            if (itR != renderPositions.end()) {
                kv.second->setPosition(itR->second);
                window.draw(*kv.second);
            }
        }
        window.display();
    }
    g_running = false;
    client.disconnect();
    return 0;
}