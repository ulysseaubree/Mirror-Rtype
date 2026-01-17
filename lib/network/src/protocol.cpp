#include "protocol.hpp"
#include <iostream>

namespace rtype::net {
    namespace protocol {

        // Rend les types du parent (rtype::net) visibles explicitement ici
        using namespace rtype::net;

        PacketHeader createHeader(Command cmd, uint16_t payloadSize, uint32_t sequence) {
            PacketHeader header;
            header.magic = 0x52545950;
            header.version = 1;
            header.sequence = sequence;
            header.command = cmd;
            header.payloadSize = payloadSize;
            return header;
        }

        std::vector<uint8_t> encodeConnectionRequest(const std::string& username) {
            std::vector<uint8_t> buffer;
            uint8_t len = static_cast<uint8_t>(username.size());
            if (len > 32) len = 32;

            buffer.push_back(len);
            buffer.insert(buffer.end(), username.begin(), username.begin() + len);
            return buffer;
        }

        bool decodeListLobbiesRequest(const uint8_t *payload, size_t length)
        {
            (void)payload;
            (void)length;
            return true;
        }

        bool decodeCreateLobbyRequest(const uint8_t *payload, size_t length, std::string &lobbyName)
        {
            if (length < 1) return false;
            
            uint8_t nameLen = payload[0];
            
            if (length < 1 + static_cast<size_t>(nameLen)) return false;

            lobbyName.assign(reinterpret_cast<const char*>(payload + 1), nameLen);
            return true;
        }

        bool decodeInput(const uint8_t *payload, size_t length, int &inputId) {
            if (length < sizeof(int)) return false;
            std::memcpy(&inputId, payload, sizeof(int));
            return true;
        }

        bool decodeEntityState(const uint8_t *payload, size_t length, EntityState &state) {
            if (length < sizeof(EntityState)) return false;
            std::memcpy(&state, payload, sizeof(EntityState));
            return true;
        }

    } // namespace protocol
} // namespace rtype::net</document>
