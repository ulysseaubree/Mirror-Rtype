# R-Type Network Protocol (Defense 1)

Le prototype utilise un protocole UDP simple avec des messages textuels terminés par `\r\n`.
Chaque message commence par un mot‑clé identifiant son type, suivi de paramètres séparés par des espaces.

## Direction des messages

- **Client → Serveur** : envoi des intentions du joueur (connexion, entrées).  
- **Serveur → Client** : envoi de l'état autoritaire du monde (positions, santé) et retour d'information.

## Messages

### HELLO (Client → Serveur)

```text
HELLO\r\n
```

Annonce la connexion d'un client. Le serveur répond par un message `WELCOME` comportant l'identifiant attribué.

### WELCOME (Serveur → Client)

```text
WELCOME <entityId>\r\n
```

Confirme la création ou la reconnexion d'un joueur et lui assigne l'entité `<entityId>` dans l'ECS serveur.

### INPUT (Client → Serveur)

```text
INPUT <direction> <fire>\r\n
```

Transmet les commandes du joueur :

- `<direction>` : entier `1` à `9` selon le pavé numérique (1=bas‑gauche, 2=bas, 3=bas‑droite, 4=gauche, 5=aucun mouvement, 6=droite, 7=haut‑gauche, 8=haut, 9=haut‑droite).
- `<fire>` : `0` pour ne pas tirer, `1` pour déclencher un tir.

Le serveur met à jour les composantes `PlayerInput` et transmettra la nouvelle position via un message `STATE`.

### STATE (Serveur → Client)

```text
STATE <msgId> <tick> <entity> <x> <y> <health>\r\n
```

Fournit l'état d'une entité à un instant `tick` :

- `<msgId>` : identifiant unique du message (incremental). Les clients doivent envoyer un `ACK` pour chaque `msgId` reçu.
- `<tick>` : compteur de tick du serveur, utile pour le debug.
- `<entity>` : identifiant de l'entité (correspond à l'ID reçu dans `WELCOME`).
- `<x> <y>` : position autoritaire de l'entité.
- `<health>` : points de vie restants.

Dans ce MVP, seul le joueur reçoit son propre `STATE`, mais le protocole est extensible à plusieurs entités.

### ACK (Client → Serveur)

```text
ACK <msgId>\r\n
```

Accuse réception d'un message `STATE`. Le serveur peut alors retirer ce message de sa file et ne plus le renvoyer.

### PING / PONG (Client ↔ Serveur)

Messages optionnels pour tester la connectivité :

```text
PING\r\n   // Client ou serveur
PONG\r\n   // Réponse
```

## Gestion des erreurs

Le serveur ignore silencieusement les messages mal formatés ou inconnus. Les clients doivent gérer l'absence de réponse (timeout) en implémentant un mode dégradé. Aucune authentification n'est prévue dans ce MVP.