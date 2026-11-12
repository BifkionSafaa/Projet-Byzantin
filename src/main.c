#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

#define BASE_PORT 8080
#define MAX_GENERALS 32
#define MAX_RECORDS_PER_PROCESS 50000
#define FRAME_SIZE 512
#define ACCEPT_TIMEOUT_MS 30000

typedef enum {
    VOTE_RETREAT = 0,
    VOTE_ATTACK = 1
} vote_t;

typedef struct {
    uint8_t depth;
    uint8_t path[MAX_GENERALS];
    vote_t value;
} message_record_t;

static int parse_integer(const char *text, int minimum, int maximum, int *value) {
    char *end;
    long parsed;

    if (text == NULL || *text == '\0' || value == NULL) {
        return 0;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || *end != '\0' || parsed < minimum || parsed > maximum) {
        return 0;
    }

    *value = (int) parsed;
    return 1;
}

static size_t permutation_count(int available, int depth) {
    size_t count = 1;
    int i;

    if (depth < 0 || depth > available) {
        return 0;
    }

    for (i = 0; i < depth; i++) {
        count *= (size_t) (available - i);
        if (count > MAX_RECORDS_PER_PROCESS) {
            return 0;
        }
    }

    return count;
}

static int calculate_record_capacity(int n, int m, size_t *capacity) {
    size_t total = 1;
    int depth;

    for (depth = 1; depth <= m; depth++) {
        size_t paths = permutation_count(n - 2, depth);
        if (paths == 0 || total > MAX_RECORDS_PER_PROCESS - paths) {
            return 0;
        }
        total += paths;
    }

    *capacity = total;
    return 1;
}

static int parse_traitor_ids(const char *text, int n, int m, int *traitors) {
    char buffer[FRAME_SIZE];
    char *saveptr = NULL;
    char *token;
    int count = 0;
    int i;

    if (text == NULL) {
        for (i = 0; i < m; i++) {
            traitors[i] = i;
        }
        return 1;
    }

    if (strlen(text) >= sizeof(buffer)) {
        return 0;
    }
    if (*text == '\0' || text[0] == ',' ||
        text[strlen(text) - 1] == ',' || strstr(text, ",,") != NULL) {
        return 0;
    }
    strcpy(buffer, text);

    token = strtok_r(buffer, ",", &saveptr);
    while (token != NULL) {
        int id;

        if (count >= m || !parse_integer(token, 0, n - 1, &id)) {
            return 0;
        }
        for (i = 0; i < count; i++) {
            if (traitors[i] == id) {
                return 0;
            }
        }
        traitors[count++] = id;
        token = strtok_r(NULL, ",", &saveptr);
    }

    return count == m;
}

static int is_traitor(int id, int traitor_count, const int *traitors) {
    int i;

    for (i = 0; i < traitor_count; i++) {
        if (traitors[i] == id) {
            return 1;
        }
    }
    return 0;
}

static int path_contains(const uint8_t *path, int depth, int id) {
    int i;

    for (i = 0; i < depth; i++) {
        if ((int) path[i] == id) {
            return 1;
        }
    }
    return 0;
}

static int find_record(const message_record_t *records, size_t count,
                       const uint8_t *path, int depth) {
    size_t i;

    for (i = 0; i < count; i++) {
        if (records[i].depth == depth &&
            (depth == 0 || memcmp(records[i].path, path, (size_t) depth) == 0)) {
            return (int) i;
        }
    }
    return -1;
}

static int store_record(message_record_t *records, size_t capacity, size_t *count,
                        const message_record_t *record) {
    int existing = find_record(records, *count, record->path, record->depth);

    if (existing >= 0) {
        return records[existing].value == record->value ? 0 : -1;
    }
    if (*count >= capacity) {
        return -1;
    }

    records[*count] = *record;
    (*count)++;
    return 1;
}

