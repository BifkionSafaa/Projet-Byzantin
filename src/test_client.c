#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "log.h"

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080

int main(void) {
    log_init("logs/trace_client.log");
    log_message(0, "CLIENT_START", "Client démarré");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return EXIT_FAILURE; }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton"); close(sock); return EXIT_FAILURE;
    }

    log_message(0, "CONNECTING", SERVER_IP);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); close(sock); return EXIT_FAILURE;
    }
    log_message(0, "CONNECTED", "Connexion établie");

    const char *msg = "Bonjour";
    send(sock, msg, strlen(msg), 0);
    log_message(0, "SENT", msg);

    close(sock);
    log_message(0, "CLIENT_END", "Client terminé");
    log_close();
    return EXIT_SUCCESS;
}