/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** simple UDP client using UdpClient
*/

#include "../includes/udpClient.hpp"

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

int main(int ac, char **av)
{
    std::signal(SIGINT, signal_handler);
    std::string ip = "127.0.0.1";
    int port = 4242;
    std::vector<int> numbers;

    if (ac >= 2)
        ip = av[1];
    if (ac >= 3)
        port = std::atoi(av[2]);

    bool debug = true;
    UdpClient client(ip, port, debug);

    if (!client.initSocket()) {
        std::cerr << "[Client] Failed to init UDP client to "
                  << ip << ":" << port << std::endl;
        return 84;
    }

    std::cout << "[Client] Connected to " << client.getServerIp()
              << ":" << client.getServerPort() << std::endl;
    std::cout << "[Client] Type messages to send, or /quit to exit." << std::endl;

    // Boucle principale : on lit l'input utilisateur,
    // on envoie au serveur, puis on lit les réponses.
    while (g_running) {
        std::string line;

        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl+D, pipe coupé, etc.)
            break;
        }

        if (line == "/quit") {
            break;
        }

        if (!line.empty()) {
            // On ajoute un \r\n pour être cohérent avec le serveur si besoin
            std::string toSend = line;
            if (toSend.size() < 2 || toSend.substr(toSend.size() - 2) != "\r\n")
                toSend += "\r\n";

            if (!client.sendData(toSend)) {
                std::cerr << "[Client] Error while sending data" << std::endl;
            }
        }

        // Petite fenêtre pour lire les messages reçus
        // (ACK du serveur, par exemple)
        auto start = std::chrono::steady_clock::now();
        while (g_running) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            ).count();

            if (elapsed > 500) // 500 ms d'écoute après chaque envoi
                break;

            std::string resp = client.receiveData(1024);
            if (!resp.empty()) {
                size_t pos = resp.find(' ');
                std::string id = resp.substr(0, pos);
                int idNum = std::atoi(id.c_str());

                client.sendData("ACK " + std::to_string(idNum) + "\r\n");
                if (std::find(numbers.begin(), numbers.end(), idNum) == numbers.end()) {
                    numbers.push_back(idNum);
                    std::cout << "[Client] Received: " << resp << std::endl;
                } else {
                    std::cout << "[Client] Duplicate message ID " << idNum << " ignored." << std::endl;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    std::cout << "\n[Client] Shutting down..." << std::endl;
    client.disconnect();
    return 0;
}
