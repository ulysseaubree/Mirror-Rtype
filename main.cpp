#include <SFML/Graphics.hpp>
#include <random>
#include <sstream>
#include "ecs/ecs.hpp"
#include "ecs/components.hpp"
#include "ecs/systems.hpp"


using namespace ecs;

// Générateur de nombres aléatoires
std::random_device rd;
std::mt19937 gen(rd());

// Constantes du jeu
constexpr float WINDOW_WIDTH = 800.f;
constexpr float WINDOW_HEIGHT = 600.f;
constexpr float ENEMY_SPAWN_INTERVAL = 1.5f;

// État du jeu
struct GameState {
    bool isRunning{true};
    bool isPaused{false};
    float enemySpawnTimer{0.f};
    Entity playerEntity{};
    int score{0};
};

// Fonction pour créer le joueur
Entity CreatePlayer() {
    Entity player = gCoordinator.CreateEntity();
    
    gCoordinator.AddComponent<Transform>(player, Transform{
        WINDOW_WIDTH / 2.f,
        WINDOW_HEIGHT / 2.f,
        0.f
    });
    
    gCoordinator.AddComponent<Velocity>(player, Velocity{});
    
    gCoordinator.AddComponent<RenderShape>(player, RenderShape{
        RenderShape::Type::Triangle,
        sf::Color::Green,
        30.f, 30.f, 15.f
    });
    
    gCoordinator.AddComponent<PlayerControl>(player, PlayerControl{250.f, 180.f});
    
    gCoordinator.AddComponent<Collider>(player, Collider{
        Collider::Shape::Circle,
        15.f
    });
    
    gCoordinator.AddComponent<Health>(player, Health{100, 100});
    
    gCoordinator.AddComponent<Boundary>(player, Boundary{
        0.f, WINDOW_WIDTH, 0.f, WINDOW_HEIGHT, false, false
    });
    
    gCoordinator.AddComponent<Score>(player, Score{0});
    
    return player;
}

// Fonction pour créer un ennemi
Entity CreateEnemy() {
    Entity enemy = gCoordinator.CreateEntity();
    
    std::uniform_real_distribution<float> disX(50.f, WINDOW_WIDTH - 50.f);
    std::uniform_real_distribution<float> disY(-50.f, -20.f);
    std::uniform_real_distribution<float> disSpeed(50.f, 150.f);
    std::uniform_int_distribution<int> disType(0, 2);
    
    float x = disX(gen);
    float y = disY(gen);
    float speed = disSpeed(gen);
    int type = disType(gen);
    
    gCoordinator.AddComponent<Transform>(enemy, Transform{x, y, 0.f});
    gCoordinator.AddComponent<Velocity>(enemy, Velocity{0.f, speed});
    
    RenderShape shape;
    Collider collider;
    
    switch (type) {
        case 0: // Cercle
            shape = RenderShape{RenderShape::Type::Circle, sf::Color::Red, 0, 0, 15.f};
            collider = Collider{Collider::Shape::Circle, 15.f};
            break;
        case 1: // Rectangle
            shape = RenderShape{RenderShape::Type::Rectangle, sf::Color::Magenta, 30.f, 20.f};
            collider = Collider{Collider::Shape::Box, 0, 30.f, 20.f};
            break;
        case 2: // Triangle
            shape = RenderShape{RenderShape::Type::Triangle, sf::Color::Yellow, 0, 0, 20.f};
            collider = Collider{Collider::Shape::Circle, 20.f};
            break;
    }
    
    gCoordinator.AddComponent<RenderShape>(enemy, shape);
    gCoordinator.AddComponent<Collider>(enemy, collider);
    gCoordinator.AddComponent<Enemy>(enemy, Enemy{10.f});
    gCoordinator.AddComponent<RotationSpeed>(enemy, RotationSpeed{90.f + speed});
    
    gCoordinator.AddComponent<Boundary>(enemy, Boundary{
        -100.f, WINDOW_WIDTH + 100.f, -100.f, WINDOW_HEIGHT + 100.f, false, true
    });
    
    return enemy;
}

