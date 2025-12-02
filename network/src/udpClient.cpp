/*
** EPITECH PROJECT, 2025
** jetpack
** File description:
** UdpClient
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


UdpClient::UdpClient(const std::string& ip, int port, bool debug)
    : _ip(ip), _port(port), _socket(-1), _initialized(false), _debug(debug)
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


bool UdpClient::initSocket()
{
    std::lock_guard<std::mutex> lock(_socketMutex);

    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0) {
        std::cerr << "Erreur lors de la création du socket UDP." << std::endl;
        return false;
    }

    if (inet_pton(AF_INET, _ip.c_str(), &_serverAddr.sin_addr) <= 0) {
        std::cerr << "Adresse IP invalide." << std::endl;
        close(_socket);
        return false;
    }

    _initialized = true;
    return true;
}


bool UdpClient::sendData(const std::string& data)
{
    std::lock_guard<std::mutex> lock(_socketMutex);

    if (!_initialized) {
        std::cerr << "Le socket n'a pas été initialisé." << std::endl;
        return false;
    }

    ssize_t bytes_sent = sendto(_socket, data.c_str(), data.size(), 0, 
                                reinterpret_cast<sockaddr*>(&_serverAddr), sizeof(_serverAddr));

    if (bytes_sent < 0) {
        std::cerr << "Erreur lors de l'envoi de données UDP." << std::endl;
        return false;
    }
    if (_debug) {
        std::cout << "envoyé :" << data << " (" << bytes_sent << " octets)" << std::endl;
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
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(_socket, &readfds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    int activity = select(_socket + 1, &readfds, nullptr, nullptr, &timeout);

    if (activity <= 0) {
        return "";
    }

    char* buffer = new char[bufferSize + 1];
    std::memset(buffer, 0, bufferSize + 1);

    sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    ssize_t bytes_received = recvfrom(_socket, buffer, bufferSize, 0, 
                                      reinterpret_cast<sockaddr*>(&sender_addr), &addr_len);

    if (bytes_received <= 0) {

        delete[] buffer;
        return "";
    }

    std::string data(buffer, bytes_received);
    delete[] buffer;

    if (_debug) {
        std::cout << "reçu :" << data << std::endl;
    }

    return data;
}

std::string UdpClient::waiter(std::string expect, std::string to_send, int i)
{
    std::string tmp = "NA";

    for (; i < 5; ++i) {
        getReceive()->try_pop(tmp);
        if (tmp.rfind(expect, 0) == 0) {
            return tmp;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        setSend(to_send);
    }
    return tmp;
}


void UdpClient::disconnect()
{
    std::lock_guard<std::mutex> lock(_socketMutex);

    if (_initialized) {
        close(_socket);
        _initialized = false;
    }
}

void UdpClient::setSend(std::string s)
{
    if (s.size() < 2 || s.substr(s.size() - 2) != "\r\n") {
        s += "\r\n";
    }
    std::lock_guard<std::mutex> lock(_socketMutex);
    _send->push(s);
}

void UdpClient::setReceive(std::string r)
{
    if (r.empty())
        return;

    std::lock_guard<std::mutex> lock(_socketMutex);
    _recive->push(r);
}

ThQueue * UdpClient::getSend()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _send;
}

ThQueue * UdpClient::getReceive()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _recive;

}

bool UdpClient::getDebug()
{
    std::lock_guard<std::mutex> lock(_socketMutex);
    return _debug;
}