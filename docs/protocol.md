# R-Type Network Protocol (Defense 1)

The prototype uses a UDP text protocol where each message is terminated by `\r\n`.
Every message starts with a keyword describing its type, followed by space-separated parameters.

## Message flow

- **Client → Server**: sends player intent (connect, inputs).
- **Server → Client**: sends the authoritative world state (positions, health) and acknowledgements.

## Messages

### HELLO (Client → Server)

```text
HELLO\r\n
```

Announces a new client connection. The server answers with a `WELCOME` message containing the assigned entity id.

### WELCOME (Server → Client)

```text
WELCOME <entityId>\r\n
```

Confirms the creation or reconnection of a player and assigns entity `<entityId>` inside the server ECS.

### INPUT (Client → Server)

```text
INPUT <direction> <fire>\r\n
```

Carries player commands:

- `<direction>`: integer `1` to `9` using the numpad layout (1=down-left, 2=down, 3=down-right, 4=left, 5=idle, 6=right, 7=up-left, 8=up, 9=up-right).
- `<fire>`: `0` for no shot, `1` to fire.

The server updates the `PlayerInput` component and will broadcast the resolved position via `STATE`.

### STATE (Server → Client)

```text
STATE <msgId> <tick> <entity> <x> <y> <health>\r\n
```

Describes the state of one entity during server tick `<tick>`:

- `<msgId>`: unique message id (incremental). Clients must send an `ACK` for each received id.
- `<tick>`: server tick counter, useful for debugging.
- `<entity>`: entity identifier (matches the id received in `WELCOME`).
- `<x> <y>`: authoritative position.
- `<health>`: remaining hit points.

In this MVP each player only receives their own `STATE`, but the format scales to multiple entities.

### ACK (Client → Server)

```text
ACK <msgId>\r\n
```

Acknowledges a `STATE` so the server can drop it from its resend queue.

### PING / PONG (Client ↔ Server)

Optional connectivity probes:

```text
PING\r\n   // Client or server
PONG\r\n   // Reply
```

## Error handling

The server silently ignores malformed or unknown messages. Clients should handle timeouts and fall back gracefully if no response arrives. No authentication is defined for this MVP.
