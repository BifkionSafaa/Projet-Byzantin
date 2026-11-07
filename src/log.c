#include "log.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/file.h> // NOUVEAU : pour les verrous

static FILE *log_file = NULL;

void log_init(const char *filename) {
    log_file = fopen(filename, "a");
    if (!log_file) {
        perror("Erreur ouverture fichier log");
        exit(EXIT_FAILURE);
    }
}

void log_message(int process_id, const char *action, const char *details) {
    if (!log_file) return;

    // On demande un accès exclusif au fichier pour écrire la ligne
    int fd = fileno(log_file);
    flock(fd, LOCK_EX); // Bloque les autres processus pendant l'écriture

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm_info;
    localtime_r(&ts.tv_sec, &tm_info);

    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);

    fprintf(log_file, "[%s.%03ld] [PID:%d] [%s] %s\n",
            time_buf, ts.tv_nsec / 1000000,
            process_id, action, details);
    fflush(log_file);

    // On libère le verrou une fois la ligne écrite
    flock(fd, LOCK_UN); 
}

void log_close(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}