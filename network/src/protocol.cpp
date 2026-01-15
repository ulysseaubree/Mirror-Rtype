/*
** EPITECH PROJECT, 2026
** r-type
** File description:
** Protocol encoding/decoding implementations
**
*/

#include "../includes/protocol.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>

namespace protocol {

std::vector<uint8_t> encodeHeader(Opcode op, uint16_t length)
{
    std::vector<uint8_t> buf(sizeof(PacketHeader));
    PacketHeader hdr;
    hdr.opcode = static_cast<uint8_t>(op);
    hdr.version = PROTOCOL_VERSION;
    hdr.length = htons(length);
    std::memcpy(buf.data(), &hdr, sizeof(PacketHeader));
    return buf;
}

bool decodeHeader(const uint8_t *data, size_t size, PacketHeader &header)
{
    if (!data || size < sizeof(PacketHeader)) {
        return false;
    }
    std::memcpy(&header, data, sizeof(PacketHeader));
    header.length = ntohs(header.length);
    return true;
}

std::string encodeHello()
{
    auto hdr = encodeHeader(Opcode::HELLO, 0);
    return std::string(reinterpret_cast<const char *>(hdr.data()), hdr.size());
}

bool decodeHello(const uint8_t *payload, size_t length)
{
    (void)payload;
    return length == 0;
}

std::string encodeWelcome(uint32_t playerId)
{
    uint16_t payloadLen = sizeof(uint32_t);
    auto hdr = encodeHeader(Opcode::WELCOME, payloadLen);
    uint32_t pidNet = htonl(playerId);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    packet.append(reinterpret_cast<const char *>(&pidNet), sizeof(uint32_t));
    return packet;
}

bool decodeWelcome(const uint8_t *payload, size_t length, uint32_t &playerId)
{
    if (length != sizeof(uint32_t)) return false;
    uint32_t pidNet;
    std::memcpy(&pidNet, payload, sizeof(uint32_t));
    playerId = ntohl(pidNet);
    return true;
}

std::string encodeInput(uint8_t direction, bool fire)
{
    uint8_t packed = (direction & 0x0F) | (fire ? 0x10 : 0x00);
    uint16_t payloadLen = sizeof(uint8_t);
    auto hdr = encodeHeader(Opcode::INPUT, payloadLen);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    packet.push_back(static_cast<char>(packed));
    return packet;
}

bool decodeInput(const uint8_t *payload, size_t length, uint8_t &direction, bool &fire)
{
    if (length != sizeof(uint8_t)) return false;
    uint8_t packed = payload[0];
    direction = packed & 0x0F;
    fire = (packed & 0x10) != 0;
    return true;
}

std::string encodeAck(uint32_t msgId)
{
    uint16_t payloadLen = sizeof(uint32_t);
    auto hdr = encodeHeader(Opcode::ACK, payloadLen);
    uint32_t midNet = htonl(msgId);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    packet.append(reinterpret_cast<const char *>(&midNet), sizeof(uint32_t));
    return packet;
}

bool decodeAck(const uint8_t *payload, size_t length, uint32_t &msgId)
{
    if (length != sizeof(uint32_t)) return false;
    uint32_t midNet;
    std::memcpy(&midNet, payload, sizeof(uint32_t));
    msgId = ntohl(midNet);
    return true;
}

std::string encodeScoreboard(const std::vector<ScoreEntry> &scores)
{
    uint16_t payloadLen = static_cast<uint16_t>(sizeof(uint16_t) + scores.size() * (sizeof(uint32_t) * 2 + sizeof(float)));
    auto hdr = encodeHeader(Opcode::SCOREBOARD, payloadLen);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    uint16_t count = htons(static_cast<uint16_t>(scores.size()));
    packet.append(reinterpret_cast<const char *>(&count), sizeof(uint16_t));
    for (const auto &entry : scores) {
        uint32_t idNet = htonl(entry.playerId);
        uint32_t scoreNet = htonl(entry.score);
        uint32_t timeBits;
        static_assert(sizeof(float) == sizeof(uint32_t), "Unexpected float size");
        std::memcpy(&timeBits, &entry.timeSurvived, sizeof(float));
        timeBits = htonl(timeBits);
        packet.append(reinterpret_cast<const char *>(&idNet), sizeof(uint32_t));
        packet.append(reinterpret_cast<const char *>(&scoreNet), sizeof(uint32_t));
        packet.append(reinterpret_cast<const char *>(&timeBits), sizeof(uint32_t));
    }
    return packet;
}

bool decodeScoreboard(const uint8_t *payload, size_t length, std::vector<ScoreEntry> &scores)
{
    if (length < sizeof(uint16_t)) return false;
    uint16_t countNet;
    std::memcpy(&countNet, payload, sizeof(uint16_t));
    size_t count = ntohs(countNet);
    size_t expectedLen = sizeof(uint16_t) + count * (sizeof(uint32_t) * 2 + sizeof(float));
    if (length != expectedLen) return false;
    scores.clear();
    const uint8_t *ptr = payload + sizeof(uint16_t);
    for (size_t i = 0; i < count; ++i) {
        ScoreEntry entry;
        uint32_t idNet, scoreNet, timeNet;
        std::memcpy(&idNet, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        std::memcpy(&scoreNet, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        std::memcpy(&timeNet, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        entry.playerId = ntohl(idNet);
        entry.score = ntohl(scoreNet);
        timeNet = ntohl(timeNet);
        std::memcpy(&entry.timeSurvived, &timeNet, sizeof(float));
        scores.push_back(entry);
    }
    return true;
}

std::string encodeListLobbiesRequest()
{
    auto hdr = encodeHeader(Opcode::LIST_LOBBIES, 0);
    return std::string(reinterpret_cast<const char *>(hdr.data()), hdr.size());
}

std::string encodeListLobbiesResponse(const std::vector<std::string> &names)
{
    size_t payloadSize = sizeof(uint16_t);
    for (const auto &n : names) payloadSize += 1 + n.size();
    if (payloadSize > UINT16_MAX) payloadSize = UINT16_MAX;
    auto hdr = encodeHeader(Opcode::LIST_LOBBIES, static_cast<uint16_t>(payloadSize));
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    uint16_t count = htons(static_cast<uint16_t>(names.size()));
    packet.append(reinterpret_cast<const char *>(&count), sizeof(uint16_t));
    for (const auto &n : names) {
        uint8_t len = static_cast<uint8_t>(std::min<size_t>(n.size(), 255));
        packet.push_back(static_cast<char>(len));
        packet.append(n.c_str(), len);
    }
    return packet;
}

bool decodeListLobbiesRequest(const uint8_t *payload, size_t length)
{
    return length == 0;
}

bool decodeListLobbiesResponse(const uint8_t *payload, size_t length, std::vector<std::string> &names)
{
    if (length < sizeof(uint16_t)) return false;
    uint16_t countNet;
    std::memcpy(&countNet, payload, sizeof(uint16_t));
    size_t count = ntohs(countNet);
    names.clear();
    const uint8_t *ptr = payload + sizeof(uint16_t);
    size_t remaining = length - sizeof(uint16_t);
    for (size_t i = 0; i < count; ++i) {
        if (remaining < 1) return false;
        uint8_t len = *ptr++;
        remaining--;
        if (remaining < len) return false;
        names.emplace_back(reinterpret_cast<const char *>(ptr), len);
        ptr += len;
        remaining -= len;
    }
    return true;
}

std::string encodeCreateLobbyRequest(const std::string &name)
{
    uint8_t len = static_cast<uint8_t>(std::min<size_t>(name.size(), 255));
    uint16_t payloadLen = sizeof(uint8_t) + len;
    auto hdr = encodeHeader(Opcode::CREATE_LOBBY, payloadLen);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    packet.push_back(static_cast<char>(len));
    packet.append(name.c_str(), len);
    return packet;
}

std::string encodeCreateLobbyResponse(uint32_t lobbyId)
{
    uint16_t payloadLen = sizeof(uint32_t);
    auto hdr = encodeHeader(Opcode::CREATE_LOBBY, payloadLen);
    uint32_t idNet = htonl(lobbyId);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    packet.append(reinterpret_cast<const char *>(&idNet), sizeof(uint32_t));
    return packet;
}

bool decodeCreateLobbyRequest(const uint8_t *payload, size_t length, std::string &name)
{
    if (length < 1) return false;
    uint8_t len = payload[0];
    if (length < 1 + len) return false;
    name.assign(reinterpret_cast<const char *>(payload + 1), len);
    return true;
}

bool decodeCreateLobbyResponse(const uint8_t *payload, size_t length, uint32_t &lobbyId)
{
    if (length != sizeof(uint32_t)) return false;
    uint32_t idNet;
    std::memcpy(&idNet, payload, sizeof(uint32_t));
    lobbyId = ntohl(idNet);
    return true;
}

std::string encodeJoinLobbyRequest(uint32_t lobbyId)
{
    uint16_t payloadLen = sizeof(uint32_t);
    auto hdr = encodeHeader(Opcode::JOIN_LOBBY, payloadLen);
    uint32_t idNet = htonl(lobbyId);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    packet.append(reinterpret_cast<const char *>(&idNet), sizeof(uint32_t));
    return packet;
}

std::string encodeJoinLobbyResponse(bool success)
{
    uint16_t payloadLen = sizeof(uint8_t);
    auto hdr = encodeHeader(Opcode::JOIN_LOBBY, payloadLen);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    packet.push_back(static_cast<char>(success ? 1 : 0));
    return packet;
}

bool decodeJoinLobbyRequest(const uint8_t *payload, size_t length, uint32_t &lobbyId)
{
    if (length != sizeof(uint32_t)) return false;
    uint32_t idNet;
    std::memcpy(&idNet, payload, sizeof(uint32_t));
    lobbyId = ntohl(idNet);
    return true;
}

bool decodeJoinLobbyResponse(const uint8_t *payload, size_t length, bool &success)
{
    if (length != sizeof(uint8_t)) return false;
    success = payload[0] != 0;
    return true;
}

std::string encodeStartGameRequest()
{
    auto hdr = encodeHeader(Opcode::START_GAME, 0);
    return std::string(reinterpret_cast<const char *>(hdr.data()), hdr.size());
}

bool decodeStartGameRequest(const uint8_t *payload, size_t length)
{
    (void)payload;
    return length == 0;
}

std::string encodeLobbyUpdate(const std::string &info)
{
    uint16_t len = static_cast<uint16_t>(std::min<size_t>(info.size(), UINT16_MAX));
    auto hdr = encodeHeader(Opcode::LOBBY_UPDATE, len);
    std::string packet(reinterpret_cast<const char *>(hdr.data()), hdr.size());
    packet.append(info.c_str(), len);
    return packet;
}

bool decodeLobbyUpdate(const uint8_t *payload, size_t length, std::string &info)
{
    info.assign(reinterpret_cast<const char *>(payload), length);
    return true;
}

} // namespace protocol