static int send_all(int socket_fd, const char *buffer, size_t length) {
    size_t sent_total = 0;

    while (sent_total < length) {
        ssize_t sent = send(socket_fd, buffer + sent_total, length - sent_total, 0);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }
        if (sent == 0) {
            return 0;
        }
        sent_total += (size_t) sent;
    }
    return 1;
}

static void sleep_milliseconds(long milliseconds) {
    struct timespec remaining;

    remaining.tv_sec = milliseconds / 1000;
    remaining.tv_nsec = (milliseconds % 1000) * 1000000L;
    while (nanosleep(&remaining, &remaining) < 0) {
        if (errno != EINTR) {
            perror("nanosleep");
            return;
        }
    }
}

static void format_path(const uint8_t *path, int depth, char *buffer, size_t size) {
    size_t used = 0;
    int i;

    if (depth == 0) {
        snprintf(buffer, size, "-");
        return;
    }

    buffer[0] = '\0';
    for (i = 0; i < depth; i++) {
        int written = snprintf(buffer + used, size - used, "%s%d",
                               i == 0 ? "" : ",", (int) path[i]);
        if (written < 0 || (size_t) written >= size - used) {
            buffer[size - 1] = '\0';
            return;
        }
        used += (size_t) written;
    }
}

static int send_frame(int sender, int receiver, int depth, const uint8_t *path, vote_t value) {
    char frame[FRAME_SIZE];
    char path_text[MAX_GENERALS * 4];
    struct sockaddr_in address;
    int socket_fd = -1;
    int attempt;
    int frame_length;

    format_path(path, depth, path_text, sizeof(path_text));
    frame_length = snprintf(frame, sizeof(frame), "%d|%d|%s|%c\n",
                            depth, sender, path_text,
                            value == VOTE_ATTACK ? 'A' : 'R');
    if (frame_length < 0 || (size_t) frame_length >= sizeof(frame)) {
        return 0;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((uint16_t) (BASE_PORT + receiver));
    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1) {
        return 0;
    }

    for (attempt = 0; attempt < 100; attempt++) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            return 0;
        }
        if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) == 0) {
            break;
        }
        close(socket_fd);
        socket_fd = -1;
        sleep_milliseconds(50);
    }

    if (socket_fd < 0) {
        return 0;
    }

    if (!send_all(socket_fd, frame, (size_t) frame_length)) {
        close(socket_fd);
        return 0;
    }
    close(socket_fd);

    {
        char details[256];
        snprintf(details, sizeof(details),
                 "round=%d path=%s value=%s dest=%d",
                 depth, path_text,
                 value == VOTE_ATTACK ? "Attaque" : "Retraite", receiver);
        log_message(sender, "OM_ENVOI", details);
    }
    return 1;
}

static int receive_line(int socket_fd, char *buffer, size_t size) {
    size_t used = 0;

    if (size < 2) {
        return 0;
    }

    while (used < size - 1) {
        char byte;
        ssize_t received = recv(socket_fd, &byte, 1, 0);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }
        if (received == 0) {
            return 0;
        }
        if (byte == '\n') {
            buffer[used] = '\0';
            return used > 0;
        }
        buffer[used++] = byte;
    }

    buffer[used] = '\0';
    return 0;
}

