#include "../includes/udpClient.hpp"
#include <iostream>
#include <cstring>

namespace rtype::net {

    UdpClient::UdpClient() : 
        _socket(INVALID_SOCKET), 
        _state(ConnectionState::Disconnected), 
        _initialized(false) 
    {
    }

    UdpClient::~UdpClient() 
    {
        disconnect();
    }

    bool UdpClient::connect(const std::string& host, uint16_t port)
    {
        std::lock_guard<std::mutex> lock(_socketMutex);
        
        if (_initialized) {
            if (_debug) std::cerr << "Déjà initialisé, fermeture du socket précédent..." << std::endl;
            CLOSESOCKET(_socket);
        }

        _socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (!IS_VALID_SOCKET(_socket)) {
            std::cerr << "Erreur socket: " << LAST_ERROR << std::endl;
            return false;
        }

        // Configuration de l'adresse du serveur
        std::memset(&_serverAddr, 0, sizeof(_serverAddr));
        _serverAddr.sin_family = AF_INET;
        _serverAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &_serverAddr.sin_addr) <= 0) {
            std::cerr << "Adresse IP invalide: " << host << std::endl;
            CLOSESOCKET(_socket);
            return false;
        }

        // Configuration timeouts (1s envoi, 100ms réception)
        SetSocketTimeout(_socket, SO_SNDTIMEO, 1, 0);
        SetSocketTimeout(_socket, SO_RCVTIMEO, 0, 100000);

        _initialized = true;
        _state = ConnectionState::Connected; // UDP est sans connexion, donc logiquement "prêt"
        
        return true;
    }

    void UdpClient::disconnect()
    {
        std::lock_guard<std::mutex> lock(_socketMutex);
        if (IS_VALID_SOCKET(_socket)) {
            CLOSESOCKET(_socket);
            _socket = INVALID_SOCKET;
        }
        _state = ConnectionState::Disconnected;
        _initialized = false;
    }

    bool UdpClient::isConnected() const
    {
        // En UDP, isConnected est relatif, on vérifie juste si le socket est init
        return _initialized && _state == ConnectionState::Connected;
    }

    UdpClient::ConnectionState UdpClient::getState() const
    {
        return _state.load();
    }

    bool UdpClient::sendData(const std::vector<uint8_t>& data)
    {
        std::lock_guard<std::mutex> lock(_socketMutex);

        if (!_initialized || data.empty()) return false;
        
        ssize_t bytes_sent = sendto(
            _socket,
            reinterpret_cast<const char*>(data.data()),
            static_cast<int>(data.size()),
            0,
            reinterpret_cast<sockaddr*>(&_serverAddr),
            sizeof(_serverAddr)
        );
        
        if (bytes_sent < 0) {
            std::cerr << "Send Error: " << LAST_ERROR << std::endl;
            recordSend(0, false);
            return false;
        }
        
        recordSend(bytes_sent, true);
        return true;
    }

    bool UdpClient::sendDataStr(const std::string& data)
    {
        std::vector<uint8_t> vec(data.begin(), data.end());
        return sendData(vec);
    }

    bool UdpClient::receiveData(std::vector<uint8_t>& buffer)
    {
        // Attention: receiveData bloque pour le temps du timeout défini plus haut (100ms)
        // Ne pas lock le _socketMutex pendant tout le receive si on veut pouvoir send en parallèle depuis un autre thread,
        // mais recvfrom est thread-safe sur le descripteur OS. 
        // Par sécurité ici on lock pour protéger l'accès à _socket et _initialized.
        
        std::lock_guard<std::mutex> lock(_socketMutex);
        if (!_initialized) return false;

        buffer.resize(4096);
        sockaddr_in fromBuf;
        SockLen fromLen = sizeof(fromBuf);
        
        ssize_t bytes = recvfrom(
            _socket,
            reinterpret_cast<char*>(buffer.data()),
            static_cast<int>(buffer.size()),
            0,
            reinterpret_cast<sockaddr*>(&fromBuf),
            &fromLen
        );

        if (bytes > 0) {
            buffer.resize(bytes);
            return true;
        } else {
            // bytes == 0 ou erreur (timeout inclus)
            buffer.clear();
            return false;
        }
    }

    void UdpClient::recordSend(ssize_t bytes, bool success)
    {
        (void)bytes;
        (void)success;
        // Implémentez des métriques ici si besoin
    }
}
