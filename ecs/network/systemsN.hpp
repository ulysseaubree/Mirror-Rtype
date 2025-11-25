#pragma once

#include "../core/ecs.hpp"
#include "../game/components.hpp"
#include "componentsN.hpp"
#include <asio.hpp>
#include <memory>
#include <iostream>
#include <cstring>
#include <cerrno>

namespace ecs {

// === UDP Network Manager (Singleton pour gérer ASIO) ===
class NetworkManager {
public:
    static NetworkManager& Instance() {
        static NetworkManager instance;
        return instance;
    }
    
    void InitializeServer(std::uint16_t port) {
        try {
            socket = std::make_unique<asio::ip::udp::socket>(
                ioContext, 
                asio::ip::udp::endpoint(asio::ip::udp::v4(), port)
            );
            socket->non_blocking(true);
            isServer = true;
            std::cout << "[Server] Listening on port " << port << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Server] Error: " << e.what() << std::endl;
        }
    }
    
    void InitializeClient() {
        try {
            socket = std::make_unique<asio::ip::udp::socket>(ioContext, asio::ip::udp::v4());
            socket->non_blocking(true);
            isServer = false;
            std::cout << "[Client] Socket initialized" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Client] Error: " << e.what() << std::endl;
        }
    }
    
    void SendTo(const std::vector<std::uint8_t>& data, const asio::ip::udp::endpoint& endpoint) {
        if (!socket) return;
        
        try {
            socket->send_to(asio::buffer(data), endpoint);
        } catch (const std::exception& e) {
            std::cerr << "[Network] Send error: " << e.what() << std::endl;
        }
    }
    
    bool ReceiveFrom(std::vector<std::uint8_t>& buffer, asio::ip::udp::endpoint& senderEndpoint) {
        if (!socket) return false;
        
        try {
            buffer.resize(1024);
            std::size_t len = socket->receive_from(asio::buffer(buffer), senderEndpoint);
            buffer.resize(len);
            return len > 0;
        } catch (const std::system_error& e) {
            // Codes d'erreur normaux en mode non-bloquant (ne rien afficher)
            auto code = e.code();
            if (code == std::errc::resource_unavailable_try_again ||
                code == std::errc::operation_would_block ||
                code.value() == EAGAIN || 
                code.value() == EWOULDBLOCK) {
                return false;
            }
            // Erreur réelle
            std::cerr << "[Network] Receive error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[Network] Receive error: " << e.what() << std::endl;
            return false;
        }
    }
    
    asio::ip::udp::endpoint ResolveEndpoint(const std::string& address, std::uint16_t port) {
        asio::ip::udp::resolver resolver(ioContext);
        auto endpoints = resolver.resolve(asio::ip::udp::v4(), address, std::to_string(port));
        return *endpoints.begin();
    }
    
    bool IsServer() const { return isServer; }
    
private:
    NetworkManager() = default;
    asio::io_context ioContext;
    std::unique_ptr<asio::ip::udp::socket> socket;
    bool isServer{false};
};

// === Server System ===
class NetworkServerSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& server = gCoordinator.GetComponent<NetworkServer>(entity);
            
            // Recevoir les messages
            ReceiveMessages(server);
            
            // Heartbeat
            server.heartbeatTimer += dt;
            if (server.heartbeatTimer >= server.heartbeatInterval) {
                SendHeartbeats(server);
                server.heartbeatTimer = 0.f;
            }
            
            // Timeout des clients
            CheckClientTimeouts(server);
        }
    }
    
    void SendSnapshot() {
        // Envoyer l'état de toutes les entités répliquées à tous les clients
        for (Entity serverEntity : entities) {
            auto& server = gCoordinator.GetComponent<NetworkServer>(serverEntity);
            
            for (const auto& client : server.clients) {
                if (!client.isConnected) continue;
                
                // Parcourir toutes les entités avec NetworkReplication
                // (géré par un autre système)
            }
        }
    }
    
