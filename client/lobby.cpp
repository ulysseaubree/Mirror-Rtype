/*
** EPITECH PROJECT, 2025
** r-type
** File description:
** Client lobby
**
*/

#include "client.hpp"
#include <algorithm>

static sf::Vector2f getMouse(sf::RenderWindow& w)
{
    auto p = sf::Mouse::getPosition(w);
    return sf::Vector2f{static_cast<float>(p.x), static_cast<float>(p.y)};
}

LobbyScreen::LobbyScreen(sf::RenderWindow& win, sf::Font& font, LobbyScreenActions actions)
    : _win(win), _font(font), _actions(std::move(actions))
{
    _title.setFont(_font);
    _title.setString("Lobbies");
    _title.setCharacterSize(32);
    _title.setPosition({24.f, 16.f});

    _status.setFont(_font);
    _status.setCharacterSize(16);
    _status.setFillColor(sf::Color(200, 200, 200));
    _status.setPosition({24.f, 56.f});
    setStatus("Refresh pour charger les lobbies.");

    _panel.setSize({760.f, 520.f});
    _panel.setPosition({20.f, 90.f});
    _panel.setFillColor(sf::Color(25, 25, 30));
    _panel.setOutlineThickness(2.f);
    _panel.setOutlineColor(sf::Color(70, 70, 90));

    _inputBox.setSize({360.f, 42.f});
    _inputBox.setFillColor(sf::Color(15, 15, 18));
    _inputBox.setOutlineThickness(2.f);
    _inputBox.setOutlineColor(sf::Color(90, 90, 120));

    _inputText.setFont(_font);
    _inputText.setCharacterSize(18);
    _inputText.setFillColor(sf::Color::White);

    _listBox.setSize({720.f, 320.f});
    _listBox.setFillColor(sf::Color(18, 18, 22));
    _listBox.setOutlineThickness(2.f);
    _listBox.setOutlineColor(sf::Color(80, 80, 100));

    _listHeader.setFont(_font);
    _listHeader.setCharacterSize(16);
    _listHeader.setFillColor(sf::Color(200, 200, 220));
    _listHeader.setString("ID   NAME                        PLAYERS   STATE");

    auto makeButton = [&](Button& b, const std::string& text) {
        b.box.setSize({160.f, 44.f});
        b.box.setFillColor(sf::Color(40, 40, 55));
        b.box.setOutlineThickness(2.f);
        b.box.setOutlineColor(sf::Color(90, 90, 120));

        b.label.setFont(_font);
        b.label.setString(text);
        b.label.setCharacterSize(18);
        b.label.setFillColor(sf::Color::White);
    };

    makeButton(_btnRefresh, "Refresh");
    makeButton(_btnCreate,  "Create");
    makeButton(_btnJoin,    "Join");
    makeButton(_btnStart,   "Start");

    layout();
}

void LobbyScreen::layout()
{
    _inputBox.setPosition({40.f, 470.f});
    _inputText.setPosition(_inputBox.getPosition() + sf::Vector2f{10.f, 8.f});

    _listBox.setPosition({40.f, 130.f});
    _listHeader.setPosition(_listBox.getPosition() + sf::Vector2f{12.f, 8.f});

    _btnRefresh.box.setPosition({420.f, 470.f});
    _btnCreate.box.setPosition({600.f, 470.f});
    _btnJoin.box.setPosition({420.f, 530.f});
    _btnStart.box.setPosition({600.f, 530.f});

    auto centerLabel = [](Button& b) {
        auto bounds = b.label.getLocalBounds();
        b.label.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                           bounds.position.y + bounds.size.y / 2.f});
        b.label.setPosition(b.box.getPosition() + b.box.getSize() / 2.f);
    };
    centerLabel(_btnRefresh);
    centerLabel(_btnCreate);
    centerLabel(_btnJoin);
    centerLabel(_btnStart);
}

void LobbyScreen::setStatus(const std::string& s)
{
    _status.setString("Status: " + s);
}

