#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <asio.hpp>
#include "../core/types.hpp"

namespace ecs {

// === Network Components ===

struct NetworkId {
    std::uint32_t id{0}; // ID unique réseau de l'entité
    bool isLocal{true};  // true si contrôlé localement
};

struct NetworkTransform {
    float lastX{0.f};
    float lastY{0.f};
    float lastRotation{0.f};
    std::chrono::steady_clock::time_point lastUpdateTime;
    bool needsSync{false};
};

struct NetworkOwnership {
    std::uint32_t ownerId{0}; // ID du client propriétaire (0 = serveur)
};

struct NetworkReplication {
    bool replicateTransform{true};
    bool replicateVelocity{false};
    bool replicateHealth{false};
    float updateRate{0.05f}; // Secondes entre chaque sync
    float timeSinceLastUpdate{0.f};
};

// Structure pour les messages réseau
enum class MessageType : std::uint8_t {
    // Client -> Server
    ClientConnect = 0,
    ClientDisconnect = 1,
    ClientInput = 2,
    
    // Server -> Client
    ServerAccept = 10,
    ServerReject = 11,
    ServerSnapshot = 12,
    EntitySpawn = 13,
    EntityDestroy = 14,
    EntityUpdate = 15,
    
    // Bidirectionnel
    Ping = 20,
    Pong = 21
};

struct NetworkMessage {
    MessageType type;
    std::uint32_t clientId;
    std::uint32_t entityId;
    std::vector<std::uint8_t> data;
    
    // Sérialisation basique
    std::vector<std::uint8_t> Serialize() const {
        std::vector<std::uint8_t> buffer;
        buffer.push_back(static_cast<std::uint8_t>(type));
        
        // ClientId (4 bytes)
        buffer.push_back((clientId >> 24) & 0xFF);
        buffer.push_back((clientId >> 16) & 0xFF);
        buffer.push_back((clientId >> 8) & 0xFF);
        buffer.push_back(clientId & 0xFF);
        
        // EntityId (4 bytes)
        buffer.push_back((entityId >> 24) & 0xFF);
        buffer.push_back((entityId >> 16) & 0xFF);
        buffer.push_back((entityId >> 8) & 0xFF);
        buffer.push_back(entityId & 0xFF);
        
        // Data length (2 bytes)
        std::uint16_t dataLen = static_cast<std::uint16_t>(data.size());
        buffer.push_back((dataLen >> 8) & 0xFF);
        buffer.push_back(dataLen & 0xFF);
        
        // Data
        buffer.insert(buffer.end(), data.begin(), data.end());
        
        return buffer;
    }
    
    static NetworkMessage Deserialize(const std::vector<std::uint8_t>& buffer) {
        NetworkMessage msg;
        if (buffer.size() < 11) return msg;
        
        msg.type = static_cast<MessageType>(buffer[0]);
        
        msg.clientId = (static_cast<std::uint32_t>(buffer[1]) << 24) |
                       (static_cast<std::uint32_t>(buffer[2]) << 16) |
                       (static_cast<std::uint32_t>(buffer[3]) << 8) |
                       static_cast<std::uint32_t>(buffer[4]);
        
        msg.entityId = (static_cast<std::uint32_t>(buffer[5]) << 24) |
                       (static_cast<std::uint32_t>(buffer[6]) << 16) |
                       (static_cast<std::uint32_t>(buffer[7]) << 8) |
                       static_cast<std::uint32_t>(buffer[8]);
        
        std::uint16_t dataLen = (static_cast<std::uint16_t>(buffer[9]) << 8) |
                                static_cast<std::uint16_t>(buffer[10]);
        
        if (buffer.size() >= 11 + dataLen) {
            msg.data.assign(buffer.begin() + 11, buffer.begin() + 11 + dataLen);
        }
        
        return msg;
    }
};

// Structure pour les endpoints clients
struct ClientEndpoint {
    asio::ip::udp::endpoint endpoint;
    std::uint32_t clientId;
    std::chrono::steady_clock::time_point lastHeartbeat;
    bool isConnected{false};
};

// Composant pour marquer le serveur
struct NetworkServer {
    std::uint16_t port{12345};
    std::vector<ClientEndpoint> clients;
    std::uint32_t nextClientId{1};
    float heartbeatInterval{5.f};
    float heartbeatTimer{0.f};
};

// Composant pour marquer le client
struct NetworkClient {
    std::string serverAddress{"127.0.0.1"};
    std::uint16_t serverPort{12345};
    std::uint32_t clientId{0};
    bool isConnected{false};
    float reconnectTimer{0.f};
    float reconnectInterval{5.f};
    asio::ip::udp::endpoint serverEndpoint;
};

// Structure pour les inputs du joueur (à envoyer sur le réseau)
struct PlayerInput {
    bool moveUp{false};
    bool moveDown{false};
    bool moveLeft{false};
    bool moveRight{false};
    bool action1{false}; // Tir, interaction, etc.
    std::uint32_t sequenceNumber{0};
};

// Composant pour stocker les inputs à envoyer
struct NetworkInput {
    PlayerInput currentInput;
    std::vector<PlayerInput> inputHistory; // Pour reconciliation client-side prediction
    std::uint32_t lastAcknowledgedSequence{0};
};

} // namespace ecs