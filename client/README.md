# R-Type Client MVP

Ce dossier contient un client minimal (« MVP ») pour l'application de jeu R‑Type.  
Ce client a été conçu pour démontrer l'intégration avec le serveur UDP fourni dans ce dépôt, en offrant un flux simple permettant de se connecter, d'envoyer des entrées et de recevoir les états du joueur.

## Fonctionnalités

Le client de la Defense 1 n'est plus une simple interface en ligne de commande mais un prototype graphique utilisant SFML.  
Il offre :

- **Fond étoilé animé** : un starfield se déplace de droite à gauche pour créer une impression de profondeur.
- **Vaisseau jouable** : représenté par un rectangle cyan, il se déplace grâce aux flèches directionnelles (ou ZQSD) et peut tirer via la touche `Espace`.
- **Réseau UDP** : au démarrage le client envoie `HELLO` et attend `WELCOME`. À chaque frame, il envoie `INPUT <direction> <fire>` au serveur et applique les positions reçues via `STATE`.
- **Configuration simple** : l'adresse IP et le port du serveur se configurent par variables d'environnement (`RTYPE_SERVER_IP`, `RTYPE_SERVER_PORT`) ou en arguments.
- **Mode hors ligne** : si aucun serveur ne répond au handshake, le prototype fonctionne en prédiction locale.
- **Sortie propre** : fermez simplement la fenêtre ou pressez `Ctrl+C` pour quitter.

## Compilation

Le dépôt utilise CMake. Depuis la racine du dépôt :

```bash
mkdir -p build
cd build
cmake ..
make client
```

Cela génère un exécutable `client` dans `build/client`.

Vous pouvez également compiler directement le client sans le reste du projet :

```bash
g++ -std=c++17 -I../includes -I../../network/includes ../main.cpp ../../network/src/udpClient.cpp ../../network/src/threadQueue.cpp -pthread -o rtype_client
```

Assurez‑vous que les dépendances système nécessaires sont installées (bibliothèques de sockets POSIX).

## Exécution

Définissez les variables d'environnement si nécessaire :

```bash
export RTYPE_SERVER_IP=127.0.0.1
export RTYPE_SERVER_PORT=4242
./client
```

ou passez l'adresse et le port directement :

```bash
./client 192.168.1.10 4242
```

Une fenêtre 800×600 s'ouvre :

- **Flèches directionnelles** pour déplacer le vaisseau (8 = haut, 2 = bas, 4 = gauche, 6 = droite).
- **Espace** pour tirer (fire = 1).  

Le client envoie vos commandes au serveur et applique les positions renvoyées. Si aucun serveur ne répond, il continue à animer localement le vaisseau et le starfield.

## Tests rapides

Pour tester rapidement la communication, vous pouvez lancer le serveur fourni dans ce dépôt (voir `server/`).  
Ouvrez un terminal :

```bash
./server
```

puis dans un autre terminal :

```bash
./client
```

Le client devrait recevoir un message `WELCOME <id>` lors de la connexion et des messages `STATE` lors des mises à jour du serveur.

## Étapes suivantes suggérées

- Ajouter une interface graphique (SFML, SDL ou autre) pour rendre l'expérience plus conviviale.
- Implémenter la gestion de plusieurs joueurs et l'affichage des entités ennemies.
- Ajouter des tests unitaires automatisés pour les messages réseau et la logique de parsing.
- Gérer les erreurs réseau de manière plus fine (reconnexion automatique, timeouts configurables).
