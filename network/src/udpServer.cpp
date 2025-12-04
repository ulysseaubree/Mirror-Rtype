/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** UdpServer
*/

#include "../includes/udpServer.hpp"
#include "../includes/threadQueue.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <algorithm>

ClientInfo::ClientInfo(const sockaddr_in& addr)
    : address(addr)
{
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    ip = ip_str;
    port = ntohs(addr.sin_port);
    lastSeen = std::chrono::steady_clock::now();
}

std::string ClientInfo::getKey() const
{
    return ip + ":" + std::to_string(port);
}

UdpServer::UdpServer(int port, bool debug)
    : _port(port)
    , _socket(-1)
    , _initialized(false)
    , _debug(debug)
{
    std::memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(_port);
    
    _recive = new ThQueue();
    _send = new ThQueue();
}

UdpServer::~UdpServer()
{
    if (_initialized) {
        disconnect();
    }
    delete _send;
    delete _recive;
}

bool UdpServer::initSocket()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0) {
        std::cerr << "Erreur lors de la création du socket UDP." << std::endl;
        return false;
    }
    
    if (!setSocketOptions()) {
        close(_socket);
        return false;
    }
    
    if (bind(_socket, reinterpret_cast<sockaddr*>(&_serverAddr), sizeof(_serverAddr)) < 0) {
        std::cerr << "Erreur lors du bind sur le port " << _port << std::endl;
        close(_socket);
        return false;
    }
    
    _initialized = true;
    
    if (_debug) {
        std::cout << "Serveur UDP démarré sur le port " << _port << std::endl;
    }
    
    return true;
}

bool UdpServer::sendData(const std::string& data, const ClientInfo& client)
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    
    if (!_initialized) {
        std::cerr << "Le socket n'a pas été initialisé." << std::endl;
        return false;
    }
    
    if (data.empty()) {
        return false;
    }
    
    ssize_t bytes_sent = sendto(_socket, data.c_str(), data.size(), 0,
                                reinterpret_cast<const sockaddr*>(&client.address),
                                sizeof(client.address));
    
    if (bytes_sent < 0) {
        std::cerr << "Erreur lors de l'envoi de données UDP à "
                  << client.getKey() << std::endl;
        return false;
    }
    
    if (_debug) {
        std::cout << "Envoyé à " << client.getKey() << " : " 
                  << data << " (" << bytes_sent << " octets)" << std::endl;
    }
    
    return true;
}

bool UdpServer::broadcast(const std::string& data)
{
    std::lock_guard<std::mutex> clientLock(_clientMutex);
    
    if (_clients.empty()) {
        if (_debug) {
            std::cout << "Aucun client pour le broadcast." << std::endl;
        }
        return false;
    }
    
    bool allSuccess = true;
    for (const auto& [key, client] : _clients) {
        if (!sendData(data, client)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

std::string UdpServer::receiveData(int bufferSize)
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    
    if (!_initialized) {
        std::cerr << "Le socket n'a pas été initialisé." << std::endl;
        return "";
    }
    
    // Non-blocking check
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(_socket, &readfds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; // 100ms
    
    int activity = select(_socket + 1, &readfds, nullptr, nullptr, &timeout);
    
    if (activity <= 0) {
        return "";
    }
    
    // Use unique_ptr for RAII
    auto buffer = std::make_unique<char[]>(bufferSize + 1);
    std::memset(buffer.get(), 0, bufferSize + 1);
    
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    ssize_t bytes_received = recvfrom(_socket, buffer.get(), bufferSize, 0,
                                      reinterpret_cast<sockaddr*>(&client_addr),
                                      &addr_len);
    
    if (bytes_received <= 0) {
        return "";
    }
    
    std::string data(buffer.get(), bytes_received);
    
    // Update client info
    _lastClient = std::make_unique<ClientInfo>(client_addr);
    updateClient(*_lastClient);
    
    if (_debug) {
        std::cout << "Reçu de " << _lastClient->getKey() << " : " << data << std::endl;
    }
    
    return data;
}

void UdpServer::disconnect()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    
    if (_initialized) {
        close(_socket);
        _initialized = false;
        
        if (_debug) {
            std::cout << "Serveur UDP arrêté." << std::endl;
        }
    }
}

void UdpServer::setSend(const std::string& data, const std::string& clientKey)
{
    std::string formatted = data;
    if (formatted.size() < 2 || formatted.substr(formatted.size() - 2) != "\r\n") {
        formatted += "\r\n";
    }
    
    std::lock_guard<std::mutex> lock(_socketMutex);
    _send->push(clientKey + "|" + formatted);
}

void UdpServer::setReceive(const std::string& data)
{
    if (data.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(_socketMutex);
    _recive->push(data);
}

ThQueue* UdpServer::getSend()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _send;
}

ThQueue* UdpServer::getReceive()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _recive;
}

void UdpServer::updateClient(const ClientInfo& client)
{
    std::lock_guard<std::mutex> lock(_clientMutex);
    
    std::string key = client.getKey();
    auto it = _clients.find(key);
    
    if (it != _clients.end()) {
        it->second.lastSeen = std::chrono::steady_clock::now();
    } else {
        _clients[key] = client;
        if (_debug) {
            std::cout << "Nouveau client connecté : " << key << std::endl;
        }
    }
}

void UdpServer::removeClient(const std::string& clientKey)
{
    std::lock_guard<std::mutex> lock(_clientMutex);
    
    auto it = _clients.find(clientKey);
    if (it != _clients.end()) {
        _clients.erase(it);
        if (_debug) {
            std::cout << "Client déconnecté : " << clientKey << std::endl;
        }
    }
}

std::vector<ClientInfo> UdpServer::getActiveClients(int timeoutSeconds)
{
    std::lock_guard<std::mutex> lock(_clientMutex);
    
    std::vector<ClientInfo> activeClients;
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = _clients.begin(); it != _clients.end();) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.lastSeen
        ).count();
        
        if (elapsed > timeoutSeconds) {
            if (_debug) {
                std::cout << "Client timeout : " << it->first << std::endl;
            }
            it = _clients.erase(it);
        } else {
            activeClients.push_back(it->second);
            ++it;
        }
    }
    
    return activeClients;
}

size_t UdpServer::getClientCount() const
{
    std::lock_guard<std::mutex> lock(_clientMutex);
    return _clients.size();
}


bool UdpServer::getDebug() const
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _debug;
}

void UdpServer::setDebug(bool debug)
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    _debug = debug;
}

int UdpServer::getPort() const
{
    return _port;
}