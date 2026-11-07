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
- [x] Écrire un programme `test_socket.c` : un serveur qui écoute sur 127.0.0.1 (port 8080). -> *(DEV A)* ✅
- [x] Écrire un programme `test_client.c` : un client qui se connecte au serveur et envoie "Bonjour". -> *(DEV B)* ✅
- [x] Tester la communication entre les deux. -> *(A et B ensemble)* ✅

### 3. Jour 5-7 : Lancement Multi-Processus (`fork`)
- [ ] Transformer le code pour qu'un programme principal lance **n** processus enfants avec `fork()`. -> *(DEV A)*
- [ ] Faire en sorte que chaque processus ait un identifiant (ID 1, 2, 3, 4...) et connaisse son rôle (Commandant ou Lieutenant). -> *(DEV B)*
- [x] Mettre en place un système de log simple (fichier texte) pour tracer qui parle à qui. -> *(DEV B)* ✅

---

## ⚔️ Semaine 2 : L'Algorithme (Tour 1 et Tour 2) (Jours 8 à 14)
**Objectif :** Coder la logique de discussion et le mensonge.

### 4. Jour 8-10 : Tour 1 (Le Commandant parle)
- [ ] Coder la fonction d'envoi de message du Commandant vers tous les Lieutenants. -> *(DEV A)*
- [ ] Coder la fonction de réception côté Lieutenant (stockage du message dans un tableau `recu[expediteur]`). -> *(DEV B)*
- [ ] Ajouter le mode Traître : si l'ID est traître, le Commandant envoie une valeur aléatoire ou inversée. -> *(DEV A)*

### 5. Jour 11-14 : Tour 2 (Les Lieutenants se parlent) et VOTE
- [ ] Coder la retransmission : chaque Lieutenant envoie à tous les autres ce qu'il a reçu au Tour 1. -> *(DEV A)*
- [ ] Coder la matrice de réception : chaque Lieutenant stocke ce que les autres lui ont dit au Tour 2. -> *(DEV B)*
- [ ] Coder la fonction Vote : analyser la matrice et décider "Attaque" (1) ou "Retraite" (0) par majorité. -> *(DEV A)*
- [ ] **Test critique :** Lancer avec 1 Commandant loyal, 3 Lieutenants loyaux. Vérifier que tout le monde vote pareil. -> *(A et B ensemble)*

---

## 🧪 Semaine 3 : Validation et Cas Complexes (Jours 15 à 21)
**Objectif :** Prouver que l'algorithme marche et échoue aux bons endroits.

### 6. Jour 15-17 : Test n=4, m=1 (La borne est respectée)
- [ ] Créer un script shell (`run_m1.sh`) qui lance 4 processus (1 Commandant + 3 Lieutenants) dont 1 traître. -> *(DEV B)*
- [ ] Vérifier dans les logs que les 3 loyaux ont pris la même décision. -> *(DEV B)*
- [ ] Créer un script Python (avec matplotlib ou Graphviz) pour dessiner les échanges à partir des logs (pour le rapport). -> *(DEV A)*

### 7. Jour 18-19 : Test n=3, m=1 (La borne est violée)
- [ ] Créer un script shell (`run_erreur.sh`) qui lance 3 processus dont 1 traître. -> *(DEV B)*
- [ ] Vérifier que les 2 loyaux ne sont **PAS** d'accord (ou sont bloqués). Prendre une capture d'écran des logs pour le rapport. -> *(DEV B)*

### 8. Jour 20-21 : Préparation pour m=2 (Optionnel mais recommandé)
- [ ] Si vous avez le temps, ajouter un 3ème tour de discussion pour gérer 2 traîtres (n=7). -> *(DEV A)*
- [ ] Sinon, préparer l'explication théorique de pourquoi ça échoue sans le 3ème tour. -> *(DEV B)*

---

## 📝 Semaine 4 : Rapport et Soutenance (Jours 22 à 28)
**Objectif :** Finaliser le dépôt Git et rédiger.

### 9. Jour 22-24 : Nettoyage du Code et Makefile
- [ ] Nettoyer le code C, ajouter des commentaires clairs. -> *(DEV A)*
- [ ] Finaliser le Makefile pour que la compilation se fasse en une seule commande `make`. -> *(DEV A)*
- [ ] Vérifier que le dépôt Git est propre et que les commits sont réguliers. -> *(DEV B)*

### 10. Jour 25-28 : Rapport et Préparation Orale
- [ ] Rédiger l'introduction, la conception (diagrammes UML) et l'explication de l'algorithme. -> *(DEV B)*
- [ ] Rédiger la partie Implémentation, les résultats des tests (n=4 vs n=3), et l'analyse de complexité (O(nm)). -> *(DEV A)*
- [ ] Préparer la démo en direct pour la soutenance. -> *(Les deux)*

---

## 🛠️ Résumé des Outils à Installer (Ordre chronologique)
- [x] **WSL2 / Ubuntu** (ou VM) -> Indispensable jour 1.
- [x] **VS Code** (avec l'extension "C/C++") -> Indispensable jour 1.
- [x] **GCC, Make, GDB** (`sudo apt install build-essential gdb`) -> Indispensable jour 1.
- [x] **Git** (`sudo apt install git`) + Compte GitHub/GitLab -> Indispensable jour 1.
- [x] **Python3 + Matplotlib / Graphviz** (`sudo apt install python3 python3-pip`) -> Requis à la semaine 3 pour faire les graphiques du rapport.

---

## 👥 Répartition du Travail (Conseil)
- **DEV A** est le *"Réseau et Logique"* : il s'occupe des sockets, du `fork()`, des boucles de discussion (Tours 1 et 2) et du Makefile.
- **DEV B** est le *"Système et Analyse"* : il s'occupe de la gestion des fichiers logs, des scripts de test automatisés (shell), des graphiques Python et de la rédaction du rapport.

*Note : Vous pouvez échanger les rôles au milieu du projet pour que chacun comprenne tout, mais pour la vitesse, il vaut mieux que chacun se spécialise sur sa partie.*