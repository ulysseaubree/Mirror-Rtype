#pragma once
#include "ecs/ecs.hpp"

namespace game {

    // Position
    struct Transform {
        float x{};
        float y{};
    };

    // Vitesse
    struct Velocity {
        float vx{};
        float vy{};
    };

    // Pour le rendu
    struct Sprite {
        int textureId{};      // un id vers une ressource texture
        int rectX{};
        int rectY{};
        int rectW{};
        int rectH{};
        int zIndex{};         // ordre d'affichage
    };

    // Collision
    struct Collider {
        float width{};
        float height{};
        bool isTrigger{};
    };

    // Vie / dégâts
    struct Health {
        int current{};
        int max{};
    };

    struct Damage {
        int amount{};
    };

    // Temps de vie d'un objet
    struct Lifetime {
        float remaining{};
    };

    // Identifiants réseau
    struct NetworkId {
        std::uint32_t id{};
    };
} // namespace game
