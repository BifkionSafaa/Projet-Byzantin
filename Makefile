CC=gcc
CFLAGS=-Wall -Wextra -g

# Par défaut, on compile les deux programmes
all: main server

main: src/main.c src/log.c
	$(CC) $(CFLAGS) -o main src/main.c src/log.c

# Règle pour le serveur
server: src/server.c
	$(CC) $(CFLAGS) -o server src/server.c

# Pour nettoyer les exécutables
clean:
	rm -f main server
