# 📅 Ordre Chronologique - Système de Vote Byzantin (C/Linux)

## 📦 Semaine 1 : Fondations et Environnement (Jours 1 à 7)
**Objectif :** Avoir Linux, écrire du C, et faire communiquer 2 programmes.

### 1. Jour 1-2 : Setup Projet et Outils
- [x] Installer WSL2 (Ubuntu) sur Windows ou utiliser une VM Linux. -> *(Les deux)* ✅
- [x] Installer VS Code avec l'extension C/C++. -> *(Les deux)* ✅
- [x] Installer GCC, Make et GDB (`sudo apt install build-essential gdb`). -> *(Les deux)* ✅
- [x] Créer le dépôt Git (GitHub ou GitLab) et inviter le binôme. -> *(Les deux)* ✅
- [x] Créer la structure de dossiers : `/src`, `/logs`, `/scripts`, et un Makefile de base. -> *(DEV A)* ✅

### 2. Jour 3-4 : Première Communication (Sockets)
- [x] Écrire un programme `test_socket.c` : un serveur qui écoute sur 127.0.0.1 (port 8080). -> *(DEV A)* ✅ *(Renommé `server.c` dans le dépôt)*
- [x] Écrire un programme `test_client.c` : un client qui se connecte au serveur et envoie "Bonjour". -> *(DEV B)* ✅
- [x] Tester la communication entre les deux. -> *(A et B ensemble)* ✅

### 3. Jour 5-7 : Lancement Multi-Processus (`fork`)
- [x] Transformer le code pour qu'un programme principal lance **n** processus enfants avec `fork()`. -> *(DEV A)* ✅
- [x] Faire en sorte que chaque processus ait un identifiant (ID 1, 2, 3, 4...) et connaisse son rôle (Commandant ou Lieutenant). -> *(DEV A)* ✅
- [x] Mettre en place un système de log simple (fichier texte) pour tracer qui parle à qui. -> *(DEV B)* ✅ *(Implémenté dans `src/log.c` et `src/log.h` avec verrouillage `flock`)*

---

## ⚔️ Semaine 2 : L'Algorithme (Tour 1 et Tour 2) (Jours 8 à 14)
**Objectif :** Coder la logique de discussion et le mensonge.

### 4. Jour 8-10 : Tour 1 (Le Commandant parle)
- [x] Coder la fonction d'envoi de message du Commandant vers tous les Lieutenants. -> *(DEV A)* ✅ *(Dans `src/main.c`)*
- [x] Coder la fonction de réception côté Lieutenant (stockage du message dans un tableau `recu[expediteur]`). -> *(DEV B)* ✅ *(Dans `agir_en_lieutenant` de `src/main.c`)*
- [x] Ajouter le mode Traître : si l'ID est traître, le Commandant envoie une valeur aléatoire ou inversée. -> *(DEV A)* ✅

### 5. Jour 11-14 : Tour 2 (Les Lieutenants se parlent) et VOTE
- [x] Coder la retransmission : chaque Lieutenant envoie à tous les autres ce qu'il a reçu au Tour 1. -> *(DEV A)* ✅
- [x] Coder la matrice de réception : chaque Lieutenant stocke ce que les autres lui ont dit au Tour 2. -> *(DEV B)* ✅
- [x] Coder la fonction Vote : analyser la matrice et décider "Attaque" (1) ou "Retraite" (0) par majorité. -> *(DEV A)* ✅ *(Fonction `voter()` dans `src/main.c`)*
- [x] **Test critique :** Lancer avec 1 Commandant loyal, 3 Lieutenants loyaux. Vérifier que tout le monde vote pareil. -> *(A et B ensemble)* ✅

---

## 🧪 Semaine 3 : Validation et Cas Complexes (Jours 15 à 21)
**Objectif :** Prouver que l'algorithme marche et échoue aux bons endroits.

### 6. Jour 15-17 : Test n=4, m=1 (La borne est respectée)
- [x] Créer un script shell (`scripts/run_test.sh`) qui lance 4 processus (1 Commandant + 3 Lieutenants) dont 1 traître. -> *(DEV B)* ✅
- [x] Vérifier dans les logs que les 3 loyaux ont pris la même décision. -> *(DEV B)* ✅
- [x] Créer un script Python (`scripts/visualiser_logs.py`) avec Graphviz pour dessiner les échanges à partir des logs (pour le rapport). -> *(DEV B)* ✅

### 7. Jour 18-19 : Test n=3, m=1 (La borne est violée)
- [x] Utiliser le script shell (`scripts/run_test.sh 3 1`) qui lance 3 processus dont 1 traître. -> *(DEV B)* ✅
- [x] Vérifier que les 2 loyaux ne sont **PAS** d'accord (ou sont bloqués). Prendre une capture d'écran des logs pour le rapport. -> *(DEV B)* ✅

---

## 🛠️ Résumé des Outils Installés
- [x] **Ubuntu / Linux** (Machine virtuelle VirtualBox)
- [x] **VS Code** (avec l'extension "C/C++")
- [x] **GCC, Make, GDB**
- [x] **Git & GitHub**
- [x] **Python3 + Matplotlib / Graphviz** (pour les graphiques)

---

## 👥 Répartition du Travail
- **DEV A** (Réseau et Logique) : Sockets, `fork()`, boucles de discussion (Tours 1 et 2) et Makefile.
- **DEV B** (Système et Analyse) : Gestion des fichiers logs, scripts de test automatisés (shell), graphiques Python et rédaction du rapport.