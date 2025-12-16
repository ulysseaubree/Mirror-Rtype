## RFC-RTYPE-UDP-01: Real-Time Game Protocol with Lightweight Reliability Layer (ACK/NACK)

> Version 1.0 — Draft

---

### Introduction

> This document specifies the binary UDP protocol used by the R-Type multiplayer game.
> The protocol is designed for:
- Low latency, suitable for real-time gameplay.
- Partial reliability using a lightweight ACK/NACK layer for critical messages.
- Compact binary encoding to minimize bandwidth.
- Extensibility to support additional gameplay commands.
> UDP is used for all in-game messages as required by the project.
> A minimal reliability layer is implemented at the application level to compensate for UDP packet loss.

---

### Terminology

- Client — the player-side application.
- Server — authoritative game host.
- Datagram — a UDP packet.
- Opcode — 1-byte command identifier.
- SeqID — 16-bit sequence number for reliability.
- ACK — acknowledgment of receipt.
- NACK — negative acknowledgment (packet rejected or corrupt).
- Critical message — must be delivered reliably (login, entity spawn, death, disconnect…).
- Non-critical message — may be dropped (movement updates, position sync).

---

### General Packet Format

> All packets share the following header:
```
0       1       2       3       4 ...
+-------+-------+------------------------+
| OP    | FLAGS | SEQ_ID (2 bytes)       |
+-------+-------+------------------------+
| PAYLOAD (variable, LEN inferred)       |
+----------------------------------------+

```

#### Fields

| Field     | Size                 | Description                               |
| --------- | -------------------- | ----------------------------------------- |
| `OP`      | 1 byte               | Opcode identifying the message type       |
| `FLAGS`   | 1 byte               | Bitfield: reliability, ACK requests, etc. |
| `SEQ_ID`  | 2 bytes (big-endian) | Sequence number used for ACK/NACK         |
| `PAYLOAD` | variable             | Message-specific data                     |

#### Flags

```
Bit:  7 6 5 4 3 2 1 0
      | | | | | | | +-- Requires ACK (1 = yes)
      | | | | | | +---- Reserved
      +--------------- Future use
```

> For now only bit 0 is used.

---

### Reliability Layer (ACK/NACK)

> UDP does not guarantee delivery.
For critical messages, the protocol implements a lightweight mechanism:

#### ACK Packet

```
OP = 0x01
Payload:
+--------------+
| SEQ_ID (2B)  |
+--------------+
```

> Sent by receiver when:
- packet is valid
- packet had the “Requires ACK” flag set.

#### NACK Packet

```
OP = 0x02
Payload:
+--------------+
| SEQ_ID (2B)  |
+--------------+
```

> Sent when:
- packet is malformed
- invalid opcode

#### Retransmission (Client and Server)

> If a message requires an ACK:
- Sender stores the packet in a reliable queue.
- If no ACK is received after 100 ms, the packet is resent.
- After 5 retries, the sender considers the peer disconnected.

> Non-critical messages are never retransmitted.

---

### Sequence Numbering

- SEQ_ID is a 16-bit unsigned integer.
- It increments per sent message, including non-critical ones.
- Wrap-around is allowed (0 → 65535 → 0).
- ACK/NACK refer to the corresponding SEQ_ID.

---

### Opcodes Summary

| Opcode | Name           | Direction | Reliable | Description                          |
| ------ | -------------- | --------- | -------- | ------------------------------------ |
| `0x01` | ACK            | both      | —        | Acknowledges a received packet       |
| `0x02` | NACK           | both      | —        | Rejects a packet                     |
| `0x10` | HELLO          | C → S     | ✔️       | Client requests connection to server |
| `0x11` | WELCOME        | S → C     | ✔️       | Server accepts client                |
| `0x12` | PLAYER_LEAVE   | C → S     | ✔️       | Client disconnects                   |
| `0x13` | PLAYER_LEFT    | S → C     | ✔️       | Notify others that a player left     |
| `0x20` | INPUT          | C → S     | ❌        | Player movement/shoot input          |
| `0x30` | ENTITY_SPAWN   | S → C     | ✔️       | Create new entity                    |
| `0x31` | ENTITY_UPDATE  | S → C     | ❌        | Position/velocity update             |
| `0x32` | ENTITY_DESTROY | S → C     | ✔️       | Remove entity from world             |
| `0x40` | HIT_EVENT      | S → C     | ✔️       | A player or entity has been damaged  |
| `0x50` | HEARTBEAT      | both      | ❌        | Keep connection alive                |

---

### Packet Definitions

#### Client → Server: HELLO (0x10)

> Purpose: request to join the game.
> Flags: Requires ACK = 1.
> Payload:
```
+--------------+------------------+
| VERSION (1B) | PLAYER_NAME (N)  |
+--------------+------------------+
```

#### Server → Client: WELCOME (0x11)

> Reliable.
> Payload:
```
+----------------+----------------+
| PLAYER_ID (1B) | SEED (4B)     |
+----------------+----------------+
```

#### Player Input (0x20)

> Non-reliable (real-time updates).
> Payload:
```
+----------------+
| INPUT_FLAGS(1) |  bitfield: 
|                | 0x01 = Up
|                | 0x02 = Down
|                | 0x04 = Left
|                | 0x08 = Right
|                | 0x10 = Shoot
+----------------+
```

#### Entity Spawn (0x30)

> Reliable.
> Payload:
```
+----------------+-------------+-------------+
| ENTITY_ID (2B) | TYPE (1B)  | X (2B) Y(2B) |
+----------------+-------------+-------------+
```

#### Entity Update (0x31)

> Non-reliable.
> Payload:
```
+----------------+-----------+-----------+
| ENTITY_ID (2B) | X (2B)   | Y (2B)    |
+----------------+-----------+-----------+
```

#### Entity Destroy (0x32)

> Reliable.
> Payload:
```
+----------------+
| ENTITY_ID (2B) |
+----------------+
```

---

### Error Handling

> A packet MUST be NACKed when:
- OP is unknown
- packet is too small
- malformed payload
- invalid values (e.g., out of range entity type)

> On NACK:
- Sender MAY resend the message.
- Too many NACKs → connection dropped.

---

### Security Considerations

> To comply with safety requirements from the subject:
- All packet sizes MUST be validated before reading.
- Payloads MUST NOT allocate unbounded memory.
- Server MUST ignore packets exceeding a maximum size (e.g., 256 bytes).
- Sequence numbers prevent trivial replay issues.
- Player inputs must be clamped to valid ranges.

---

### Example Datagram (hex)

> Example EntityUpdate:
OP      = 0x31
FLAGS   = 0x00
SEQ_ID  = 0x12 0x5A
ENTITY  = 0x03 0x10
X       = 0x01 0x90
Y       = 0x00 0x50

> Hex:
31 00 12 5A 03 10 01 90 00 50

---

### Future Extensions

- Checksum field
- Delta compression of entity states
- Bit-packed coordinates
- Client-side interpolation metadata

---