void LobbyScreen::setLobbies(std::vector<LobbyInfo> lobbies)
{
    _lobbies = std::move(lobbies);
    _selectedIndex = (_lobbies.empty() ? -1 : std::min(_selectedIndex, (int)_lobbies.size() - 1));

    _listLines.clear();
    _listLines.reserve(_lobbies.size());

    float y = _listBox.getPosition().y + 40.f;
    for (size_t i = 0; i < _lobbies.size(); ++i) {
        const auto& l = _lobbies[i];

        std::string state = l.inGame ? "IN_GAME" : "WAITING";
        std::string line =
            std::to_string(l.id) + "   " +
            l.name + std::string(std::max(1, 28 - (int)l.name.size()), ' ') +
            std::to_string(l.players) + "/" + std::to_string(l.maxPlayers) + "     " +
            state;

        sf::Text t;
        t.setFont(_font);
        t.setCharacterSize(16);
        t.setString(line);
        t.setPosition({_listBox.getPosition().x + 12.f, y});
        t.setFillColor(sf::Color(220, 220, 220));

        _listLines.push_back(std::move(t));
        y += 24.f;
    }

    setStatus("Lobbies recupérés: " + std::to_string(_lobbies.size()));
}

bool LobbyScreen::buttonHit(const Button& b, sf::Vector2f mouse) const
{
    return b.box.getGlobalBounds().contains(mouse);
}

void LobbyScreen::setButtonStyle(Button& b)
{
    if (b.pressed) {
        b.box.setFillColor(sf::Color(70, 70, 95));
    } else if (b.hovered) {
        b.box.setFillColor(sf::Color(55, 55, 75));
    } else {
        b.box.setFillColor(sf::Color(40, 40, 55));
    }
}

void LobbyScreen::handleEvent(const sf::Event& ev)
{
    if (ev.is<sf::Event::Closed>()) {
        _win.close();
        _quit = true;
        return;
    }

    if (const auto* key = ev.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            _quit = true;
            return;
        }
        if (key->code == sf::Keyboard::Key::Enter) {
            // enter -> create
            if (!_input.empty() && _actions.requestCreate) {
                _actions.requestCreate(_input);
                setStatus("Create lobby: " + _input);
            }
        }
        if (key->code == sf::Keyboard::Key::Up) {
            if (_selectedIndex > 0) _selectedIndex--;
        }
        if (key->code == sf::Keyboard::Key::Down) {
            if (_selectedIndex + 1 < (int)_lobbies.size()) _selectedIndex++;
        }
    }

    if (const auto* txt = ev.getIf<sf::Event::TextEntered>()) {
        // SFML3 : txt->unicode
        const uint32_t u = txt->unicode;
        if (u == 8) { // backspace
            if (!_input.empty())
                _input.pop_back();
        } else if (u >= 32 && u < 127) {
            if (_input.size() < 20)
                _input.push_back(static_cast<char>(u));
        }
        _inputText.setString(_input.empty() ? "<lobby name>" : _input);
        _inputText.setFillColor(_input.empty() ? sf::Color(130,130,130) : sf::Color::White);
    }

    if (const auto* mouse = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouse->button == sf::Mouse::Button::Left) {
            sf::Vector2f m = getMouse(_win);

            // click list
            for (int i = 0; i < (int)_listLines.size(); ++i) {
                if (_listLines[i].getGlobalBounds().contains(m)) {
                    _selectedIndex = i;
                    setStatus("Selected lobby id=" + std::to_string(_lobbies[i].id));
                }
            }

            // click buttons
            _btnRefresh.pressed = buttonHit(_btnRefresh, m);
            _btnCreate.pressed  = buttonHit(_btnCreate, m);
            _btnJoin.pressed    = buttonHit(_btnJoin, m);
            _btnStart.pressed   = buttonHit(_btnStart, m);
        }
    }

    if (const auto* mouse = ev.getIf<sf::Event::MouseButtonReleased>()) {
        if (mouse->button == sf::Mouse::Button::Left) {
            sf::Vector2f m = getMouse(_win);

            auto wasPressed = [&](Button& b) {
                bool click = b.pressed && buttonHit(b, m);
                b.pressed = false;
                return click;
            };

            if (wasPressed(_btnRefresh)) {
                if (_actions.requestList) _actions.requestList();
                setStatus("Refresh requested");
            }
            if (wasPressed(_btnCreate)) {
                if (_input.empty()) {
                    setStatus("Nom de lobby vide.");
                } else if (_actions.requestCreate) {
                    _actions.requestCreate(_input);
                    setStatus("Create requested: " + _input);
                }
            }
            if (wasPressed(_btnJoin)) {
                if (_selectedIndex < 0 || _selectedIndex >= (int)_lobbies.size()) {
                    setStatus("Aucun lobby selectionné.");
                } else if (_actions.requestJoin) {
                    uint32_t id = _lobbies[_selectedIndex].id;
                    _actions.requestJoin(id);
                    setStatus("Join requested: " + std::to_string(id));
                    _joinedLobby = id;
                }
            }
            if (wasPressed(_btnStart)) {
                if (_selectedIndex < 0 || _selectedIndex >= (int)_lobbies.size()) {
                    setStatus("Aucun lobby selectionné.");
                } else if (_actions.requestStart) {
                    uint32_t id = _lobbies[_selectedIndex].id;
                    _actions.requestStart(id);
                    setStatus("Start requested: " + std::to_string(id));
                }
            }
        }
    }
}

