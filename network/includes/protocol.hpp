/*
** EPITECH PROJECT, 2026
** r-type
** File description:
** Protocol definitions for the binary network layer
**
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace protocol {

enum class Opcode : uint8_t {
    HELLO        = 1,  //!< Client → server: handshake initiation
    WELCOME      = 2,  //!< Server → client: handshake response, includes client id
    INPUT        = 3,  //!< Client → server: directional input and fire flag
    STATE        = 4,  //!< Server → client: snapshot of the world
    ACK          = 5,  //!< Client → server: acknowledges receipt of a STATE packet
    SCOREBOARD   = 6,  //!< Server → client: scoreboard sent at end of game
    LIST_LOBBIES = 7,  //!< Client ↔ server: list available lobbies
    CREATE_LOBBY = 8,  //!< Client → server: create a new lobby
    JOIN_LOBBY   = 9,  //!< Client → server: join an existing lobby
    START_GAME   = 10, //!< Client → server: lobby host signals game start
    LOBBY_UPDATE = 11  //!< Server → client: lobby state update (players ready, etc.)
};

constexpr uint8_t PROTOCOL_VERSION = 1;

struct PacketHeader {
    uint8_t opcode;   //!< Operation code defined in protocol::Opcode
    uint8_t version;  //!< Protocol version (see PROTOCOL_VERSION)
    uint16_t length;  //!< Length of the payload (network byte order)
};

std::vector<uint8_t> encodeHeader(Opcode op, uint16_t length);

bool decodeHeader(const uint8_t *data, size_t size, PacketHeader &header);

std::string encodeHello();

std::string encodeWelcome(uint32_t playerId);

std::string encodeInput(uint8_t direction, bool fire);

std::string encodeAck(uint32_t msgId);

struct ScoreEntry {
    uint32_t playerId;
    uint32_t score;
    float timeSurvived;
};

std::string encodeScoreboard(const std::vector<ScoreEntry> &scores);

std::string encodeListLobbiesRequest();
std::string encodeListLobbiesResponse(const std::vector<std::string> &names);
std::string encodeCreateLobbyRequest(const std::string &name);
std::string encodeCreateLobbyResponse(uint32_t lobbyId);
std::string encodeJoinLobbyRequest(uint32_t lobbyId);
std::string encodeJoinLobbyResponse(bool success);
std::string encodeStartGameRequest();
std::string encodeLobbyUpdate(const std::string &info);

bool decodeHello(const uint8_t *payload, size_t length);
bool decodeWelcome(const uint8_t *payload, size_t length, uint32_t &playerId);
bool decodeInput(const uint8_t *payload, size_t length, uint8_t &direction, bool &fire);
bool decodeAck(const uint8_t *payload, size_t length, uint32_t &msgId);
bool decodeScoreboard(const uint8_t *payload, size_t length, std::vector<ScoreEntry> &scores);
// Decoders for lobby messages
bool decodeListLobbiesRequest(const uint8_t *payload, size_t length);
bool decodeListLobbiesResponse(const uint8_t *payload, size_t length, std::vector<std::string> &names);
bool decodeCreateLobbyRequest(const uint8_t *payload, size_t length, std::string &name);
bool decodeCreateLobbyResponse(const uint8_t *payload, size_t length, uint32_t &lobbyId);
bool decodeJoinLobbyRequest(const uint8_t *payload, size_t length, uint32_t &lobbyId);
bool decodeJoinLobbyResponse(const uint8_t *payload, size_t length, bool &success);
bool decodeStartGameRequest(const uint8_t *payload, size_t length);
bool decodeLobbyUpdate(const uint8_t *payload, size_t length, std::string &info);

} // namespace protocol