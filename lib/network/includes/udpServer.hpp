#pragma once

#include "export.hpp"
#include "compat.hpp"   
#include <string>
#include <mutex>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

class ThQueue;

namespace rtype::net {

    struct ClientInfo {
        sockaddr_in address;
        std::string ip;
        int port;
        std::chrono::steady_clock::time_point lastSeen;
        
        ClientInfo() = default; 
        ClientInfo(const sockaddr_in& addr);
        std::string getKey() const;
    };

    class RTYPE_API UdpServer {
    public:
        UdpServer(uint16_t port);
        ~UdpServer();

        bool start();
        void stop();
        
        // Empile un message à envoyer à un client spécifique
        void send(const std::string& clientKey, const std::vector<uint8_t>& data);
        
        // Récupère les messages reçus depuis la queue thread-safe
        bool popMessage(std::pair<std::string, std::vector<uint8_t>>& msg);

        // Nettoyage des clients inactifs
        void cleanInactiveClients(int timeoutSeconds = 10);

        bool isRunning() const { return _running; }

    private:
        void receiveLoop();
        bool setSocketOptions();
        void removeClient(const std::string& clientKey);

        NetworkInitializer _netInit; // RAII pour Windows
        SocketType _socket;
        sockaddr_in _serverAddr;
        uint16_t _port;
        bool _running;
        bool _debug{true}; // Mettre à false en prod

        std::mutex _clientMutex;
        std::unordered_map<std::string, ClientInfo> _clients;

        // Queue thread-safe pour les messages entrants
        // On évite d'inclure ThreadQueue partout, on utilise un pointeur opaque ou include
        // Ici simplification: on suppose que vous gérez la queue
        // Pour l'exemple je mets des vectors protégés, mais utilisez votre ThreadQueue
        std::mutex _queueMutex;
        std::vector<std::pair<std::string, std::vector<uint8_t>>> _incomingQueue;
    };
}
