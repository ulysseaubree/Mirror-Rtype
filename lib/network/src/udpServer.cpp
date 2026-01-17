#include "../includes/udpServer.hpp"
#include <iostream>
#include <cstring>
#include <thread>

namespace rtype::net {

    ClientInfo::ClientInfo(const sockaddr_in& addr) : address(addr)
    {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ipStr, INET_ADDRSTRLEN);
        ip = std::string(ipStr);
        port = ntohs(addr.sin_port);
        lastSeen = std::chrono::steady_clock::now();
    }

    std::string ClientInfo::getKey() const
    {
        return ip + ":" + std::to_string(port);
    }

    UdpServer::UdpServer(uint16_t port) : 
        _socket(INVALID_SOCKET), 
        _port(port), 
        _running(false)
    {
    }

    UdpServer::~UdpServer()
    {
        stop();
    }

    bool UdpServer::start()
    {
        // Création du socket
        _socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (!IS_VALID_SOCKET(_socket)) {
            std::cerr << "Erreur création socket UDP: " << LAST_ERROR << std::endl;
            return false;
        }

        // Options
        if (!setSocketOptions()) {
            CLOSESOCKET(_socket);
            return false;
        }

        // Bind
        std::memset(&_serverAddr, 0, sizeof(_serverAddr));
        _serverAddr.sin_family = AF_INET;
        _serverAddr.sin_addr.s_addr = INADDR_ANY;
        _serverAddr.sin_port = htons(_port);

        if (bind(_socket, reinterpret_cast<sockaddr*>(&_serverAddr), sizeof(_serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Erreur bind UDP: " << LAST_ERROR << std::endl;
            CLOSESOCKET(_socket);
            return false;
        }

        _running = true;
        
        // Démarrage du thread de réception
        std::thread([this]() { receiveLoop(); }).detach();
        
        std::cout << "Serveur UDP démarré sur le port " << _port << std::endl;
        return true;
    }

    void UdpServer::stop()
    {
        _running = false;
        if (IS_VALID_SOCKET(_socket)) {
            CLOSESOCKET(_socket);
            _socket = INVALID_SOCKET;
        }
    }

    bool UdpServer::setSocketOptions()
    {
        // 1. Timeout Réception (avoid blocking forever) : 100ms
        if (!SetSocketTimeout(_socket, SO_RCVTIMEO, 0, 100000)) {
            if (_debug) std::cerr << "Warning: Could not set SO_RCVTIMEO" << std::endl;
        }

        // 2. Timeout Envoi : 1s
        if (!SetSocketTimeout(_socket, SO_SNDTIMEO, 1, 0)) {
            if (_debug) std::cerr << "Warning: Could not set SO_SNDTIMEO" << std::endl;
        }

        return true;
    }

    void UdpServer::receiveLoop()
    {
        std::vector<uint8_t> buffer(4096); 
        sockaddr_in clientAddr;
        SockLen clientLen = sizeof(clientAddr);

        while (_running) {
            ssize_t bytesReceived = recvfrom(
                _socket, 
                reinterpret_cast<char*>(buffer.data()), // Cast char* nécessaire pour Win
                static_cast<int>(buffer.size()),        // Cast int pour Length
                0, 
                reinterpret_cast<sockaddr*>(&clientAddr), 
                &clientLen
            );

            if (bytesReceived > 0) {
                // Gestion Client
                ClientInfo info(clientAddr);
                std::string key = info.getKey();

                {
                    std::lock_guard<std::mutex> lock(_clientMutex);
                    auto it = _clients.find(key);
                    if (it == _clients.end()) {
                        _clients[key] = info;
                        if (_debug) std::cout << "Nouveau client: " << key << std::endl;
                    } else {
                        it->second.lastSeen = std::chrono::steady_clock::now();
                    }
                }

                // Push message to queue
                std::vector<uint8_t> data(buffer.begin(), buffer.begin() + bytesReceived);
                {
                    std::lock_guard<std::mutex> qLock(_queueMutex);
                    _incomingQueue.push_back({key, data});
                }
            } 
            else if (bytesReceived == SOCKET_ERROR) {
                // Sur Windows, WSAGetLastError() pour check si c'est un timeout
                // Sur Linux, errno
                // Ici on ignore les timeouts normaux
            }
        }
    }

    void UdpServer::send(const std::string& clientKey, const std::vector<uint8_t>& data)
    {
        sockaddr_in target;
        {
            std::lock_guard<std::mutex> lock(_clientMutex);
            auto it = _clients.find(clientKey);
            if (it == _clients.end()) return; // Client inconnu
            target = it->second.address;
        }

        ssize_t sent = sendto(
            _socket,
            reinterpret_cast<const char*>(data.data()),
            static_cast<int>(data.size()),
            0,
            reinterpret_cast<sockaddr*>(&target),
            sizeof(target)
        );

        if (sent < 0 && _debug) {
            std::cerr << "Erreur send UDP vers " << clientKey << std::endl;
        }
    }

    void UdpServer::removeClient(const std::string& clientKey)
    {
        std::lock_guard<std::mutex> lock(_clientMutex);
        auto it = _clients.find(clientKey);
        if (it != _clients.end()) {
            _clients.erase(it);
        }
    }

    void UdpServer::cleanInactiveClients(int timeoutSeconds)
    {
        std::lock_guard<std::mutex> lock(_clientMutex);
        auto now = std::chrono::steady_clock::now();
        
        for (auto it = _clients.begin(); it != _clients.end(); ) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastSeen).count();
            if (elapsed > timeoutSeconds) {
                if (_debug) std::cout << "Timeout client: " << it->first << std::endl;
                it = _clients.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    bool UdpServer::popMessage(std::pair<std::string, std::vector<uint8_t>>& msg) {
        std::lock_guard<std::mutex> lock(_queueMutex);
        if (_incomingQueue.empty()) return false;
        msg = _incomingQueue.front();
        _incomingQueue.erase(_incomingQueue.begin());
        return true;
    }
}