static int parse_message(const char *line, int n, int receiver, int max_depth,
                         message_record_t *record, int *sender) {
    char copy[FRAME_SIZE];
    char path_copy[FRAME_SIZE];
    char *saveptr = NULL;
    char *depth_text;
    char *sender_text;
    char *path_text;
    char *value_text;
    char *extra;
    char *path_saveptr = NULL;
    char *part;
    int parsed_depth;
    int parsed_sender;
    int count = 0;
    int i;
    int separators = 0;
    const char *cursor;

    if (strlen(line) >= sizeof(copy)) {
        return 0;
    }
    for (cursor = line; *cursor != '\0'; cursor++) {
        if (*cursor == '|') {
            separators++;
        }
    }
    if (separators != 3) {
        return 0;
    }
    strcpy(copy, line);

    depth_text = strtok_r(copy, "|", &saveptr);
    sender_text = strtok_r(NULL, "|", &saveptr);
    path_text = strtok_r(NULL, "|", &saveptr);
    value_text = strtok_r(NULL, "|", &saveptr);
    extra = strtok_r(NULL, "|", &saveptr);
    if (depth_text == NULL || sender_text == NULL || path_text == NULL ||
        value_text == NULL || extra != NULL ||
        !parse_integer(depth_text, 0, max_depth, &parsed_depth) ||
        !parse_integer(sender_text, 0, n - 1, &parsed_sender)) {
        return 0;
    }

    memset(record, 0, sizeof(*record));
    record->depth = (uint8_t) parsed_depth;
    *sender = parsed_sender;

    if (parsed_depth == 0) {
        if (strcmp(path_text, "-") != 0 || parsed_sender != 0) {
            return 0;
        }
    } else {
        if (strcmp(path_text, "-") == 0 || strlen(path_text) >= sizeof(path_copy) ||
            path_text[0] == ',' || path_text[strlen(path_text) - 1] == ',' ||
            strstr(path_text, ",,") != NULL) {
            return 0;
        }
        strcpy(path_copy, path_text);
        part = strtok_r(path_copy, ",", &path_saveptr);
        while (part != NULL) {
            int id;
            if (count >= parsed_depth ||
                !parse_integer(part, 1, n - 1, &id) ||
                id == receiver || path_contains(record->path, count, id)) {
                return 0;
            }
            record->path[count++] = (uint8_t) id;
            part = strtok_r(NULL, ",", &path_saveptr);
        }
        if (count != parsed_depth || parsed_sender != (int) record->path[count - 1]) {
            return 0;
        }
    }

    if (strcmp(value_text, "A") == 0) {
        record->value = VOTE_ATTACK;
    } else if (strcmp(value_text, "R") == 0) {
        record->value = VOTE_RETREAT;
    } else {
        return 0;
    }

    for (i = 0; i < parsed_depth; i++) {
        if (record->path[i] == 0) {
            return 0;
        }
    }
    return 1;
}

static int create_listener(int id) {
    int server_fd;
    int option = 1;
    struct sockaddr_in address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket lieutenant");
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("setsockopt lieutenant");
        close(server_fd);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons((uint16_t) (BASE_PORT + id));
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind lieutenant");
        close(server_fd);
        return -1;
    }
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen lieutenant");
        close(server_fd);
        return -1;
    }

    {
        char details[128];
        snprintf(details, sizeof(details), "En attente sur le port %d", BASE_PORT + id);
        log_message(id, "ATTENTE", details);
    }
    return server_fd;
}

static int accept_and_store(int server_fd, int receiver, int n, int m,
                            message_record_t *records, size_t capacity, size_t *count) {
    struct pollfd descriptor;
    int poll_result;
    int connection_fd;
    char frame[FRAME_SIZE];
    message_record_t record;
    int sender;
    int stored;

    descriptor.fd = server_fd;
    descriptor.events = POLLIN;
    descriptor.revents = 0;
    do {
        poll_result = poll(&descriptor, 1, ACCEPT_TIMEOUT_MS);
    } while (poll_result < 0 && errno == EINTR);

    if (poll_result <= 0) {
        if (poll_result == 0) {
            log_message(receiver, "TIMEOUT", "Delai depasse en attente d'un message OM");
        } else {
            perror("poll");
        }
        return 0;
    }

    connection_fd = accept(server_fd, NULL, NULL);
    if (connection_fd < 0) {
        if (errno == EINTR) {
            return 1;
        }
        perror("accept");
        return 0;
    }

    if (!receive_line(connection_fd, frame, sizeof(frame))) {
        close(connection_fd);
        log_message(receiver, "ERREUR", "Trame OM vide ou incomplete");
        return 1;
    }
    close(connection_fd);

    if (!parse_message(frame, n, receiver, m, &record, &sender)) {
        log_message(receiver, "ERREUR", "Trame OM invalide");
        return 1;
    }

    stored = store_record(records, capacity, count, &record);
    if (stored < 0) {
        log_message(receiver, "ERREUR", "Message OM duplique ou capacite depassee");
        return 1;
    }
    if (stored == 0) {
        return 1;
    }

    {
        char path_text[MAX_GENERALS * 4];
        char details[256];
        format_path(record.path, record.depth, path_text, sizeof(path_text));
        snprintf(details, sizeof(details),
                 "round=%u path=%s value=%s source=%d",
                 (unsigned int) record.depth, path_text,
                 record.value == VOTE_ATTACK ? "Attaque" : "Retraite", sender);
        log_message(receiver, "OM_RECEPTION", details);
    }
    return 1;
}

