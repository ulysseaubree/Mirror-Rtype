#pragma once

    #include "../network/includes/udpClient.hpp"
    #include "../network/includes/protocol.hpp"
    #include "../network/includes/thread_utils.hpp"
    #include <SFML/Graphics.hpp>
    #include <csignal>
    #include <atomic>
    #include <thread>
    #include <chrono>
    #include <iostream>
    #include <vector>
    #include <algorithm>
    #include <random>
    #include <cstdlib>
    #include <sstream>
    #include <limits>
    #include <optional>
    #include <string>
    #include <functional>
    // Containers for tracking networked entities and their states.
    #include <unordered_map>
    #include <unordered_set>
    #include <memory>
    #include <cmath>

static std::atomic_bool g_running{true};

struct LobbyInfo {
    uint32_t id = 0;
    std::string name;
    uint16_t players = 0;
    uint16_t maxPlayers = 4;
    bool inGame = false;
};

struct LobbyScreenActions {
    // À brancher sur ton protocole réseau :
    std::function<void()> requestList;                            // LIST_LOBBIES
    std::function<void(const std::string&)> requestCreate;        // CREATE_LOBBY(name)
    std::function<void(uint32_t)> requestJoin;                    // JOIN_LOBBY(id)
    std::function<void(uint32_t)> requestStart;                   // START_GAME(id) optionnel
};

class LobbyScreen {
public:
    LobbyScreen(sf::RenderWindow& win, sf::Font& font, LobbyScreenActions actions);

    // Appelé quand tu reçois une réponse réseau listant les lobbies
    void setLobbies(std::vector<LobbyInfo> lobbies);

    // Retourne true si l'écran veut quitter (ex: Esc / close)
    bool update(float dt);

    // Retourne un lobby "joint" si tu veux changer d’état dans ton jeu
    std::optional<uint32_t> joinedLobby() const { return _joinedLobby; }

private:
    struct Button {
        sf::RectangleShape box;
        sf::Text label;
        bool hovered = false;
        bool pressed = false;
    };

    void draw();
    void handleEvent(const sf::Event& ev);

    void layout();
    bool buttonHit(const Button& b, sf::Vector2f mouse) const;
    void setButtonStyle(Button& b);
    void setStatus(const std::string& s);

private:
    sf::RenderWindow& _win;
    sf::Font& _font;
    LobbyScreenActions _actions;

    std::vector<LobbyInfo> _lobbies;
    int _selectedIndex = -1;

    // UI
    sf::Text _title;
    sf::Text _status;
    sf::RectangleShape _panel;
    sf::RectangleShape _inputBox;
    sf::Text _inputText;
    std::string _input;

    Button _btnRefresh;
    Button _btnCreate;
    Button _btnJoin;
    Button _btnStart;

    // liste
    sf::RectangleShape _listBox;
    sf::Text _listHeader;
    std::vector<sf::Text> _listLines;

    bool _quit = false;
    std::optional<uint32_t> _joinedLobby;
};

int wait_to_join(sf::RenderWindow *window);