private:
    void ReceiveMessages(NetworkServer& server) {
        std::vector<std::uint8_t> buffer;
        asio::ip::udp::endpoint senderEndpoint;
        
        while (NetworkManager::Instance().ReceiveFrom(buffer, senderEndpoint)) {
            NetworkMessage msg = NetworkMessage::Deserialize(buffer);
            HandleMessage(server, msg, senderEndpoint);
        }
    }
    
    void HandleMessage(NetworkServer& server, const NetworkMessage& msg, 
                      const asio::ip::udp::endpoint& senderEndpoint) {
        switch (msg.type) {
            case MessageType::ClientConnect: {
                // Vérifier si le client existe déjà (même endpoint)
                bool alreadyConnected = false;
                for (const auto& client : server.clients) {
                    if (client.endpoint == senderEndpoint && client.isConnected) {
                        // Client déjà connecté, renvoyer l'acceptation
                        NetworkMessage response;
                        response.type = MessageType::ServerAccept;
                        response.clientId = client.clientId;
                        response.entityId = 0;
                        NetworkManager::Instance().SendTo(response.Serialize(), senderEndpoint);
                        alreadyConnected = true;
                        break;
                    }
                }
                
                if (alreadyConnected) break;
                
                // Nouveau client
                std::uint32_t newClientId = server.nextClientId++;
                ClientEndpoint client;
                client.endpoint = senderEndpoint;
                client.clientId = newClientId;
                client.lastHeartbeat = std::chrono::steady_clock::now();
                client.isConnected = true;
                server.clients.push_back(client);
                
                // Envoyer l'acceptation
                NetworkMessage response;
                response.type = MessageType::ServerAccept;
                response.clientId = newClientId;
                response.entityId = 0;
                
                NetworkManager::Instance().SendTo(response.Serialize(), senderEndpoint);
                
                std::cout << "[Server] Client " << newClientId << " connected from " 
                         << senderEndpoint.address() << ":" << senderEndpoint.port() << std::endl;
                
                // Callback pour créer l'entité joueur
                if (onClientConnected) {
                    onClientConnected(newClientId);
                }
                break;
            }
            
            case MessageType::ClientInput: {
                // Traiter les inputs du client
                if (onClientInput && msg.data.size() >= sizeof(PlayerInput)) {
                    PlayerInput input;
                    std::memcpy(&input, msg.data.data(), sizeof(PlayerInput));
                    onClientInput(msg.clientId, input);
                }
                
                // Mettre à jour le heartbeat
                for (auto& client : server.clients) {
                    if (client.clientId == msg.clientId) {
                        client.lastHeartbeat = std::chrono::steady_clock::now();
                        break;
                    }
                }
                break;
            }
            
            case MessageType::Pong: {
                // Mettre à jour le heartbeat
                for (auto& client : server.clients) {
                    if (client.clientId == msg.clientId) {
                        client.lastHeartbeat = std::chrono::steady_clock::now();
                        break;
                    }
                }
                break;
            }
            
            default:
                break;
        }
    }
    
    void SendHeartbeats(NetworkServer& server) {
        NetworkMessage ping;
        ping.type = MessageType::Ping;
        ping.clientId = 0;
        ping.entityId = 0;
        
        auto serialized = ping.Serialize();
        for (const auto& client : server.clients) {
            if (client.isConnected) {
                NetworkManager::Instance().SendTo(serialized, client.endpoint);
            }
        }
    }
    
    void CheckClientTimeouts(NetworkServer& server) {
        auto now = std::chrono::steady_clock::now();
        const float timeoutSeconds = 15.f;
        
        for (auto& client : server.clients) {
            if (!client.isConnected) continue;
            
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - client.lastHeartbeat).count();
            
            if (elapsed > timeoutSeconds) {
                std::cout << "[Server] Client " << client.clientId << " timed out" << std::endl;
                client.isConnected = false;
                
                if (onClientDisconnected) {
                    onClientDisconnected(client.clientId);
                }
            }
        }
    }
    
public:
    // Callbacks
    std::function<void(std::uint32_t)> onClientConnected;
    std::function<void(std::uint32_t)> onClientDisconnected;
    std::function<void(std::uint32_t, const PlayerInput&)> onClientInput;
};

// === Client System ===
class NetworkClientSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& client = gCoordinator.GetComponent<NetworkClient>(entity);
            
            if (!client.isConnected) {
                // Tentative de connexion
                client.reconnectTimer += dt;
                if (client.reconnectTimer >= client.reconnectInterval) {
                    AttemptConnection(client);
                    client.reconnectTimer = 0.f;
                }
            }
            
            // Toujours recevoir les messages (même en attente de connexion)
            ReceiveMessages(client);
        }
    }
    
    void SendInput(const PlayerInput& input) {
        for (Entity entity : entities) {
            auto& client = gCoordinator.GetComponent<NetworkClient>(entity);
            if (!client.isConnected) continue;
            
            NetworkMessage msg;
            msg.type = MessageType::ClientInput;
            msg.clientId = client.clientId;
            msg.entityId = 0;
            msg.data.resize(sizeof(PlayerInput));
            std::memcpy(msg.data.data(), &input, sizeof(PlayerInput));
            
            NetworkManager::Instance().SendTo(msg.Serialize(), client.serverEndpoint);
        }
    }
    
