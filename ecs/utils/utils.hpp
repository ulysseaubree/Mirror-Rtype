#pragma once

#include <string>
#include <memory>
#include <functional>
#include <SFML/Graphics.hpp>
#include "ecs.hpp"
#include "components.hpp"
#include "systems.hpp"

namespace ecs {

/**
 * @brief Classe utilitaire pour simplifier l'utilisation de l'ECS
 * 
 * Cette classe fournit une API haut niveau pour :
 * - Créer des entités facilement (joueurs, ennemis, projectiles)
 * - Gérer les systèmes sans toucher à SFML
 * - Simplifier les opérations courantes
 */
class Utils {
public:
    // ========================================
    // Initialisation
    // ========================================
    
    /**
     * @brief Initialise l'ECS et enregistre tous les composants
     */
    static void InitECS();
    
    /**
     * @brief Enregistre tous les systèmes nécessaires
     * @param window Pointeur vers la fenêtre SFML (nullptr pour le serveur)
     */
    static void RegisterSystems(sf::RenderWindow* window = nullptr);
    
    // ========================================
    // Création d'entités - Joueurs
    // ========================================
    
    /**
     * @brief Crée un joueur avec tous les composants nécessaires
     * @param x Position X initiale
     * @param y Position Y initiale
     * @param playerNumber Numéro du joueur (1-4)
     * @param isLocal True si c'est le joueur local (contrôlé par ce client)
     * @return L'entité créée
     */
    static Entity CreatePlayer(float x, float y, int playerNumber, bool isLocal = true);
    
    /**
     * @brief Crée un joueur réseau (contrôlé par un autre client)
     * @param x Position X initiale
     * @param y Position Y initiale
     * @param networkId ID réseau unique
     * @param playerNumber Numéro du joueur (1-4)
     * @return L'entité créée
     */
    static Entity CreateNetworkPlayer(float x, float y, uint32_t networkId, int playerNumber);
    
    // ========================================
    // Création d'entités - Ennemis
    // ========================================
    
    enum class EnemyType {
        Basic,      // Ennemi basique qui avance
        Zigzag,     // Ennemi en zigzag
        Shooter     // Ennemi qui tire
    };
    
    /**
     * @brief Crée un ennemi
     * @param x Position X initiale
     * @param y Position Y initiale
     * @param type Type d'ennemi
     * @return L'entité créée
     */
    static Entity CreateEnemy(float x, float y, EnemyType type);
    
    // ========================================
    // Création d'entités - Projectiles
    // ========================================
    
    enum class ProjectileOwner {
        Player,
        Enemy
    };
    
    /**
     * @brief Crée un projectile
     * @param x Position X initiale
     * @param y Position Y initiale
     * @param velocityX Vitesse X
     * @param velocityY Vitesse Y
     * @param owner Propriétaire (joueur ou ennemi)
     * @param ownerEntity Entité propriétaire
     * @return L'entité créée
     */
    static Entity CreateProjectile(float x, float y, float velocityX, float velocityY,
                                   ProjectileOwner owner, Entity ownerEntity);
    
    // ========================================
    // Création d'entités - Environnement
    // ========================================
    
    /**
     * @brief Crée le background scrolling
     * @return L'entité créée
     */
    static Entity CreateBackground();
    
    /**
     * @brief Crée un power-up
     * @param x Position X
     * @param y Position Y
     * @param powerUpType Type de power-up (0=vie, 1=arme, 2=shield)
     * @return L'entité créée
     */
    static Entity CreatePowerUp(float x, float y, int powerUpType);
    
    // ========================================
    // Gestion des entités
    // ========================================
    
    /**
     * @brief Détruit une entité en toute sécurité
     * @param entity L'entité à détruire
     */
    static void DestroyEntity(Entity entity);
    
    /**
     * @brief Vérifie si une entité existe
     * @param entity L'entité à vérifier
     * @return True si l'entité existe
     */
    static bool EntityExists(Entity entity);
    
    // ========================================
    // Manipulation des composants
    // ========================================
    
    /**
     * @brief Déplace une entité
     * @param entity L'entité à déplacer
     * @param dx Déplacement X
     * @param dy Déplacement Y
     */
    static void MoveEntity(Entity entity, float dx, float dy);
    
