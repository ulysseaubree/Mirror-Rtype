#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>

class ThQueue;

struct ClientInfo {
    sockaddr_in address;
    std::string ip;
    int port;
    std::chrono::steady_clock::time_point lastSeen;
    
    ClientInfo() = default; 
    ClientInfo(const sockaddr_in& addr);
    std::string getKey() const;
};

class UdpServer {
public:
    using MessageHandler = std::function<void(const std::string&, const ClientInfo&)>;
    
    UdpServer(int port, bool debug = false);
    ~UdpServer();
    
    // Disable copy
    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;
    
    bool initSocket();
    bool sendData(const std::string& data, const ClientInfo& client);
    bool broadcast(const std::string& data);
    std::string receiveData(int bufferSize = 1024);
    void disconnect();
    
    // Queue management
    void setSend(const std::string& data, const std::string& clientKey);
    void setReceive(const std::string& data);
    ThQueue* getSend();
    ThQueue* getReceive();
    
    // Client management
    void updateClient(const ClientInfo& client);
    void removeClient(const std::string& clientKey);
    std::vector<ClientInfo> getActiveClients(int timeoutSeconds = 30);
    size_t getClientCount() const;
    
    // Configuration
    bool getDebug() const;
    void setDebug(bool debug);
    int getPort() const;
    
    // Last received client info
    const ClientInfo* getLastClient() const { return _lastClient.get(); }
    
private:
    int _port;
    int _socket;
    bool _initialized;
    bool _debug;
    
    sockaddr_in _serverAddr;
    mutable std::mutex _socketMutex;
    mutable std::mutex _clientMutex;
    
    ThQueue* _send;
    ThQueue* _recive;
    
    // Track connected clients
    bool setSocketOptions();
    std::unordered_map<std::string, ClientInfo> _clients;
    std::unique_ptr<ClientInfo> _lastClient;
    
};