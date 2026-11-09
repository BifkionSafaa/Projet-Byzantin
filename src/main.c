#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h> // NOUVEAU : pour le hasard
#include "log.h" 

#define BASE_PORT 8080 
#define BUFFER_SIZE 1024

// ==========================================
// FONCTION POUR LE LIEUTENANT (Réception)
// ==========================================
void agir_en_lieutenant(int id) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    int port = BASE_PORT + id; 

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Erreur socket lieutenant");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Erreur bind lieutenant");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Erreur listen lieutenant");
        exit(EXIT_FAILURE);
    }

    char msg_log[100];
    snprintf(msg_log, sizeof(msg_log), "En attente sur le port %d", port);
    log_message(id, "ATTENTE", msg_log);

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Erreur accept lieutenant");
        exit(EXIT_FAILURE);
    }

    // Lecture du message (Pas de stockage, c'est la tâche de DEV B)
    read(new_socket, buffer, BUFFER_SIZE);
    
    snprintf(msg_log, sizeof(msg_log), "J'ai recu le message : %s", buffer);
    log_message(id, "RECEPTION", msg_log);

    close(new_socket);
    close(server_fd);
}

// ==========================================
// FONCTION POUR LE COMMANDANT (Envoi)
// ==========================================
void agir_en_commandant(int n, int est_traitre) {
    char *vrai_message = "Attaque"; 
    char *faux_message = "Retraite";

    // Initialisation du générateur aléatoire (une seule fois par processus)
    srand(time(NULL) + getpid()); 

    log_message(0, "ENVOI", "Je commence a envoyer mon ordre aux lieutenants");

    for (int i = 1; i < n; i++) {
        int sock = 0;
        struct sockaddr_in serv_addr;
        
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Erreur socket commandant");
            continue;
        }
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(BASE_PORT + i);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

        sleep(1); 

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            char msg_err[100];
            snprintf(msg_err, sizeof(msg_err), "Impossible de contacter le lieutenant %d", i);
            log_message(0, "ERREUR", msg_err);
        } else {
            char *message_a_envoyer;
            
            // === MODE TRAITRE : ENVOI ALEATOIRE ===
            if (est_traitre) {
                // Si c'est un traître, il tire à pile ou face pour mentir ou dire la vérité
                if (rand() % 2 == 0) {
                    message_a_envoyer = vrai_message;
                } else {
                    message_a_envoyer = faux_message;
                }
            } else {
                // Si loyal, il dit la vérité
                message_a_envoyer = vrai_message;
            }
            
            send(sock, message_a_envoyer, strlen(message_a_envoyer), 0);
            
            char msg_log[100];
            snprintf(msg_log, sizeof(msg_log), "Message '%s' envoye au lieutenant %d", message_a_envoyer, i);
            log_message(0, "ENVOI", msg_log);
        }
        close(sock);
    }
}

// ==========================================
// PROGRAMME PRINCIPAL
// ==========================================
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <nombre_total_n> <nombre_traitres_m>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]); 
    int m = atoi(argv[2]); 

    printf("Lancement du systeme avec n=%d et m=%d\n", n, m);

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Erreur fork");
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) {
            log_init("logs/trace_globale.log");

            char fonction[20];
            if (i == 0) sprintf(fonction, "COMMANDANT");
            else sprintf(fonction, "LIEUTENANT");

            int est_traitre = (i < m); 
            char loyaute[20];
            if (est_traitre) sprintf(loyaute, "TRAITRE");
            else sprintf(loyaute, "LOYAL");

            char details[150];
            sprintf(details, "Je suis le general %d. Fonction : %s. Loyaute : %s", i, fonction, loyaute);
            log_message(i, "DEMARRAGE", details);

            if (i == 0) {
                agir_en_commandant(n, est_traitre); 
            } else {
                agir_en_lieutenant(i); 
            }

            log_close();
            exit(0); 
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }
    printf("Tous les processus sont termines. Regardez le fichier logs/trace_globale.log\n");

    return EXIT_SUCCESS;
}