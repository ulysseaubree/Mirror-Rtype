/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** UdpClient (improved version)
*/

#include "../includes/udpClient.hpp"
#include "../includes/threadQueue.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <cerrno>


UdpClient::UdpClient(const std::string& ip, int port, bool debug)
    : _ip(ip)
    , _port(port)
    , _socket(-1)
    , _initialized(false)
    , _debug(debug)
    , _state(ConnectionState::Disconnected)
{
    std::memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_port = htons(_port);

    _recive = new ThQueue();
    _send = new ThQueue();
}

UdpClient::~UdpClient()
{
    if (_initialized) {
        disconnect();
    }
    delete _send;
    delete _recive;
}


bool UdpClient::setSocketOptions()
{
    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100ms
    
    if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        if (_debug) {
            std::cerr << "Warning: Could not set SO_RCVTIMEO" << std::endl;
        }
    }
    
    // Set send timeout
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    if (setsockopt(_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        if (_debug) {
            std::cerr << "Warning: Could not set SO_SNDTIMEO" << std::endl;
        }
    }
    
    return true;
}

bool UdpClient::initSocket()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    
    if (_initialized) {
        if (_debug) {
            std::cout << "Socket déjà initialisé." << std::endl;
        }
        return true;
    }
    
    updateState(ConnectionState::Connecting);
    
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0) {
        std::cerr << "Erreur lors de la création du socket UDP." << std::endl;
        updateState(ConnectionState::Failed);
        return false;
    }
    
    if (inet_pton(AF_INET, _ip.c_str(), &_serverAddr.sin_addr) <= 0) {
        std::cerr << "Adresse IP invalide: " << _ip << std::endl;
        close(_socket);
        _socket = -1;
        updateState(ConnectionState::Failed);
        return false;
    }
    
    if (!setSocketOptions()) {
        if (_debug) {
            std::cerr << "Warning: Failed to set some socket options" << std::endl;
        }
    }
    
    _initialized = true;
    updateState(ConnectionState::Connected);
    
    if (_debug) {
        std::cout << "Client UDP connecté à " << _ip << ":" << _port << std::endl;
    }
    
    return true;
}

void UdpClient::disconnect()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    
    if (_initialized) {
        close(_socket);
        _socket = -1;
        _initialized = false;
        updateState(ConnectionState::Disconnected);
        
        if (_debug) {
            std::cout << "Client UDP déconnecté." << std::endl;
        }
    }
}

bool UdpClient::isConnected() const
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _initialized && _state == ConnectionState::Connected;
}

UdpClient::ConnectionState UdpClient::getState() const
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _state;
}

void UdpClient::updateState(ConnectionState newState)
{
    _state = newState;
}

bool UdpClient::sendData(const std::string& data)
{
    std::lock_guard<std::mutex> lock(_socketMutex);

    if (!_initialized) {
        std::cerr << "Le socket n'a pas été initialisé." << std::endl;
        recordSend(0, false);
        return false;
    }
    
    if (data.empty()) {
        if (_debug) {
            std::cerr << "Tentative d'envoi de données vides." << std::endl;
        }
        return false;
    }
    
    ssize_t bytes_sent = sendto(
        _socket,
        data.c_str(),
        data.size(),
        0,
        reinterpret_cast<sockaddr*>(&_serverAddr),
        sizeof(_serverAddr)
    );
    
    if (bytes_sent < 0) {
        std::cerr << "Erreur lors de l'envoi de données UDP: " 
                  << strerror(errno) << std::endl;
        recordSend(0, false);
        return false;
    }
    
    recordSend(bytes_sent, true);
    
    if (_debug) {
        // std::cout << "Envoyé: " << data << " (" << bytes_sent << " octets)" << std::endl;
    }
    
    return true;
}