// Fonction pour gérer les collisions
void HandleCollisions(std::shared_ptr<CollisionSystem> collisionSystem, GameState& gameState) {
    for (auto& [e1, e2] : collisionSystem->collisions) {
        bool e1IsPlayer = false;
        bool e2IsPlayer = false;
        
        // Vérifier si l'une des entités est le joueur
        try {
            gCoordinator.GetComponent<PlayerControl>(e1);
            e1IsPlayer = true;
        } catch(...) {}
        
        try {
            gCoordinator.GetComponent<PlayerControl>(e2);
            e2IsPlayer = true;
        } catch(...) {}
        
        // Collision joueur-ennemi
        if (e1IsPlayer || e2IsPlayer) {
            Entity player = e1IsPlayer ? e1 : e2;
            Entity enemy = e1IsPlayer ? e2 : e1;
            
            try {
                auto& enemyComp = gCoordinator.GetComponent<Enemy>(enemy);
                auto& health = gCoordinator.GetComponent<Health>(player);
                
                if (health.invincibilityTimer <= 0.f && !health.invincible) {
                    health.current -= static_cast<int>(enemyComp.damage);
                    health.invincibilityTimer = 1.f; // 1 seconde d'invincibilité
                }
                
                // Détruire l'ennemi
                gCoordinator.DestroyEntity(enemy);
                
                // Score
                auto& score = gCoordinator.GetComponent<Score>(player);
                score.points += 10;
                gameState.score = score.points;
                
            } catch(...) {}
        }
    }
    
    collisionSystem->collisions.clear();
}

