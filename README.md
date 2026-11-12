# Simulation du problème des généraux byzantins (C/Linux)

Projet pédagogique qui simule l'algorithme oral OM(m) de Lamport, Shostak et Pease avec des processus Linux et des sockets TCP locales.

## Prérequis et compilation

- Linux ou WSL2 avec une distribution Linux
- GCC et GNU Make
- Python 3, le module Python `graphviz` et le programme Graphviz pour produire le schéma

Depuis la racine du dépôt :

```sh
make
```

Cette commande crée `logs/` et compile `main`, `server` et `test_client`. Les deux derniers sont des exercices client/serveur indépendants de la simulation.

```sh
make clean
```

Supprime les exécutables et les fichiers générés dans `logs/`, y compris les journaux archivés.

## Lancer une configuration

```sh
./main <n> <m> [liste_ids_traitres]
```

- `n` : nombre de généraux, commandant compris. Le commandant a l'ID `0` et les lieutenants les IDs `1` à `n-1`.
- `m` : nombre de processus byzantins.
- `liste_ids_traitres` : exactement `m` IDs distincts compris entre `0` et `n-1`. Il permet de choisir le commandant, un ou plusieurs lieutenants, ou les deux.

Si la liste est omise, les IDs `0` à `m-1` sont utilisés par défaut. Exemples :

```sh
./main 4 1 1       # un lieutenant traître, commandant loyal
./main 4 1 0       # commandant traître
./main 7 2 0,3      # commandant et lieutenant 3 traîtres
./main 3 1 0       # scénario hors borne
```

Le programme accepte `3 <= n <= 32` et `0 <= m <= n-2`. Une limite de 50 000 chemins par processus prévient les configurations trop coûteuses en mémoire et en messages.

## Algorithme et borne

OM(m) nécessite `m+1` tours pour tolérer jusqu'à `m` fautes byzantines :

1. Au premier tour, le commandant envoie une valeur à chaque lieutenant.
2. Aux tours suivants, les lieutenants retransmettent récursivement les valeurs reçues, avec un chemin d'expéditeurs pour distinguer chaque branche de l'arbre OM.
3. Chaque lieutenant calcule les majorités de l'arbre, puis sa décision finale. En cas d'égalité, la valeur par défaut est `Retraite`.

Un processus déclaré traître simule un comportement byzantin déterministe : il peut envoyer des valeurs différentes selon le destinataire et le chemin. Ce modèle permet de répéter les scénarios, mais ne couvre pas toutes les stratégies byzantines possibles.

La garantie théorique d'accord et de validité exige `n >= 3m + 1`. Une configuration hors borne reste autorisée afin d'observer les limites. Elle ne doit pas nécessairement produire des décisions différentes lors de chaque exécution : la borne indique l'absence de garantie, pas un désaccord obligatoire dans tout scénario.

Scénarios demandés :

```sh
./scripts/run_test.sh 4 1 1
./scripts/run_test.sh 7 2 0,3
./scripts/run_test.sh 3 1 0
```

Le script vérifie l'accord des lieutenants loyaux et, si le commandant est loyal, la validité de sa valeur. Pour `n=3, m=1`, il signale le résultat observé sans exiger un désaccord, puisque le scénario est hors borne.

Pour lancer les trois cas :

```sh
bash scripts/run_test_suite.sh
```

Chaque exécution conserve une copie du journal sous `logs/trace_n*_m*_traitres-*.log`. Le journal courant reste `logs/trace_globale.log`.

## Complexité des messages

Le nombre de messages de cette version d'OM(m) est :

```text
(n - 1) + (n - 1) * somme(P(n - 2, k), k = 1..m)
```

où `P(a,k) = a * (a-1) * ... * (a-k+1)`. Pour `m` fixé, cela donne `O(n^(m+1))`; quand `m` augmente, le nombre de messages croît de façon combinatoire. Ce n'est donc pas `O(nm)`. Cette croissance explique la limite de chemins imposée par le programme.

## Journaux et visualisation

Les processus écrivent dans `logs/trace_globale.log` au format :

```text
[AAAA-MM-JJ HH:MM:SS.mmm] [PID:ID] [ACTION] détails
```

Les journaux incluent les rôles, les messages OM (tour, chemin, valeur et destinataire), les réceptions, les votes et les décisions. L'accès concurrent au journal est verrouillé par `flock`.

Pour dessiner le graphe du dernier scénario exécuté :

```sh
python3 scripts/visualiser_logs.py
```

Le graphe est enregistré dans `logs/graphe_reseau.png` et construit ses nœuds à partir du journal, y compris pour `n=7`.

## Structure du dépôt

```text
src/
  main.c          Simulation OM(m), processus, sockets et votes
  log.c, log.h    Journalisation partagée
  server.c        Serveur TCP de démonstration
  test_client.c   Client TCP de démonstration
scripts/
  run_test.sh              Lance et vérifie une configuration
  run_test_suite.sh        Lance les trois scénarios demandés
  verifier_resultats.py    Vérifie accord et validité selon la borne
  visualiser_logs.py       Génère le graphe des échanges
logs/                      Journaux archivés et graphe généré
```
