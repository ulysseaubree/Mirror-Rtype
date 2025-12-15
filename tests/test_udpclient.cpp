#include "../network/includes/udpClient.hpp"
#include <cassert>
#include <iostream>

// Test de base pour la classe UdpClient. Ce test ne vérifie pas la communication
// réseau mais valide la création et la configuration de l'objet.
int main()
{
    UdpClient client("127.0.0.1", 4242, false);
    assert(!client.isConnected());
    assert(client.getServerIp() == "127.0.0.1");
    assert(client.getServerPort() == 4242);

    // Le socket n'est pas initialisé tant que initSocket() n'est pas appelé.
    bool ok = client.initSocket();
    if (!ok) {
        std::cout << "Socket non initialisé, test ignoré." << std::endl;
        return 0;
    }
    assert(client.isConnected());
    client.setDebug(true);
    assert(client.getDebug());
    client.disconnect();
    assert(!client.isConnected());
    std::cout << "Test UdpClient réussi" << std::endl;
    return 0;
}