static size_t count_records_at_depth(const message_record_t *records, size_t count, int depth) {
    size_t found = 0;
    size_t i;

    for (i = 0; i < count; i++) {
        if (records[i].depth == depth) {
            found++;
        }
    }
    return found;
}

static int receive_depth(int server_fd, int receiver, int n, int m, int depth,
                         size_t expected, message_record_t *records,
                         size_t capacity, size_t *count) {
    while (count_records_at_depth(records, *count, depth) < expected) {
        if (!accept_and_store(server_fd, receiver, n, m, records, capacity, count)) {
            return 0;
        }
    }
    return 1;
}

static vote_t traitor_message(int sender, int receiver, int depth,
                              const uint8_t *path, int path_depth) {
    unsigned int parity = (unsigned int) (sender + receiver + depth);
    int i;

    for (i = 0; i < path_depth; i++) {
        parity += path[i];
    }
    return (parity % 2 == 0) ? VOTE_ATTACK : VOTE_RETREAT;
}

static int send_round_with_recipient(int id, int n, int round, int faulty,
                                     const message_record_t *records, size_t count) {
    size_t i;
    int receiver;

    for (i = 0; i < count; i++) {
        uint8_t path[MAX_GENERALS] = {0};
        int path_depth;

        if (records[i].depth != round - 1 ||
            path_contains(records[i].path, records[i].depth, id)) {
            continue;
        }

        path_depth = round;
        memcpy(path, records[i].path, records[i].depth);
        path[path_depth - 1] = (uint8_t) id;

        for (receiver = 1; receiver < n; receiver++) {
            vote_t value;
            if (receiver == id || path_contains(path, path_depth, receiver)) {
                continue;
            }

            value = faulty
                ? traitor_message(id, receiver, round, records[i].path, records[i].depth)
                : records[i].value;
            if (!send_frame(id, receiver, path_depth, path, value)) {
                char details[160];
                snprintf(details, sizeof(details),
                         "Echec d'envoi round=%d au lieutenant %d", round, receiver);
                log_message(id, "ERREUR", details);
                return 0;
            }
        }
    }
    return 1;
}

static int evaluate_subtree(int receiver, int n, int remaining,
                            uint8_t *path, int depth,
                            const message_record_t *records, size_t count,
                            int *result) {
    int index = find_record(records, count, path, depth);
    int votes_attack;
    int votes_retreat;
    int candidate;

    if (index < 0) {
        return 0;
    }
    if (remaining == 0) {
        *result = records[index].value;
        return 1;
    }

    votes_attack = records[index].value == VOTE_ATTACK;
    votes_retreat = records[index].value == VOTE_RETREAT;
    for (candidate = 1; candidate < n; candidate++) {
        int child_result;

        if (candidate == receiver || path_contains(path, depth, candidate)) {
            continue;
        }

        path[depth] = (uint8_t) candidate;
        if (!evaluate_subtree(receiver, n, remaining - 1, path, depth + 1,
                              records, count, &child_result)) {
            return 0;
        }
        if (child_result == VOTE_ATTACK) {
            votes_attack++;
        } else {
            votes_retreat++;
        }
    }

    *result = votes_attack > votes_retreat ? VOTE_ATTACK : VOTE_RETREAT;
    return 1;
}