private:
    void AttemptConnection(NetworkClient& client) {
        // Ne tenter de se connecter que si vraiment pas connecté
        if (client.isConnected) return;
        
        client.serverEndpoint = NetworkManager::Instance().ResolveEndpoint(
            client.serverAddress, client.serverPort);
        
        NetworkMessage msg;
        msg.type = MessageType::ClientConnect;
        msg.clientId = 0;
        msg.entityId = 0;
        
        NetworkManager::Instance().SendTo(msg.Serialize(), client.serverEndpoint);
        std::cout << "[Client] Attempting to connect to " 
                 << client.serverAddress << ":" << client.serverPort << std::endl;
    }
    
    void ReceiveMessages(NetworkClient& client) {
        std::vector<std::uint8_t> buffer;
        asio::ip::udp::endpoint senderEndpoint;
        
        while (NetworkManager::Instance().ReceiveFrom(buffer, senderEndpoint)) {
            NetworkMessage msg = NetworkMessage::Deserialize(buffer);
            HandleMessage(client, msg);
        }
    }
    
    void HandleMessage(NetworkClient& client, const NetworkMessage& msg) {
        switch (msg.type) {
            case MessageType::ServerAccept: {
                client.clientId = msg.clientId;
                client.isConnected = true;
                std::cout << "[Client] Connected! Assigned ID: " << client.clientId << std::endl;
                
                if (onConnected) {
                    onConnected(client.clientId);
                }
                break;
            }
            
            case MessageType::EntitySpawn: {
                if (onEntitySpawned) {
                    onEntitySpawned(msg);
                }
                break;
            }
            
            case MessageType::EntityDestroy: {
                if (onEntityDestroyed) {
                    onEntityDestroyed(msg.entityId);
                }
                break;
            }
            
            case MessageType::EntityUpdate: {
                if (onEntityUpdated) {
                    onEntityUpdated(msg);
                }
                break;
            }
            
            case MessageType::Ping: {
                // Répondre avec un Pong
                NetworkMessage pong;
                pong.type = MessageType::Pong;
                pong.clientId = client.clientId;
                pong.entityId = 0;
                
                NetworkManager::Instance().SendTo(pong.Serialize(), client.serverEndpoint);
                break;
            }
            
            default:
                break;
        }
    }
    
public:
    // Callbacks
    std::function<void(std::uint32_t)> onConnected;
    std::function<void(const NetworkMessage&)> onEntitySpawned;
    std::function<void(std::uint32_t)> onEntityDestroyed;
    std::function<void(const NetworkMessage&)> onEntityUpdated;
};

// === Network Replication System (pour serveur) ===
class NetworkReplicationSystem : public System {
public:
    void Update(float dt) {
        for (Entity entity : entities) {
            auto& replication = gCoordinator.GetComponent<NetworkReplication>(entity);
            replication.timeSinceLastUpdate += dt;
            
            if (replication.timeSinceLastUpdate >= replication.updateRate) {
                SyncEntity(entity, replication);
                replication.timeSinceLastUpdate = 0.f;
            }
        }
    }
    
private:
    void SyncEntity(Entity entity, NetworkReplication& replication) {
        // Créer un message de mise à jour
        NetworkMessage msg;
        msg.type = MessageType::EntityUpdate;
        msg.clientId = 0;
        
        try {
            const auto& netId = gCoordinator.GetComponent<NetworkId>(entity);
            msg.entityId = netId.id;
            
            // Sérialiser les composants à répliquer
            if (replication.replicateTransform) {
                const auto& transform = gCoordinator.GetComponent<Transform>(entity);
                
                // Format: [float x][float y][float rotation]
                std::size_t offset = msg.data.size();
                msg.data.resize(offset + sizeof(Transform));
                std::memcpy(msg.data.data() + offset, &transform, sizeof(Transform));
            }
            
            // Envoyer à tous les clients (via le serveur)
            if (pServerSystem) {
                auto serialized = msg.Serialize();
                for (Entity serverEntity : pServerSystem->entities) {
                    auto& server = gCoordinator.GetComponent<NetworkServer>(serverEntity);
                    for (const auto& client : server.clients) {
                        if (client.isConnected) {
                            NetworkManager::Instance().SendTo(serialized, client.endpoint);
                        }
                    }
                }
            }
        } catch (...) {
            // L'entité n'a pas tous les composants nécessaires
        }
    }
    
public:
    NetworkServerSystem* pServerSystem{nullptr};
};

} // namespace ecs