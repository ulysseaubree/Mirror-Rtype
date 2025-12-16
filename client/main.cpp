/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** Client prototype for Defense 1.
**
** Ce client crée une fenêtre SFML avec un fond étoilé (starfield), un vaisseau
** contrôlé par l'utilisateur et un réseau minimal pour envoyer les entrées au
** serveur et recevoir les états du monde. L'architecture est volontairement
** simple : le but est de disposer d'un prototype jouable et présentable pour
** la soutenance. Le réseau utilise un modèle serveur autoritaire :
** chaque frame, le client envoie ses commandes (direction, tir) et applique
** localement les positions envoyées par le serveur pour les entités qu'il
** possède.
*/

#include "../network/includes/udpClient.hpp"

#include <SFML/Graphics.hpp>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <sstream>
#include <limits>

// Containers for tracking networked entities and their states.
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cmath>

static std::atomic_bool g_running{true};

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

    // Containers for dynamic entities.  Each network id is mapped to a
    // unique pointer holding its SFML shape.  Additional maps store
    // the type ('P', 'E', 'B'), the health and the authoritative and
    // rendered positions to support interpolation.  The local player's
    // network id (localNetId) is assigned after the handshake.
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
        // Handshake : envoyer HELLO et attendre une réponse WELCOME <id>
        client.sendData("HELLO\r\n");
        bool gotWelcome = false;
        auto start = std::chrono::steady_clock::now();
        while (!gotWelcome) {
            std::string resp = client.receiveData(1024);
            if (!resp.empty() && resp.rfind("WELCOME", 0) == 0) {
                std::cout << resp;
                std::istringstream iss(resp);
                std::string kw; unsigned id;
                if (iss >> kw >> id) {
                    localNetId = id;
                    // Créer la forme locale
                    auto shape = std::make_unique<sf::RectangleShape>(sf::Vector2f{30.f, 20.f});
                    shape->setOrigin(shape->getSize() / 2.f);
                    shape->setFillColor(sf::Color::Cyan);
                    shape->setPosition({100.f, window.getSize().y / 2.f});
                    entities[id] = std::move(shape);
                    entityTypes[id] = 'P';
                    entityHealth[id] = 1; // provisoire
                    sf::Vector2f pos = entities[id]->getPosition();
                    serverPositions[id] = pos;
                    renderPositions[id] = pos;
                }
                gotWelcome = true;
            }
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > std::chrono::seconds(2)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!gotWelcome) {
            std::cout << "[Client] No WELCOME from server, playing offline" << std::endl;
            localNetId = 0;
            auto shape = std::make_unique<sf::RectangleShape>(sf::Vector2f{30.f, 20.f});
            shape->setOrigin(shape->getSize() / 2.f);
            shape->setFillColor(sf::Color::Cyan);
            shape->setPosition({100.f, window.getSize().y / 2.f});
            entities[localNetId] = std::move(shape);
            entityTypes[localNetId] = 'P';
            entityHealth[localNetId] = 1;
            sf::Vector2f pos = entities[localNetId]->getPosition();
            serverPositions[localNetId] = pos;
            renderPositions[localNetId] = pos;
        }
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
        // Envoyer au serveur
        if (client.isConnected()) {
            std::string m = "INPUT " + std::to_string(dir) + " " + std::to_string(fire) + "\r\n";
            client.sendData(m);
        }
        // Recevoir snapshots
        if (client.isConnected()) {
            std::string data;
            do {
                data = client.receiveData(4096);
                if (!data.empty()) {
                    std::stringstream whole(data);
                    std::string line;
                    while (std::getline(whole, line)) {
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        if (line.empty()) continue;
                        if (line.rfind("STATE", 0) == 0) {
                            std::stringstream header(line);
                            std::string kw; unsigned msgId=0, tick=0;
                            header >> kw >> msgId >> tick;
                            client.sendData("ACK " + std::to_string(msgId) + "\r\n");
                            std::unordered_set<unsigned> seen;
                            std::streampos save = whole.tellg();
                            std::string entLine;
                            while (std::getline(whole, entLine)) {
                                if (!entLine.empty() && entLine.back() == '\r') entLine.pop_back();
                                if (entLine.empty()) break;
                                if (entLine.rfind("STATE",0)==0) { whole.seekg(save); break; }
                                std::stringstream ls(entLine);
                                unsigned eid; char type; float x,y; int hp=0;
                                ls >> eid >> type >> x >> y;
                                if (type == 'P' || type == 'E') ls >> hp;
                                seen.insert(eid);
                                if (entities.find(eid) == entities.end()) {
                                    if (type == 'P') {
                                        auto s = std::make_unique<sf::RectangleShape>(sf::Vector2f{30.f,20.f});
                                        s->setOrigin(s->getSize()/2.f);
                                        s->setFillColor(eid==localNetId?sf::Color::Cyan:sf::Color::Green);
                                        entities[eid] = std::move(s);
                                    } else if (type == 'E') {
                                        auto s = std::make_unique<sf::CircleShape>(20.f);
                                        s->setOrigin(sf::Vector2f{20.f,20.f});
                                        s->setFillColor(sf::Color::Red);
                                        entities[eid] = std::move(s);
                                    } else if (type == 'B') {
                                        auto s = std::make_unique<sf::CircleShape>(5.f);
                                        s->setOrigin(sf::Vector2f{5.f,5.f});
                                        s->setFillColor(sf::Color::Yellow);
                                        entities[eid] = std::move(s);
                                    }
                                    entityTypes[eid] = type;
                                    entityHealth[eid] = hp;
                                    serverPositions[eid] = {x,y};
                                    renderPositions[eid] = {x,y};
                                } else {
                                    entityTypes[eid] = type;
                                    entityHealth[eid] = hp;
                                    serverPositions[eid] = {x,y};
                                }
                                save = whole.tellg();
                            }
                            // Supprimer les entités absentes du snapshot (sauf locale)
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
                        }
                    }
                }
            } while (!data.empty());
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