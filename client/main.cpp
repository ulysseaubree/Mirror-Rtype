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

    // Vaisseau du joueur : un simple rectangle
    sf::RectangleShape player(sf::Vector2f{30.f, 20.f});
    player.setFillColor(sf::Color::Cyan);
    player.setOrigin(player.getSize() / 2.f);
    player.setPosition({100.f, window.getSize().y / 2.f});

    // Vitesse du joueur (appliquée côté client pour l'animation locale)
    sf::Vector2f playerVel(0.f, 0.f);

    // Gestion réseau
    bool debug = false;
    UdpClient client(ip, port, debug);
    if (!client.initSocket()) {
        std::cerr << "[Client] Failed to initialize UDP client" << std::endl;
        // On continue quand même en mode solo
    } else {
        // Handshake : envoyer HELLO et attendre WELCOME
        client.sendData("HELLO\r\n");
        bool gotWelcome = false;
        auto start = std::chrono::steady_clock::now();
        while (!gotWelcome) {
            std::string resp = client.receiveData(1024);
            if (!resp.empty() && resp.rfind("WELCOME", 0) == 0) {
                std::cout << resp;
                gotWelcome = true;
            }
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > std::chrono::seconds(2)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!gotWelcome) {
            std::cout << "[Client] No WELCOME from server, playing offline" << std::endl;
        }
    }

    // Boucle principale
    sf::Clock clock;
    std::vector<int> handledIds;
    while (window.isOpen() && g_running) {
        const float dt = clock.restart().asSeconds();

        // --- Gestion des événements SFML 3 ---
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            // (optionnel) fermer avec Échap
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) {
                    window.close();
                    break;
                }
            }
        }

        // --- Entrée clavier ---
        int dir = 5; // 5 = aucune direction
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    dir = 8;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  dir = 2;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  dir = 4;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) dir = 6;
        const int fire = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ? 1 : 0;

        // Appliquer un mouvement local lissé (client side prediction simple)
        const float speed = 200.f;
        playerVel = {0.f, 0.f};
        switch (dir) {
            case 8: playerVel.y = -speed; break;
            case 2: playerVel.y =  speed; break;
            case 4: playerVel.x = -speed; break;
            case 6: playerVel.x =  speed; break;
            default: break;
        }
        player.move(playerVel * dt);

        // Contrainte aux bords de la fenêtre
        sf::Vector2f pos = player.getPosition();
        const auto winSize = window.getSize();
        pos.x = std::clamp(pos.x, 0.f, static_cast<float>(winSize.x));
        pos.y = std::clamp(pos.y, 0.f, static_cast<float>(winSize.y));
        player.setPosition(pos);

        // Envoyer au serveur
        if (client.isConnected()) {
            std::string msg = "INPUT " + std::to_string(dir) + " " + std::to_string(fire) + "\r\n";
            client.sendData(msg);
        }

        // Recevoir les états du serveur
        if (client.isConnected()) {
            std::string resp;
            do {
                resp = client.receiveData(1024);
                if (!resp.empty() && resp.rfind("STATE", 0) == 0) {
                    std::stringstream ss(resp);
                    std::string keyword; int id; unsigned tick; unsigned entity;
                    float x, y; int health;
                    ss >> keyword >> id >> tick >> entity >> x >> y >> health;

                    client.sendData("ACK " + std::to_string(id) + "\r\n");

                    if (std::find(handledIds.begin(), handledIds.end(), id) == handledIds.end()) {
                        handledIds.push_back(id);
                        player.setPosition({x, y});
                    }
                }
            } while (!resp.empty());
        }

        // Mettre à jour le starfield
        for (auto& star : stars) {
            star.position.x -= star.speed * dt;
            if (star.position.x < 0.f) {
                star.position.x = static_cast<float>(window.getSize().x);
            }
        }

        // Rendu
        window.clear(sf::Color::Black);
        for (const auto& star : stars) {
            starShape.setPosition(star.position);
            window.draw(starShape);
        }
        window.draw(player);
        window.display();
    }
    g_running = false;
    client.disconnect();
    return 0;
}