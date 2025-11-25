#include <SFML/Graphics.hpp>
#include <iostream>
#include "ecs.hpp"
#include "components.hpp"
#include "systems.hpp"
#include "componentsN.hpp"
#include "systemsN.hpp"

int main(int argc, char* argv[]) {
    std::cout << "=== R-Type Client ===" << std::endl;
    
    // Créer la fenêtre SFML
    sf::RenderWindow window(sf::VideoMode(800, 600), "R-Type");
    window.setFramerateLimit(60);
    
    std::cout << "Window created successfully!" << std::endl;
    
    // Initialiser l'ECS
    ecs::gCoordinator.Init();
    
    std::cout << "ECS initialized successfully!" << std::endl;
    
    // TODO: Initialiser les composants
    // TODO: Initialiser les systèmes
    // TODO: Connexion au serveur
    
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
        
        // TODO: Update systems
        
        // Rendu
        window.clear(sf::Color(20, 20, 40));
        
        // TODO: Render systems
        
        // Afficher un texte de test
        sf::RectangleShape testRect(sf::Vector2f(100, 50));
        testRect.setPosition(350, 275);
        testRect.setFillColor(sf::Color::Green);
        window.draw(testRect);
        
        window.display();
    }
    
    std::cout << "Client closed." << std::endl;
    
    return 0;
}