std::string UdpClient::receiveData(int bufferSize)
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    
    if (!_initialized) {
        std::cerr << "Le socket n'a pas été initialisé." << std::endl;
        return "";
    }
    
    // Non-blocking check with select
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(_socket, &readfds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; // 100ms
    
    int activity = select(_socket + 1, &readfds, nullptr, nullptr, &timeout);
    
    if (activity < 0) {
        std::cerr << "Erreur select(): " << strerror(errno) << std::endl;
        recordReceive(0, false);
        return "";
    }
    
    if (activity == 0) {
        // Timeout, no data available
        return "";
    }
    
    // Use unique_ptr for automatic memory management
    auto buffer = std::make_unique<char[]>(bufferSize + 1);
    std::memset(buffer.get(), 0, bufferSize + 1);
    
    sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    
    ssize_t bytes_received = recvfrom(
        _socket,
        buffer.get(),
        bufferSize,
        0,
        reinterpret_cast<sockaddr*>(&sender_addr),
        &addr_len
    );
    
    if (bytes_received < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Erreur lors de la réception: " 
                      << strerror(errno) << std::endl;
            recordReceive(0, false);
        }
        return "";
    }
    
    if (bytes_received == 0) {
        return "";
    }
    
    std::string data(buffer.get(), bytes_received);
    recordReceive(bytes_received, true);
    
    if (_debug) {
        std::cout << "Reçu: " << data << " (" << bytes_received << " octets)" << std::endl;
    }
    
    return data;
}

UdpClient::WaitResult UdpClient::waitForResponse(
    const std::string& expectedPrefix,
    const std::string& messageToSend,
    int maxAttempts,
    int timeoutMs
)
{
    WaitResult result{false, "", 0};
    
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        result.attempts = attempt + 1;
        
        // Send message
        if (!messageToSend.empty()) {
            setSend(messageToSend);
        }
        
        // Wait for response
        auto startTime = std::chrono::steady_clock::now();
        
        while (true) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime
            ).count();
            
            if (elapsed >= timeoutMs) {
                if (_debug) {
                    std::cout << "Timeout tentative " << (attempt + 1) 
                              << "/" << maxAttempts << std::endl;
                }
                break;
            }
            
            std::string response;
            if (getReceive()->try_pop(response)) {
                if (response.rfind(expectedPrefix, 0) == 0) {
                    // Response matches expected prefix
                    result.success = true;
                    result.data = response;
                    
                    if (_debug) {
                        std::cout << "Réponse reçue après " << (attempt + 1) 
                                  << " tentative(s): " << response << std::endl;
                    }
                    
                    return result;
                }
            }
            
            // Small sleep to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Wait before retry
        if (attempt < maxAttempts - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    if (_debug) {
        std::cout << "Échec après " << maxAttempts << " tentatives" << std::endl;
    }
    
    return result;
}

void UdpClient::setSend(const std::string& data)
{
    // In the binary protocol we preserve the data exactly as provided.
    // Do not append CR/LF as was done in the textual protocol.
    std::lock_guard<std::mutex> lock(_socketMutex);
    _send->push(data);
}

void UdpClient::setReceive(const std::string& data)
{
    if (data.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(_socketMutex);
    _recive->push(data);
}

ThQueue* UdpClient::getSend()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _send;
}

ThQueue* UdpClient::getReceive()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _recive;
}

// ============================================================================
// Configuration
// ============================================================================

bool UdpClient::getDebug() const
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _debug;
}

void UdpClient::setDebug(bool debug)
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    _debug = debug;
}

std::string UdpClient::getServerIp() const
{
    return _ip;
}

int UdpClient::getServerPort() const
{
    return _port;
}

void UdpClient::recordSend(size_t bytes, bool success)
{
    std::lock_guard<std::mutex> lock(_statsMutex);
    
    if (success) {
        _stats.bytesSent += bytes;
        _stats.packetsSent++;
        _stats.lastSendTime = std::chrono::steady_clock::now();
    } else {
        _stats.sendErrors++;
    }
}

void UdpClient::recordReceive(size_t bytes, bool success)
{
    std::lock_guard<std::mutex> lock(_statsMutex);
    
    if (success) {
        _stats.bytesReceived += bytes;
        _stats.packetsReceived++;
        _stats.lastReceiveTime = std::chrono::steady_clock::now();
    } else {
        _stats.receiveErrors++;
    }
}

const UdpClient::Statistics& UdpClient::getStats() const
{
    std::lock_guard<std::mutex> lock(_statsMutex);
    return _stats;
}

void UdpClient::resetStats()
{
    std::lock_guard<std::mutex> lock(_statsMutex);
    _stats = Statistics{};
}