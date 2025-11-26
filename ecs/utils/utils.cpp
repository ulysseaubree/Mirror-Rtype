#include "utils.hpp"
#include "../network/componentsN.hpp"
#include <iostream>
#include <cmath>

namespace ecs {

// ========================================
// Initialisation
// ========================================

void Utils::InitECS() {
    gCoordinator.Init();
    
    // Enregistrer tous les composants de base
    gCoordinator.RegisterComponent<Transform>();
    gCoordinator.RegisterComponent<Velocity>();
    gCoordinator.RegisterComponent<RenderShape>();
    gCoordinator.RegisterComponent<PlayerControl>();
    gCoordinator.RegisterComponent<Collider>();
    gCoordinator.RegisterComponent<Health>();
    gCoordinator.RegisterComponent<Lifetime>();
    gCoordinator.RegisterComponent<Enemy>();
    gCoordinator.RegisterComponent<Projectile>();
    gCoordinator.RegisterComponent<Score>();
    gCoordinator.RegisterComponent<Boundary>();
    gCoordinator.RegisterComponent<RotationSpeed>();
    
    // Composants réseau
    gCoordinator.RegisterComponent<NetworkId>();
    gCoordinator.RegisterComponent<NetworkOwnership>();
    gCoordinator.RegisterComponent<NetworkReplication>();
}

void Utils::RegisterSystems(sf::RenderWindow* window) {
    // Système de mouvement
    auto movementSystem = gCoordinator.RegisterSystem<MovementSystem>();
    Signature moveSig;
    moveSig.set(gCoordinator.GetComponentType<Transform>());
    moveSig.set(gCoordinator.GetComponentType<Velocity>());
    gCoordinator.SetSystemSignature<MovementSystem>(moveSig);
    
    // Système de rotation
    auto rotationSystem = gCoordinator.RegisterSystem<RotationSystem>();
    Signature rotationSig;
    rotationSig.set(gCoordinator.GetComponentType<Transform>());
    rotationSig.set(gCoordinator.GetComponentType<RotationSpeed>());
    gCoordinator.SetSystemSignature<RotationSystem>(rotationSig);
    
    // Système de boundaries
    auto boundarySystem = gCoordinator.RegisterSystem<BoundarySystem>();
    Signature boundarySig;
    boundarySig.set(gCoordinator.GetComponentType<Transform>());
    boundarySig.set(gCoordinator.GetComponentType<Boundary>());
    gCoordinator.SetSystemSignature<BoundarySystem>(boundarySig);
    
    // Système de collision
    auto collisionSystem = gCoordinator.RegisterSystem<CollisionSystem>();
    Signature collisionSig;
    collisionSig.set(gCoordinator.GetComponentType<Transform>());
    collisionSig.set(gCoordinator.GetComponentType<Collider>());
    gCoordinator.SetSystemSignature<CollisionSystem>(collisionSig);
    
    // Système de santé
    auto healthSystem = gCoordinator.RegisterSystem<HealthSystem>();
    Signature healthSig;
    healthSig.set(gCoordinator.GetComponentType<Health>());
    gCoordinator.SetSystemSignature<HealthSystem>(healthSig);
    
    // Système de lifetime
    auto lifetimeSystem = gCoordinator.RegisterSystem<LifetimeSystem>();
    Signature lifetimeSig;
    lifetimeSig.set(gCoordinator.GetComponentType<Lifetime>());
    gCoordinator.SetSystemSignature<LifetimeSystem>(lifetimeSig);
    
    // Systèmes client uniquement
    if (window != nullptr) {
        // Système d'input joueur
        auto inputSystem = gCoordinator.RegisterSystem<PlayerInputSystem>();
        inputSystem->pWindow = window;
        Signature inputSig;
        inputSig.set(gCoordinator.GetComponentType<Transform>());
        inputSig.set(gCoordinator.GetComponentType<Velocity>());
        inputSig.set(gCoordinator.GetComponentType<PlayerControl>());
        gCoordinator.SetSystemSignature<PlayerInputSystem>(inputSig);
        
        // Système de rendu
        auto renderSystem = gCoordinator.RegisterSystem<RenderSystem>();
        Signature renderSig;
        renderSig.set(gCoordinator.GetComponentType<Transform>());
        renderSig.set(gCoordinator.GetComponentType<RenderShape>());
        gCoordinator.SetSystemSignature<RenderSystem>(renderSig);
    }
}

// ========================================
// Création d'entités - Joueurs
// ========================================

Entity Utils::CreatePlayer(float x, float y, int playerNumber, bool isLocal) {
    Entity player = gCoordinator.CreateEntity();
    
    // Transform
    gCoordinator.AddComponent<Transform>(player, Transform{x, y, 0.0f});
    
    // Velocity
    gCoordinator.AddComponent<Velocity>(player, Velocity{0.0f, 0.0f});
    
    // Render
    gCoordinator.AddComponent<RenderShape>(player, RenderShape{
        RenderShape::Type::Triangle,
        GetPlayerColor(playerNumber),
        30.0f, 30.0f, 15.0f
    });
    
    // Player control (seulement si local)
    if (isLocal) {
        gCoordinator.AddComponent<PlayerControl>(player, PlayerControl{PLAYER_SPEED, 180.0f});
    }
    
    // Collider
    gCoordinator.AddComponent<Collider>(player, Collider{
        Collider::Shape::Circle,
        15.0f
    });
    
    // Health
    gCoordinator.AddComponent<Health>(player, Health{100, 100, false, 0.0f});
    
    // Score
    gCoordinator.AddComponent<Score>(player, Score{0});
    
    // Boundary (ne peut pas sortir de l'écran)
    gCoordinator.AddComponent<Boundary>(player, Boundary{
        0.0f, 800.0f, 0.0f, 600.0f, false, false
    });
    
    return player;
}

Entity Utils::CreateNetworkPlayer(float x, float y, uint32_t networkId, int playerNumber) {
    Entity player = CreatePlayer(x, y, playerNumber, false);
    
    // Ajouter les composants réseau
    gCoordinator.AddComponent<NetworkId>(player, NetworkId{networkId, false});
    
    gCoordinator.AddComponent<NetworkReplication>(player, NetworkReplication{
        true,  // Transform
        true,  // Velocity
        true,  // Health
        0.05f  // Update rate
    });
    
    return player;
}

// ========================================
// Création d'entités - Ennemis
// ========================================

Entity Utils::CreateEnemy(float x, float y, EnemyType type) {
    Entity enemy = gCoordinator.CreateEntity();
    
    float speed = GetSpeedForEnemyType(type);
    
    // Transform
    gCoordinator.AddComponent<Transform>(enemy, Transform{x, y, 0.0f});
    
    // Velocity (se déplace vers la gauche)
    gCoordinator.AddComponent<Velocity>(enemy, Velocity{-speed, 0.0f});
    
    // Render
    RenderShape::Type shapeType = (type == EnemyType::Shooter) ? 
        RenderShape::Type::Rectangle : RenderShape::Type::Circle;
    
    gCoordinator.AddComponent<RenderShape>(enemy, RenderShape{
        shapeType,
        GetColorForEnemyType(type),
        25.0f, 25.0f, 12.0f
    });
    
    // Collider
    gCoordinator.AddComponent<Collider>(enemy, Collider{
        Collider::Shape::Circle,
        12.0f
    });
    
    // Enemy marker
    gCoordinator.AddComponent<Enemy>(enemy, Enemy{10.0f});
    
    // Health
    int health = (type == EnemyType::Basic) ? 30 : 50;
    gCoordinator.AddComponent<Health>(enemy, Health{health, health});
    
    // Rotation pour effet visuel
    gCoordinator.AddComponent<RotationSpeed>(enemy, RotationSpeed{90.0f + speed});
    
    // Boundary (détruit si sort de l'écran)
    gCoordinator.AddComponent<Boundary>(enemy, Boundary{
        -100.0f, 900.0f, -100.0f, 700.0f, false, true
    });
    
    return enemy;
}

// ========================================
// Création d'entités - Projectiles
// ========================================

Entity Utils::CreateProjectile(float x, float y, float velocityX, float velocityY,
                               ProjectileOwner owner, Entity ownerEntity) {
    Entity projectile = gCoordinator.CreateEntity();
    
    // Transform
    gCoordinator.AddComponent<Transform>(projectile, Transform{x, y, 0.0f});
    
    // Velocity
    gCoordinator.AddComponent<Velocity>(projectile, Velocity{velocityX, velocityY});
    
    // Render
    sf::Color color = (owner == ProjectileOwner::Player) ? 
        sf::Color::Cyan : sf::Color::Red;
    
    gCoordinator.AddComponent<RenderShape>(projectile, RenderShape{
        RenderShape::Type::Circle,
        color,
        0.0f, 0.0f, 5.0f
    });
    
    // Collider
    gCoordinator.AddComponent<Collider>(projectile, Collider{
        Collider::Shape::Circle,
        5.0f
    });
    
    // Projectile marker
    gCoordinator.AddComponent<Projectile>(projectile, Projectile{25.0f, ownerEntity});
    
    // Lifetime (5 secondes)
    gCoordinator.AddComponent<Lifetime>(projectile, Lifetime{5.0f});
    
    // Boundary (détruit si sort de l'écran)
    gCoordinator.AddComponent<Boundary>(projectile, Boundary{
        -50.0f, 850.0f, -50.0f, 650.0f, false, true
    });
    
    return projectile;
}

// ========================================
// Création d'entités - Environnement
// ========================================

Entity Utils::CreateBackground() {
    Entity background = gCoordinator.CreateEntity();
    
    // Transform (commence à gauche)
    gCoordinator.AddComponent<Transform>(background, Transform{0.0f, 0.0f, 0.0f});
    
    // Velocity (scroll vers la gauche)
    gCoordinator.AddComponent<Velocity>(background, Velocity{-BACKGROUND_SCROLL_SPEED, 0.0f});
    
    // Render (grand rectangle pour le fond)
    gCoordinator.AddComponent<RenderShape>(background, RenderShape{
        RenderShape::Type::Rectangle,
        sf::Color(20, 20, 40),
        1600.0f, 600.0f, 0.0f
    });
    
    return background;
}

Entity Utils::CreatePowerUp(float x, float y, int powerUpType) {
    Entity powerUp = gCoordinator.CreateEntity();
    
    // Transform
    gCoordinator.AddComponent<Transform>(powerUp, Transform{x, y, 0.0f});
    
    // Velocity (dérive lentement vers la gauche)
    gCoordinator.AddComponent<Velocity>(powerUp, Velocity{-30.0f, 0.0f});
    
    // Render (différentes couleurs selon le type)
    sf::Color color;
    switch (powerUpType) {
        case 0: color = sf::Color::Green; break;   // Vie
        case 1: color = sf::Color::Yellow; break;  // Arme
        case 2: color = sf::Color::Cyan; break;    // Shield
        default: color = sf::Color::White; break;
    }
    
    gCoordinator.AddComponent<RenderShape>(powerUp, RenderShape{
        RenderShape::Type::Circle,
        color,
        0.0f, 0.0f, 10.0f
    });
    
    // Collider
    gCoordinator.AddComponent<Collider>(powerUp, Collider{
        Collider::Shape::Circle,
        10.0f,
        0.0f, 0.0f,
        true  // isTrigger
    });
    
    // Rotation pour effet visuel
    gCoordinator.AddComponent<RotationSpeed>(powerUp, RotationSpeed{180.0f});
    
    // Lifetime
    gCoordinator.AddComponent<Lifetime>(powerUp, Lifetime{15.0f});
    
    // Boundary
    gCoordinator.AddComponent<Boundary>(powerUp, Boundary{
        -50.0f, 850.0f, -50.0f, 650.0f, false, true
    });
    
    return powerUp;
}

// ========================================
// Gestion des entités
// ========================================

void Utils::DestroyEntity(Entity entity) {
    try {
        gCoordinator.DestroyEntity(entity);
    } catch (...) {
        std::cerr << "Warning: Failed to destroy entity " << entity << std::endl;
    }
}

bool Utils::EntityExists(Entity entity) {
    try {
        // Essayer de récupérer un composant commun
        gCoordinator.GetComponent<Transform>(entity);
        return true;
    } catch (...) {
        return false;
    }
}

// ========================================
// Manipulation des composants
// ========================================

void Utils::MoveEntity(Entity entity, float dx, float dy) {
    try {
        auto& transform = gCoordinator.GetComponent<Transform>(entity);
        transform.x += dx;
        transform.y += dy;
    } catch (...) {
        std::cerr << "Warning: Entity " << entity << " has no Transform component" << std::endl;
    }
}

void Utils::SetPosition(Entity entity, float x, float y) {
    try {
        auto& transform = gCoordinator.GetComponent<Transform>(entity);
        transform.x = x;
        transform.y = y;
    } catch (...) {
        std::cerr << "Warning: Entity " << entity << " has no Transform component" << std::endl;
    }
}

bool Utils::GetPosition(Entity entity, float& outX, float& outY) {
    try {
        const auto& transform = gCoordinator.GetComponent<Transform>(entity);
        outX = transform.x;
        outY = transform.y;
        return true;
    } catch (...) {
        return false;
    }
}

void Utils::SetVelocity(Entity entity, float vx, float vy) {
    try {
        auto& velocity = gCoordinator.GetComponent<Velocity>(entity);
        velocity.vx = vx;
        velocity.vy = vy;
    } catch (...) {
        std::cerr << "Warning: Entity " << entity << " has no Velocity component" << std::endl;
    }
}

bool Utils::DamageEntity(Entity entity, int damage) {
    try {
        auto& health = gCoordinator.GetComponent<Health>(entity);
        
        if (health.invincibilityTimer > 0.0f || health.invincible) {
            return true; // Toujours vivant (invincible)
        }
        
        health.current -= damage;
        health.invincibilityTimer = 0.5f; // 0.5s d'invincibilité
        
        return health.current > 0;
    } catch (...) {
        return false;
    }
}

void Utils::HealEntity(Entity entity, int amount) {
    try {
        auto& health = gCoordinator.GetComponent<Health>(entity);
        health.current = std::min(health.current + amount, health.max);
    } catch (...) {
        std::cerr << "Warning: Entity " << entity << " has no Health component" << std::endl;
    }
}

int Utils::GetHealth(Entity entity) {
    try {
        const auto& health = gCoordinator.GetComponent<Health>(entity);
        return health.current;
    } catch (...) {
        return -1;
    }
}

// ========================================
// Couleurs des joueurs
// ========================================

sf::Color Utils::GetPlayerColor(int playerNumber) {
    switch (playerNumber) {
        case 1: return sf::Color::Green;
        case 2: return sf::Color::Blue;
        case 3: return sf::Color::Yellow;
        case 4: return sf::Color::Magenta;
        default: return sf::Color::White;
    }
}

// ========================================
// Utilitaires de collision
// ========================================

bool Utils::CheckCollision(Entity entity1, Entity entity2) {
    try {
        const auto& t1 = gCoordinator.GetComponent<Transform>(entity1);
        const auto& t2 = gCoordinator.GetComponent<Transform>(entity2);
        const auto& c1 = gCoordinator.GetComponent<Collider>(entity1);
        const auto& c2 = gCoordinator.GetComponent<Collider>(entity2);
        
        float dx = t2.x - t1.x;
        float dy = t2.y - t1.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // Collision circulaire simple
        float r1 = (c1.shape == Collider::Shape::Circle) ? c1.radius : 
                   std::sqrt(c1.width * c1.width + c1.height * c1.height) / 2.0f;
        float r2 = (c2.shape == Collider::Shape::Circle) ? c2.radius : 
                   std::sqrt(c2.width * c2.width + c2.height * c2.height) / 2.0f;
        
        return distance < (r1 + r2);
    } catch (...) {
        return false;
    }
}

std::vector<Entity> Utils::FindEntitiesInRadius(float x, float y, float radius) {
    std::vector<Entity> result;
    
    // TODO: Parcourir toutes les entités avec Transform
    // Pour l'instant, pas d'implémentation optimale
    
    return result;
}

// ========================================
// Utilitaires de gameplay
// ========================================

void Utils::PlayerShoot(Entity playerEntity) {
    try {
        const auto& transform = gCoordinator.GetComponent<Transform>(playerEntity);
        
        // Créer un projectile devant le joueur
        CreateProjectile(
            transform.x + 20.0f, 
            transform.y,
            PROJECTILE_SPEED, 
            0.0f,
            ProjectileOwner::Player,
            playerEntity
        );
    } catch (...) {
        std::cerr << "Warning: Cannot shoot from entity " << playerEntity << std::endl;
    }
}

void Utils::EnemyShoot(Entity enemyEntity) {
    try {
        const auto& transform = gCoordinator.GetComponent<Transform>(enemyEntity);
        
        // Créer un projectile vers la gauche
        CreateProjectile(
            transform.x - 20.0f, 
            transform.y,
            -PROJECTILE_SPEED, 
            0.0f,
            ProjectileOwner::Enemy,
            enemyEntity
        );
    } catch (...) {
        std::cerr << "Warning: Cannot shoot from entity " << enemyEntity << std::endl;
    }
}

void Utils::AddScore(Entity playerEntity, int points) {
    try {
        auto& score = gCoordinator.GetComponent<Score>(playerEntity);
        score.points += points;
    } catch (...) {
        std::cerr << "Warning: Entity " << playerEntity << " has no Score component" << std::endl;
    }
}

int Utils::GetScore(Entity playerEntity) {
    try {
        const auto& score = gCoordinator.GetComponent<Score>(playerEntity);
        return score.points;
    } catch (...) {
        return -1;
    }
}

// ========================================
// Informations de debug
// ========================================

void Utils::PrintEntityInfo(Entity entity) {
    std::cout << "Entity " << entity << ":" << std::endl;
    
    try {
        const auto& transform = gCoordinator.GetComponent<Transform>(entity);
        std::cout << "  Position: (" << transform.x << ", " << transform.y << ")" << std::endl;
    } catch (...) {}
    
    try {
        const auto& velocity = gCoordinator.GetComponent<Velocity>(entity);
        std::cout << "  Velocity: (" << velocity.vx << ", " << velocity.vy << ")" << std::endl;
    } catch (...) {}
    
    try {
        const auto& health = gCoordinator.GetComponent<Health>(entity);
        std::cout << "  Health: " << health.current << "/" << health.max << std::endl;
    } catch (...) {}
    
    try {
        const auto& score = gCoordinator.GetComponent<Score>(entity);
        std::cout << "  Score: " << score.points << std::endl;
    } catch (...) {}
}

int Utils::CountEntities() {
    // TODO: Implémenter avec un compteur dans l'EntityManager
    return 0;
}

int Utils::CountEnemies() {
    // TODO: Parcourir toutes les entités avec Enemy component
    return 0;
}

int Utils::CountProjectiles() {
    // TODO: Parcourir toutes les entités avec Projectile component
    return 0;
}

// ========================================
// Helpers internes
// ========================================

sf::Color Utils::GetColorForEnemyType(EnemyType type) {
    switch (type) {
        case EnemyType::Basic: return sf::Color::Red;
        case EnemyType::Zigzag: return sf::Color::Magenta;
        case EnemyType::Shooter: return sf::Color::Yellow;
        default: return sf::Color::White;
    }
}

float Utils::GetSpeedForEnemyType(EnemyType type) {
    switch (type) {
        case EnemyType::Basic: return 80.0f;
        case EnemyType::Zigzag: return 120.0f;
        case EnemyType::Shooter: return 60.0f;
        default: return 100.0f;
    }
}

} // namespace ecs