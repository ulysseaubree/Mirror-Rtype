#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class ThQueue;

class UdpClient {
public:
    UdpClient(const std::string& ip, int port, bool debug = false);
    ~UdpClient();

    bool initSocket();
    void disconnect();

    bool sendData(const std::string& data);
    std::string receiveData(int bufferSize = 1024);

    std::string waiter(std::string expect, std::string to_send, int i = 0);

    void setSend(std::string s);
    void setReceive(std::string r);
    void setRun(bool rn);

    ThQueue *getSend();
    ThQueue *getReceive();
    bool getRun();
    bool getDebug();

private:
    std::string _ip;
    int _port;
    bool _debug;

    int _socket;
    bool _initialized;

    struct sockaddr_in _serverAddr;
    std::mutex _socketMutex;

    ThQueue *_recive;
    ThQueue *_send;
};