    /**
     * @brief Définit la position d'une entité
     * @param entity L'entité
     * @param x Nouvelle position X
     * @param y Nouvelle position Y
     */
    static void SetPosition(Entity entity, float x, float y);
    
    /**
     * @brief Récupère la position d'une entité
     * @param entity L'entité
     * @param outX Position X (sortie)
     * @param outY Position Y (sortie)
     * @return True si l'entité a un composant Transform
     */
    static bool GetPosition(Entity entity, float& outX, float& outY);
    
    /**
     * @brief Définit la vélocité d'une entité
     * @param entity L'entité
     * @param vx Vélocité X
     * @param vy Vélocité Y
     */
    static void SetVelocity(Entity entity, float vx, float vy);
    
    /**
     * @brief Inflige des dégâts à une entité
     * @param entity L'entité
     * @param damage Dégâts à infliger
     * @return True si l'entité est toujours vivante
     */
    static bool DamageEntity(Entity entity, int damage);
    
    /**
     * @brief Soigne une entité
     * @param entity L'entité
     * @param amount Points de vie à restaurer
     */
    static void HealEntity(Entity entity, int amount);
    
    /**
     * @brief Récupère les points de vie d'une entité
     * @param entity L'entité
     * @return Points de vie actuels (-1 si pas de composant Health)
     */
    static int GetHealth(Entity entity);
    
    // ========================================
    // Couleurs des joueurs
    // ========================================
    
    /**
     * @brief Récupère la couleur d'un joueur selon son numéro
     * @param playerNumber Numéro du joueur (1-4)
     * @return Couleur SFML
     */
    static sf::Color GetPlayerColor(int playerNumber);
    
    // ========================================
    // Utilitaires de collision
    // ========================================
    
    /**
     * @brief Vérifie si deux entités sont en collision
     * @param entity1 Première entité
     * @param entity2 Seconde entité
     * @return True si collision
     */
    static bool CheckCollision(Entity entity1, Entity entity2);
    
    /**
     * @brief Trouve toutes les entités dans un rayon
     * @param x Position X du centre
     * @param y Position Y du centre
     * @param radius Rayon de recherche
     * @return Liste des entités dans le rayon
     */
    static std::vector<Entity> FindEntitiesInRadius(float x, float y, float radius);
    
    // ========================================
    // Utilitaires de gameplay
    // ========================================
    
    /**
     * @brief Fait tirer un joueur
     * @param playerEntity Entité du joueur
     */
    static void PlayerShoot(Entity playerEntity);
    
    /**
     * @brief Fait tirer un ennemi
     * @param enemyEntity Entité de l'ennemi
     */
    static void EnemyShoot(Entity enemyEntity);
    
    /**
     * @brief Augmente le score d'un joueur
     * @param playerEntity Entité du joueur
     * @param points Points à ajouter
     */
    static void AddScore(Entity playerEntity, int points);
    
    /**
     * @brief Récupère le score d'un joueur
     * @param playerEntity Entité du joueur
     * @return Score actuel (-1 si pas de composant Score)
     */
    static int GetScore(Entity playerEntity);
    
    // ========================================
    // Informations de debug
    // ========================================
    
    /**
     * @brief Affiche les informations d'une entité
     * @param entity L'entité
     */
    static void PrintEntityInfo(Entity entity);
    
    /**
     * @brief Compte le nombre total d'entités
     * @return Nombre d'entités
     */
    static int CountEntities();
    
    /**
     * @brief Compte le nombre d'ennemis vivants
     * @return Nombre d'ennemis
     */
    static int CountEnemies();
    
    /**
     * @brief Compte le nombre de projectiles actifs
     * @return Nombre de projectiles
     */
    static int CountProjectiles();

private:
    // Variables de configuration
    static constexpr float PLAYER_SPEED = 250.0f;
    static constexpr float PLAYER_FIRE_RATE = 0.2f;
    static constexpr float PROJECTILE_SPEED = 400.0f;
    static constexpr float ENEMY_SPEED = 100.0f;
    static constexpr float BACKGROUND_SCROLL_SPEED = 50.0f;
    
    // Helpers internes
    static sf::Color GetColorForEnemyType(EnemyType type);
    static float GetSpeedForEnemyType(EnemyType type);
};

} // namespace ecs