static int calculate_decision(int id, int n, int m,
                              const message_record_t *records, size_t count,
                              vote_t *decision, int *votes_attack, int *votes_retreat) {
    uint8_t path[MAX_GENERALS] = {0};
    int root_index = find_record(records, count, path, 0);
    int candidate;

    if (root_index < 0) {
        return 0;
    }

    *votes_attack = records[root_index].value == VOTE_ATTACK;
    *votes_retreat = records[root_index].value == VOTE_RETREAT;

    for (candidate = 1; candidate < n; candidate++) {
        int value;

        if (candidate == id || m == 0) {
            continue;
        }

        path[0] = (uint8_t) candidate;
        if (!evaluate_subtree(id, n, m - 1, path, 1,
                              records, count, &value)) {
            return 0;
        }
        if (value == VOTE_ATTACK) {
            (*votes_attack)++;
        } else {
            (*votes_retreat)++;
        }
    }

    *decision = *votes_attack > *votes_retreat ? VOTE_ATTACK : VOTE_RETREAT;
    return 1;
}

static int act_as_commander(int n, int faulty) {
    int receiver;

    log_message(0, "OM_DEMARRAGE", "Commandant demarre OM(m)");
    for (receiver = 1; receiver < n; receiver++) {
        uint8_t empty_path[MAX_GENERALS] = {0};
        vote_t value = faulty
            ? traitor_message(0, receiver, 0, empty_path, 0)
            : VOTE_ATTACK;

        if (!send_frame(0, receiver, 0, empty_path, value)) {
            char details[128];
            snprintf(details, sizeof(details),
                     "Impossible de contacter le lieutenant %d", receiver);
            log_message(0, "ERREUR", details);
            return 0;
        }
    }
    return 1;
}

static int act_as_lieutenant(int id, int n, int m, int faulty,
                             size_t capacity, vote_t *decision,
                             int *votes_attack, int *votes_retreat) {
    int server_fd = create_listener(id);
    message_record_t *records;
    size_t record_count = 0;
    int round;
    int result = 0;

    if (server_fd < 0) {
        return 0;
    }

    records = calloc(capacity, sizeof(*records));
    if (records == NULL) {
        perror("calloc messages");
        close(server_fd);
        return 0;
    }

    if (!receive_depth(server_fd, id, n, m, 0, 1,
                       records, capacity, &record_count)) {
        goto cleanup;
    }

    for (round = 1; round <= m; round++) {
        size_t expected = permutation_count(n - 2, round);

        if (!send_round_with_recipient(id, n, round, faulty, records, record_count)) {
            goto cleanup;
        }
        if (!receive_depth(server_fd, id, n, m, round, expected,
                           records, capacity, &record_count)) {
            goto cleanup;
        }
    }

    if (!calculate_decision(id, n, m, records, record_count,
                            decision, votes_attack, votes_retreat)) {
        log_message(id, "ERREUR", "Arbre OM incomplet, vote impossible");
        goto cleanup;
    }
    result = 1;

cleanup:
    free(records);
    close(server_fd);
    return result;
}

