#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include "log.h" 

#define BASE_PORT 8080 
#define BUFFER_SIZE 1024

// ==========================================
// FONCTION DE VOTE
// ==========================================
void voter(int id, int n, char matrice[][BUFFER_SIZE], char *message_commandant, char *decision_finale) {
    int votes_attaque = 0;
    int votes_retraite = 0;

    // 1. On compte le vote du Commandant
    if (strcmp(message_commandant, "Attaque") == 0) {
        votes_attaque++;
    } else if (strcmp(message_commandant, "Retraite") == 0) {
        votes_retraite++;
    }

    // 2. On compte les votes des autres lieutenants
    for (int i = 0; i < n - 2; i++) {
        if (strcmp(matrice[i], "Attaque") == 0) {
            votes_attaque++;
        } else if (strcmp(matrice[i], "Retraite") == 0) {
            votes_retraite++;
        }
    }
    
    // 3. Décision par majorité simple
    if (votes_attaque > votes_retraite) {
        strcpy(decision_finale, "Attaque");
    } else if (votes_retraite > votes_attaque) {
        strcpy(decision_finale, "Retraite");
    } else {
        strcpy(decision_finale, "Egalite (Retraite par defaut)"); 
    }
    
    char msg_log[1500]; 
    snprintf(msg_log, sizeof(msg_log), "Vote : %d Attaque, %d Retraite -> Decision : %s", 
             votes_attaque, votes_retraite, decision_finale);
    log_message(id, "VOTE", msg_log);
}

// ==========================================
// FONCTION POUR LE LIEUTENANT
// ==========================================
void agir_en_lieutenant(int id, int n) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    int port = BASE_PORT + id; 

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { perror("socket"); exit(1); }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("bind"); exit(1); }
    if (listen(server_fd, 5) < 0) { perror("listen"); exit(1); }

    char msg_log[1500];
    snprintf(msg_log, sizeof(msg_log), "En attente sur le port %d", port);
    log_message(id, "ATTENTE", msg_log);

    // --- TOUR 1 : Réception du Commandant ---
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) { perror("accept"); exit(1); }
    read(new_socket, buffer, BUFFER_SIZE);
    
    // On envoie un ACK (Accusé de réception) au Commandant
    send(new_socket, "OK", 2, 0); 
    close(new_socket);

    snprintf(msg_log, sizeof(msg_log), "J'ai recu du Commandant : %s", buffer);
    log_message(id, "RECEPTION_TOUR1", msg_log);

    // On attend un peu pour laisser le temps au Commandant de finir ses envois
    usleep(500000); // 500 ms

    // --- TOUR 2 (ENVOI) ---
    log_message(id, "TOUR2_ENVOI", "Je retransmets ce que j'ai recu aux autres");
    
    for (int j = 1; j < n; j++) {
        if (id == j) continue; 
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(BASE_PORT + j);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
            char message_a_envoyer[1100]; 
            snprintf(message_a_envoyer, sizeof(message_a_envoyer), "%d:%s", id, buffer);
            
            send(sock, message_a_envoyer, strlen(message_a_envoyer), 0);
            snprintf(msg_log, sizeof(msg_log), "Envoye '%s' au lieutenant %d", message_a_envoyer, j);
            log_message(id, "TOUR2_ENVOI", msg_log);
        }
        close(sock);
    }

    // --- TOUR 2 (RECEPTION) ---
    log_message(id, "TOUR2_RECEPTION", "J'écoute les messages des autres lieutenants");
    
    char matrice[n][BUFFER_SIZE]; 
    memset(matrice, 0, sizeof(matrice)); 

    struct timeval tv;
    tv.tv_sec = 2;  
    tv.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    for (int k = 0; k < n - 2; k++) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) { 
            log_message(id, "TIMEOUT", "J'ai attendu trop longtemps, j'arrête d'écouter");
            break; 
        }
        char buf_tour2[BUFFER_SIZE] = {0};
        read(new_socket, buf_tour2, BUFFER_SIZE);
        close(new_socket);
        
        int expediteur_id;
        char contenu[BUFFER_SIZE];
        if (sscanf(buf_tour2, "%d:%s", &expediteur_id, contenu) == 2) {
            strcpy(matrice[k], contenu); 
            snprintf(msg_log, sizeof(msg_log), "J'ai recu '%s' du lieutenant %d", contenu, expediteur_id);
            log_message(id, "TOUR2_RECEPTION", msg_log);
        }
    }

    close(server_fd);

    // --- APPEL DE LA FONCTION VOTE ---
    char decision[BUFFER_SIZE];
    // On passe "buffer" qui contient le message du Commandant
    voter(id, n, matrice, buffer, decision); 
    
    snprintf(msg_log, sizeof(msg_log), "Ma decision finale est : %s", decision);
    log_message(id, "DECISION_FINALE", msg_log);

    log_message(id, "FIN", "J'ai fini mes tours de discussion");
}

// ==========================================
// FONCTION POUR LE COMMANDANT (Envoi)
// ==========================================
void agir_en_commandant(int n, int est_traitre) {
    char *vrai_message = "Attaque"; 
    char *faux_message = "Retraite";

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

        // On essaie de se connecter (on réessaie 5 fois si besoin)
        int connecte = 0;
        for (int essai = 0; essai < 5; essai++) {
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) {
                connecte = 1;
                break;
            }
            usleep(100000); // Attendre 100ms et réessayer
        }

        if (!connecte) {
            char msg_err[150];
            snprintf(msg_err, sizeof(msg_err), "Impossible de contacter le lieutenant %d", i);
            log_message(0, "ERREUR", msg_err);
        } else {
            char *message_a_envoyer;
            
            if (est_traitre) {
                if (rand() % 2 == 0) {
                    message_a_envoyer = vrai_message;
                } else {
                    message_a_envoyer = faux_message;
                }
            } else {
                message_a_envoyer = vrai_message;
            }
            
            send(sock, message_a_envoyer, strlen(message_a_envoyer), 0);
            
            char msg_log[150];
            snprintf(msg_log, sizeof(msg_log), "Message '%s' envoye au lieutenant %d", message_a_envoyer, i);
            log_message(0, "ENVOI", msg_log);
            
            // ⚠️ CRUCIAL : On attend l'ACK du Lieutenant
            char ack[10];
            read(sock, ack, sizeof(ack)); 
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

            char details[200];
            sprintf(details, "Je suis le general %d. Fonction : %s. Loyaute : %s", i, fonction, loyaute);
            log_message(i, "DEMARRAGE", details);

            if (i == 0) {
                agir_en_commandant(n, est_traitre); 
            } else {
                agir_en_lieutenant(i, n); 
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