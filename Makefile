CC=gcc
CFLAGS=-Wall -Wextra -g

# Par défaut, on compile les deux programmes
all: main server

# Règle pour le programme principal (Hello World)
main: src/main.c
	$(CC) $(CFLAGS) -o main src/main.c

# Règle pour le serveur
server: src/server.c
	$(CC) $(CFLAGS) -o server src/server.c

# Pour nettoyer les exécutables
clean:
	rm -f main server