bool LobbyScreen::update(float /*dt*/)
{
    // hover states
    sf::Vector2f m = getMouse(_win);
    _btnRefresh.hovered = buttonHit(_btnRefresh, m);
    _btnCreate.hovered  = buttonHit(_btnCreate, m);
    _btnJoin.hovered    = buttonHit(_btnJoin, m);
    _btnStart.hovered   = buttonHit(_btnStart, m);

    setButtonStyle(_btnRefresh);
    setButtonStyle(_btnCreate);
    setButtonStyle(_btnJoin);
    setButtonStyle(_btnStart);

    // placeholder input display
    if (_inputText.getString().isEmpty()) {
        _inputText.setString("<lobby name>");
        _inputText.setFillColor(sf::Color(130,130,130));
    }

    // consume events
    while (auto ev = _win.pollEvent()) {
        handleEvent(*ev);
        if (_quit)
            return true;
    }

    draw();
    return _quit;
}

void LobbyScreen::draw()
{
    _win.clear(sf::Color(10, 10, 12));

    _win.draw(_title);
    _win.draw(_status);
    _win.draw(_panel);

    _win.draw(_listBox);
    _win.draw(_listHeader);

    for (int i = 0; i < (int)_listLines.size(); ++i) {
        if (i == _selectedIndex) {
            // highlight row
            sf::RectangleShape hl;
            hl.setPosition({_listBox.getPosition().x + 6.f, _listLines[i].getPosition().y - 2.f});
            hl.setSize({_listBox.getSize().x - 12.f, 22.f});
            hl.setFillColor(sf::Color(60, 60, 85));
            _win.draw(hl);
        }
        _win.draw(_listLines[i]);
    }

    _win.draw(_inputBox);
    _win.draw(_inputText);

    _win.draw(_btnRefresh.box); _win.draw(_btnRefresh.label);
    _win.draw(_btnCreate.box);  _win.draw(_btnCreate.label);
    _win.draw(_btnJoin.box);    _win.draw(_btnJoin.label);
    _win.draw(_btnStart.box);   _win.draw(_btnStart.label);

    _win.display();
}


int wait_to_join(sf::RenderWindow *window)
{
    LobbyScreenActions actions;
    actions.requestList = [&]() {
        client.sendData(protocol::encodeListLobbies());
    };
    actions.requestCreate = [&](const std::string& name) {
        client.sendData(protocol::encodeCreateLobby(name));
    };
    actions.requestJoin = [&](uint32_t lobbyId) {
        client.sendData(protocol::encodeJoinLobby(lobbyId));
    };
    actions.requestStart = [&](uint32_t lobbyId) {
        client.sendData(protocol::encodeStartGame(lobbyId));
    };
    LobbyScreen lobbyScreen(window, font, actions);
    actions.requestList();

    sf::Clock clock;
    while (window->isOpen() && g_running) {
        float dt = clock.restart().asSeconds();
        if (lobbyScreen.update(dt)) {
            return 84;
        }
        

        while (auto ev = window->pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                window->close();
                return 84;
            }
            if (const auto* key = ev->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) {
                    window->close();
                    return 84;
                }
            }
        }
    }
    return 0;
}