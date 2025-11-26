#include <SFML/Graphics.hpp>
#include <iostream>

#include "utils.hpp"

using namespace ecs;

int main(int argc, char* argv[]) {
    std::cout << "=== R-Type Client ===" << std::endl;
    
    // Créer la fenêtre SFML
    sf::RenderWindow window(sf::VideoMode(800, 600), "R-Type");
    window.setFramerateLimit(60);
    
    std::cout << "Window created!" << std::endl;
    
    // Initialiser l'ECS avec Utils
    Utils::InitECS();
    Utils::RegisterSystems(&window);
    
    std::cout << "ECS initialized!" << std::endl;
    
    // Créer le joueur avec Utils (super simple!)
    Entity player = Utils::CreatePlayer(100.0f, 300.0f, 1, true);
    std::cout << "Player created!" << std::endl;
    
    // Créer quelques ennemis
    Entity enemy1 = Utils::CreateEnemy(700.0f, 150.0f, Utils::EnemyType::Basic);
    Entity enemy2 = Utils::CreateEnemy(750.0f, 300.0f, Utils::EnemyType::Zigzag);
    Entity enemy3 = Utils::CreateEnemy(700.0f, 450.0f, Utils::EnemyType::Shooter);
    
    std::cout << "Enemies created!" << std::endl;
    
    // Variables pour le tir
    sf::Clock shootClock;
    const float shootCooldown = 0.3f;
    
    // Horloge pour le delta time
    sf::Clock clock;
    
    // Boucle principale
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        
        // Événements
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
            }
        }
        
        // Tir avec espace (utilise Utils::PlayerShoot!)
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            if (shootClock.getElapsedTime().asSeconds() >= shootCooldown) {
                Utils::PlayerShoot(player);
                shootClock.restart();
            }
        }
        
        // TODO: Update systems
        // movementSystem->Update(dt);
        // collisionSystem->Update();
        // etc.
        
        // Rendu
        window.clear(sf::Color(20, 20, 40));
        
        // TODO: renderSystem->Render(window);
        
        // Afficher un texte de debug
        sf::RectangleShape debugRect(sf::Vector2f(200, 30));
        debugRect.setPosition(10, 10);
        debugRect.setFillColor(sf::Color::Green);
        window.draw(debugRect);
        
        window.display();
    }
    
    std::cout << "Client closed." << std::endl;
    
    return 0;
}