int main() {
    // Initialisation de la fenêtre
    sf::RenderWindow window(sf::VideoMode(static_cast<unsigned int>(WINDOW_WIDTH), 
                                         static_cast<unsigned int>(WINDOW_HEIGHT)), 
                           "ECS Prototype - Esquive les obstacles!");
    window.setFramerateLimit(60);
    
    // Initialisation de l'ECS
    gCoordinator.Init();
    
    // Enregistrer les composants
    gCoordinator.RegisterComponent<Transform>();
    gCoordinator.RegisterComponent<Velocity>();
    gCoordinator.RegisterComponent<RenderShape>();
    gCoordinator.RegisterComponent<PlayerControl>();
    gCoordinator.RegisterComponent<Collider>();
    gCoordinator.RegisterComponent<Health>();
    gCoordinator.RegisterComponent<Lifetime>();
    gCoordinator.RegisterComponent<Enemy>();
    gCoordinator.RegisterComponent<Score>();
    gCoordinator.RegisterComponent<Boundary>();
    gCoordinator.RegisterComponent<RotationSpeed>();
    
    // Créer les systèmes
    auto playerInputSystem = gCoordinator.RegisterSystem<PlayerInputSystem>();
    playerInputSystem->pWindow = &window;
    Signature inputSig;
    inputSig.set(gCoordinator.GetComponentType<Transform>());
    inputSig.set(gCoordinator.GetComponentType<Velocity>());
    inputSig.set(gCoordinator.GetComponentType<PlayerControl>());
    gCoordinator.SetSystemSignature<PlayerInputSystem>(inputSig);
    
    auto movementSystem = gCoordinator.RegisterSystem<MovementSystem>();
    Signature moveSig;
    moveSig.set(gCoordinator.GetComponentType<Transform>());
    moveSig.set(gCoordinator.GetComponentType<Velocity>());
    gCoordinator.SetSystemSignature<MovementSystem>(moveSig);
    
    auto renderSystem = gCoordinator.RegisterSystem<RenderSystem>();
    Signature renderSig;
    renderSig.set(gCoordinator.GetComponentType<Transform>());
    renderSig.set(gCoordinator.GetComponentType<RenderShape>());
    gCoordinator.SetSystemSignature<RenderSystem>(renderSig);
    
    auto collisionSystem = gCoordinator.RegisterSystem<CollisionSystem>();
    Signature collisionSig;
    collisionSig.set(gCoordinator.GetComponentType<Transform>());
    collisionSig.set(gCoordinator.GetComponentType<Collider>());
    gCoordinator.SetSystemSignature<CollisionSystem>(collisionSig);
    
    auto boundarySystem = gCoordinator.RegisterSystem<BoundarySystem>();
    Signature boundarySig;
    boundarySig.set(gCoordinator.GetComponentType<Transform>());
    boundarySig.set(gCoordinator.GetComponentType<Boundary>());
    gCoordinator.SetSystemSignature<BoundarySystem>(boundarySig);
    
    auto rotationSystem = gCoordinator.RegisterSystem<RotationSystem>();
    Signature rotationSig;
    rotationSig.set(gCoordinator.GetComponentType<Transform>());
    rotationSig.set(gCoordinator.GetComponentType<RotationSpeed>());
    gCoordinator.SetSystemSignature<RotationSystem>(rotationSig);
    
    auto healthSystem = gCoordinator.RegisterSystem<HealthSystem>();
    Signature healthSig;
    healthSig.set(gCoordinator.GetComponentType<Health>());
    gCoordinator.SetSystemSignature<HealthSystem>(healthSig);
    
    // État du jeu
    GameState gameState;
    gameState.playerEntity = CreatePlayer();
    
    // Police pour le texte
    sf::Font font;
    // Note: Vous devrez charger une police, sinon utilisez des formes
    
    // Horloge pour le delta time
    sf::Clock clock;
    
    // Boucle principale
    while (window.isOpen() && gameState.isRunning) {
        float dt = clock.restart().asSeconds();
        
        // Gestion des événements
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                if (event.key.code == sf::Keyboard::P) {
                    gameState.isPaused = !gameState.isPaused;
                }
            }
        }
        
        if (!gameState.isPaused) {
            // Mise à jour des systèmes
            playerInputSystem->Update(dt);
            movementSystem->Update(dt);
            rotationSystem->Update(dt);
            boundarySystem->Update();
            collisionSystem->Update();
            healthSystem->Update(dt);
            
            // Spawn des ennemis
            gameState.enemySpawnTimer += dt;
            if (gameState.enemySpawnTimer >= ENEMY_SPAWN_INTERVAL) {
                CreateEnemy();
                gameState.enemySpawnTimer = 0.f;
            }
            
            // Gérer les collisions
            HandleCollisions(collisionSystem, gameState);
            
            // Détruire les entités hors limites
            for (Entity e : boundarySystem->toDestroy) {
                gCoordinator.DestroyEntity(e);
            }
            boundarySystem->toDestroy.clear();
            
            // Détruire les entités mortes
            for (Entity e : healthSystem->toDestroy) {
                if (e == gameState.playerEntity) {
                    gameState.isRunning = false;
                }
                gCoordinator.DestroyEntity(e);
            }
            healthSystem->toDestroy.clear();
        }
        
        // Rendu
        window.clear(sf::Color(20, 20, 40));
        
        renderSystem->Render(window);
        
        // Afficher le score et la santé
        try {
            const auto& health = gCoordinator.GetComponent<Health>(gameState.playerEntity);
            
            // Barre de vie (rectangle simple)
            sf::RectangleShape healthBar(sf::Vector2f(200.f, 20.f));
            healthBar.setPosition(10.f, 10.f);
            healthBar.setFillColor(sf::Color(50, 50, 50));
            window.draw(healthBar);
            
            float healthPercent = static_cast<float>(health.current) / static_cast<float>(health.max);
            sf::RectangleShape healthFill(sf::Vector2f(200.f * healthPercent, 20.f));
            healthFill.setPosition(10.f, 10.f);
            
            if (health.invincibilityTimer > 0.f) {
                healthFill.setFillColor(sf::Color::Cyan);
            } else if (healthPercent > 0.5f) {
                healthFill.setFillColor(sf::Color::Green);
            } else if (healthPercent > 0.25f) {
                healthFill.setFillColor(sf::Color::Yellow);
            } else {
                healthFill.setFillColor(sf::Color::Red);
            }
            window.draw(healthFill);
            
            // Score (avec des rectangles pour former les chiffres - simple)
            std::ostringstream oss;
            oss << "Score: " << gameState.score;
            // Sans police, on affiche juste un indicateur visuel
            
        } catch(...) {}
        
        // Message Game Over
        if (!gameState.isRunning) {
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);
            
            // "GAME OVER" avec des rectangles
            sf::RectangleShape gameOverBox(sf::Vector2f(400.f, 100.f));
            gameOverBox.setPosition(200.f, 250.f);
            gameOverBox.setFillColor(sf::Color::Red);
            window.draw(gameOverBox);
        }
        
        if (gameState.isPaused) {
            sf::RectangleShape pauseBar1(sf::Vector2f(20.f, 60.f));
            pauseBar1.setPosition(WINDOW_WIDTH / 2.f - 25.f, WINDOW_HEIGHT / 2.f - 30.f);
            pauseBar1.setFillColor(sf::Color::White);
            window.draw(pauseBar1);
            
            sf::RectangleShape pauseBar2(sf::Vector2f(20.f, 60.f));
            pauseBar2.setPosition(WINDOW_WIDTH / 2.f + 5.f, WINDOW_HEIGHT / 2.f - 30.f);
            pauseBar2.setFillColor(sf::Color::White);
            window.draw(pauseBar2);
        }
        
        window.display();
    }
    
    return 0;
}