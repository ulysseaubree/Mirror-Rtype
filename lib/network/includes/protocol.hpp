#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>

namespace rtype::net {

    enum class Command : uint8_t {
        CONNECT, SERVER_FULL, LOBBY_JOIN, LOBBY_CREATE, 
        GAME_START, INPUT, WORLD_STATE, DISCONNECT
    };

    #pragma pack(push, 1)
    struct PacketHeader {
        uint32_t magic = 0x52545950; // "RTYP"
        uint16_t version = 1;
        uint32_t sequence;           
        Command command;
        uint16_t payloadSize;
    };

    struct EntityState {
        uint32_t id;
        float x, y;
        float vx, vy;
        uint8_t type; 
        int32_t health;
    };
    #pragma pack(pop)

    // Déclarations des fonctions du protocole
    namespace protocol {
        PacketHeader createHeader(Command cmd, uint16_t payloadSize, uint32_t sequence);
        std::vector<uint8_t> encodeConnectionRequest(const std::string& username);
        bool decodeListLobbiesRequest(const uint8_t *payload, size_t length);
        bool decodeCreateLobbyRequest(const uint8_t *payload, size_t length, std::string &lobbyName);
        bool decodeInput(const uint8_t *payload, size_t length, int &inputId);
        bool decodeEntityState(const uint8_t *payload, size_t length, EntityState &state);
    }

} // namespace rtype::net
