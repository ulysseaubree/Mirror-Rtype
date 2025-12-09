/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** simple UDP server using UdpServer
*/

#include "../network/includes/udpServer.hpp"

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

static std::atomic_bool g_running{true};

static void signal_handler(int)
{
    g_running = false;
}

std::string processMessages(const std::string& msg, int id)
{
    std::string resp = std::to_string(id) + " ";
    size_t pos = msg.find(' ');
    std::string type = msg.substr(0, pos);

    if (type == "HELLO") {
        resp += "WELCOME\r\n";
    } else if (type == "PING") {
        resp += "PONG\r\n";
    } else {
        resp += "ECHO: " + msg + "\r\n";
    }
    return resp;
}

int main(void)
{
    std::signal(SIGINT, signal_handler);
    int port = 4242;
    int id = 0;
    std::vector<std::pair<int, std::string>> line;
    UdpServer server(port, true);
    if (!server.initSocket()) {
        std::cerr << "Failed to init UDP server on port " << port << std::endl;
        return 84;
    }
    std::cout << "[Server] Ready on port " << server.getPort()
              << " (press Ctrl+C to quit)" << std::endl;
    while (g_running) {
        std::string msg = server.receiveData(1024);

        if (!msg.empty()) {
            const ClientInfo *client = server.getLastClient();
            if (client != nullptr) {
                std::cout << "[Server] Received from " << client->getKey()
                          << " : " << msg << std::endl;
                size_t pos = msg.find(' ');
                std::string type = msg.substr(0, pos);
                if (type == "ACK") {
                    std::string idStr = msg.substr(pos + 1);
                    int ackId = std::atoi(idStr.c_str());
                    std::cout << "[Server] ACK received for message ID " << ackId << std::endl;

                    line.erase(
                        std::remove_if(
                            line.begin(),
                            line.end(),
                            [ackId](const std::pair<int, std::string>& p) {
                                return p.first == ackId;
                            }
                        ),
                        line.end()
                    );
                    std::cout << "[Server] " + std::to_string(ackId) + " erased" << std::endl;
                    std::cout << "[Server] Pending messages count: " << line.size() << std::endl;
                } else {
                    id++;
                    line.push_back(std::make_pair(id, processMessages(msg, id)));
                }
            }
        }

        for (const auto& p : line) {
            std::cout << "[Server] Sending to "
                        << server.getLastClient()->getKey()
                        << " : " << p.second;
            if (!server.sendData(p.second, *server.getLastClient())) {
                std::cerr << "[Client] Error while sending data" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "\n[Server] Shutting down..." << std::endl;
    server.disconnect();
    return 0;
}
