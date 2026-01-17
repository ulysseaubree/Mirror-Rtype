#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <functional>
#include <chrono>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>

class ThQueue;

class UdpClient {
public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Failed
    };
    
    UdpClient(const std::string& ip, int port, bool debug = false);
    ~UdpClient();
    
    // Disable copy
    UdpClient(const UdpClient&) = delete;
    UdpClient& operator=(const UdpClient&) = delete;
    
    // Connection management
    bool initSocket();
    void disconnect();
    bool isConnected() const;
    ConnectionState getState() const;
    
    // Data transmission
    bool sendData(const std::string& data);
    std::string receiveData(int bufferSize = 1024);
    
    // Advanced waiting with retry logic
    struct WaitResult {
        bool success;
        std::string data;
        int attempts;
    };
    WaitResult waitForResponse(
        const std::string& expectedPrefix,
        const std::string& messageToSend,
        int maxAttempts = 5,
        int timeoutMs = 2000
    );
    
    // Queue management
    void setSend(const std::string& data);
    void setReceive(const std::string& data);
    ThQueue* getSend();
    ThQueue* getReceive();
    
    // Configuration
    bool getDebug() const;
    void setDebug(bool debug);
    std::string getServerIp() const;
    int getServerPort() const;
    
    // Statistics
    struct Statistics {
        size_t bytesSent{0};
        size_t bytesReceived{0};
        size_t packetsSent{0};
        size_t packetsReceived{0};
        size_t sendErrors{0};
        size_t receiveErrors{0};
        std::chrono::steady_clock::time_point lastSendTime;
        std::chrono::steady_clock::time_point lastReceiveTime;
    };
    const Statistics& getStats() const;
    void resetStats();
    
private:
    std::string _ip;
    int _port;
    int _socket;
    bool _initialized;
    bool _debug;
    ConnectionState _state;
    
    sockaddr_in _serverAddr;
    mutable std::mutex _socketMutex;
    mutable std::mutex _statsMutex;
    
    ThQueue* _send;
    ThQueue* _recive;
    
    Statistics _stats;
    
    bool setSocketOptions();
    void updateState(ConnectionState newState);
    void recordSend(size_t bytes, bool success);
    void recordReceive(size_t bytes, bool success);
};