int main(int argc, char *argv[]) {
    int n;
    int m;
    int traitors[MAX_GENERALS] = {0};
    size_t record_capacity;
    pid_t children[MAX_GENERALS];
    int child_count = 0;
    int i;
    int failures = 0;

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s <n> <m> [liste_ids_traitres]\n", argv[0]);
        fprintf(stderr, "Exemple: %s 7 2 0,3 (commandant 0 et lieutenant 3)\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (!parse_integer(argv[1], 3, MAX_GENERALS, &n) ||
        !parse_integer(argv[2], 0, MAX_GENERALS - 1, &m) ||
        m > n - 2) {
        fprintf(stderr, "Erreur: il faut 3 <= n <= %d et 0 <= m <= n-2.\n",
                MAX_GENERALS);
        return EXIT_FAILURE;
    }
    if ((m == 0 && argc == 4) ||
        !parse_traitor_ids(argc == 4 ? argv[3] : NULL, n, m, traitors)) {
        fprintf(stderr, "Erreur: fournissez exactement m IDs distincts entre 0 et n-1.\n");
        return EXIT_FAILURE;
    }
    if (!calculate_record_capacity(n, m, &record_capacity)) {
        fprintf(stderr,
                "Erreur: OM(%d) avec n=%d depasse la limite de %d chemins par processus.\n",
                m, n, MAX_RECORDS_PER_PROCESS);
        return EXIT_FAILURE;
    }

    if (3 * m >= n) {
        fprintf(stderr,
                "Avertissement: 3*m >= n; la borne n >= 3m+1 est violee, "
                "l'accord et la validite ne sont pas garantis.\n");
    }

    printf("Simulation OM(%d), n=%d, m=%d, tours=%d\n", m, n, m, m + 1);
    printf("IDs traitres :");
    for (i = 0; i < m; i++) {
        printf(" %d", traitors[i]);
    }
    printf("\n");

    signal(SIGPIPE, SIG_IGN);
    fflush(stdout);

    for (i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            failures = 1;
            break;
        }
        if (pid == 0) {
            char details[256];
            int faulty = is_traitor(i, m, traitors);
            int status;
            vote_t decision = VOTE_RETREAT;
            int votes_attack = 0;
            int votes_retreat = 0;

            log_init("logs/trace_globale.log");
            snprintf(details, sizeof(details),
                     "General %d role=%s status=%s",
                     i, i == 0 ? "COMMANDANT" : "LIEUTENANT",
                     faulty ? "TRAITRE" : "LOYAL");
            log_message(i, "DEMARRAGE", details);

            if (i == 0) {
                status = act_as_commander(n, faulty);
            } else {
                status = act_as_lieutenant(i, n, m, faulty, record_capacity,
                                           &decision, &votes_attack, &votes_retreat);
                if (status) {
                    snprintf(details, sizeof(details),
                             "Vote : %d Attaque, %d Retraite -> Decision : %s",
                             votes_attack, votes_retreat,
                             decision == VOTE_ATTACK ? "Attaque" : "Retraite");
                    log_message(i, "VOTE", details);
                    snprintf(details, sizeof(details), "Ma decision finale est : %s",
                             decision == VOTE_ATTACK ? "Attaque" : "Retraite");
                    log_message(i, "DECISION_FINALE", details);
                }
            }

            log_message(i, status ? "FIN" : "ECHEC",
                        status ? "Processus termine" : "Echec de la simulation OM");
            log_close();
            _exit(status ? EXIT_SUCCESS : EXIT_FAILURE);
        }

        children[child_count++] = pid;
    }

    for (i = 0; i < child_count; i++) {
        int child_status = 0;
        pid_t waited;

        do {
            waited = waitpid(children[i], &child_status, 0);
        } while (waited < 0 && errno == EINTR);

        if (waited < 0 || !WIFEXITED(child_status) ||
            WEXITSTATUS(child_status) != EXIT_SUCCESS) {
            failures = 1;
        }
    }

    if (failures) {
        fprintf(stderr, "La simulation s'est terminee avec au moins une erreur. "
                        "Consultez logs/trace_globale.log.\n");
        return EXIT_FAILURE;
    }

    printf("Tous les processus sont termines. "
           "Consultez logs/trace_globale.log\n");
    return EXIT_SUCCESS;
}
