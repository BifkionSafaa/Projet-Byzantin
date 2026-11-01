#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char *hello = "Bonjour depuis le Serveur !";

    // 1. Création du socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    // 2. Configuration de l'adresse (Port 8080, toutes les interfaces)
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 3. Attachement du socket au port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Erreur bind");
        exit(EXIT_FAILURE);
    }

    // 4. Mise en écoute (attente de connexion)
    if (listen(server_fd, 3) < 0) {
        perror("Erreur listen");
        exit(EXIT_FAILURE);
    }
    printf("Serveur en attente sur le port %d...\n", PORT);

    // 5. Acceptation de la connexion du client
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Erreur accept");
        exit(EXIT_FAILURE);
    }

    // 6. Lecture du message du client
    read(new_socket, buffer, BUFFER_SIZE);
    printf("Message recu du client : %s\n", buffer);

    // 7. Envoi d'une réponse au client
    send(new_socket, hello, strlen(hello), 0);
    printf("Reponse envoyee au client.\n");

    // 8. Fermeture
    close(new_socket);
    close(server_fd);
    return 0;
}