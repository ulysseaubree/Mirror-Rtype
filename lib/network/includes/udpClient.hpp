#pragma once

#include "export.hpp"
#include "compat.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace rtype::net {

    class RTYPE_API UdpClient {
    public:
        enum class ConnectionState {
            Disconnected,
            Connecting,
            Connected
        };

        UdpClient();
        ~UdpClient();

        bool connect(const std::string& host, uint16_t port);
        void disconnect();

        bool sendData(const std::vector<uint8_t>& data);
        bool receiveData(std::vector<uint8_t>& buffer);
        
        // Version string pour debug simple
        bool sendDataStr(const std::string& data);

        bool isConnected() const;
        ConnectionState getState() const;

    private:
        NetworkInitializer _netInit; // RAII WSAStartup
        SocketType _socket;
        sockaddr_in _serverAddr;
        
        mutable std::mutex _socketMutex;
        std::atomic<ConnectionState> _state;
        bool _initialized;
        bool _debug{true};

        // Stats basiques pour debug
        void recordSend(ssize_t bytes, bool success);
    };
}
