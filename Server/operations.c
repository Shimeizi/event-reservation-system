#include "operations.h"

///////////////////////////////////////////DIRECTORY OPERATIONS////////////////////////////////////////////////////
// Criar diretorio
void create_directory(const char *path) {
    struct stat st;

    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return;

    if (mkdir(path, 0755) != 0)
        perror("mkdir");
}

// Criar diretorio de um USER (uid)
void create_user_structure(int uid) {
    char path[128];

    snprintf(path, sizeof(path), "USERS/%06d", uid);
    create_directory(path);

    snprintf(path, sizeof(path), "USERS/%06d/CREATED", uid);
    create_directory(path);

    snprintf(path, sizeof(path), "USERS/%06d/RESERVED", uid);
    create_directory(path);
}

// Criar ficheiros do USER
void create_user_pass_file(int uid, const char *password) {
    char path[128];
    snprintf(path, sizeof(path), "USERS/%06d/%06d_pass.txt", uid, uid);

    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "%s\n", password);
    fclose(f);
}

// logged / not logged
void create_user_login_file(int uid) {
    char path[128];
    snprintf(path, sizeof(path), "USERS/%06d/%06d_login.txt", uid, uid);

    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "0\n");  // 0 = not logged, 1 = logged
    fclose(f);
}

// Criar estrutura de um EVENTO (eid)
void create_event_structure(int eid) {
    char path[128];

    snprintf(path, sizeof(path), "EVENTS/%03d", eid);
    create_directory(path);

    snprintf(path, sizeof(path), "EVENTS/%03d/DESCRIPTION", eid);
    create_directory(path);

    snprintf(path, sizeof(path), "EVENTS/%03d/RESERVATIONS", eid);
    create_directory(path);
}

// Criar ficheiros do EVENTO
void create_event_files(int eid) {
    char path[128];
    FILE *f;

    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    if ((f = fopen(path, "w"))) fclose(f);

    snprintf(path, sizeof(path), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    if ((f = fopen(path, "w"))) fclose(f);
}

// Criar Reserva
void create_reservation(int uid, int eid, const char *date) {
    char path[256];
    FILE *f;

    snprintf(path, sizeof(path),
        "USERS/%06d/RESERVED/R-%06d-%s.txt", uid, uid, date);
    if ((f = fopen(path, "w"))) fclose(f);

    snprintf(path, sizeof(path),
        "EVENTS/%03d/RESERVATIONS/R-%06d-%s.txt", eid, uid, date);
    if ((f = fopen(path, "w"))) fclose(f);
}

////////////////////////////////////////////CHECK COMMANDS/////////////////////////////////////////////////////

int check_command_UID_Password(char *input[], int count) {
    int i;

    if (count != 3)
        return NOT_VALID;

    if (strlen(input[1]) != 6 || !is_all_digits(input[1]))
        return NOT_VALID;

    if (strlen(input[2]) == 0)
        return NOT_VALID;

    for (i = 0; input[2][i] != '\0'; i++) {
        if (!isalnum((unsigned char)input[2][i]))
            return NOT_VALID;
    }

    return VALID;
}


//////////////////////////////////////////////AUXILIARY FUNCTIONS/////////////////////////////////////////

void update_user_login(int uid, int status) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%06d/%06d_login.txt", BASE_DIR, uid, uid);
    if (status == 1) {
        FILE *f = fopen(path, "w");
        if (!f) {
            perror("fopen login");
            exit(1);
        }
        fprintf(f, "1\n");
        fclose(f);
    } else {
        // Remover ficheiro quando faz logout
        if (unlink(path) == -1 && errno != ENOENT) {
            perror("unlink login");
        }
    }
}

// Ler password existente
int read_user_pass(int uid, char *buffer, size_t size) {
    char path[128];
    snprintf(path, sizeof(path), "%s/%06d/%06d_pass.txt", BASE_DIR, uid, uid);

    FILE *f = fopen(path, "r");
    if (!f) return 0; // ficheiro não existe

    if (fgets(buffer, size, f) == NULL) {
        fclose(f);
        return 0;
    }
    buffer[strcspn(buffer, "\n")] = 0; // remove newline
    fclose(f);
    return 1;
}

int is_user_logged(int uid) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%06d/%06d_login.txt", BASE_DIR, uid, uid);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    return 1; 
}

// Eliminate redundant space in the end of string
void trim_spaces(char *str) {
    int start = 0, end = strlen(str) - 1;

    // Trim leading spaces
    while (isspace((unsigned char)str[start])) {
        start++;
    }

    // Trim trailing spaces
    while (end >= start && isspace((unsigned char)str[end])) {
        end--;
    }

    // Shift the trimmed string to the beginning
    int i = 0;
    while (start <= end) {
        str[i++] = str[start++];
    }
    str[i] = '\0'; // Null-terminate the trimmed string
}

int is_all_digits(const char *str) {
    int len = strlen(str);
    for (int i = 0; i < len - 1; i++) {
        if (!isdigit(str[i])) {
            return NOT_VALID;
        }
    }
    return VALID;
}

int user_exists(int uid) {
    char path[128];
    struct stat st;

    snprintf(path, sizeof(path), "USERS/%06d", uid);
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int check_user_password(int uid, const char *password) {
    char path[128], stored[64];
    FILE *f;

    snprintf(path, sizeof(path), "USERS/%06d/%06d_pass.txt", uid, uid);

    f = fopen(path, "r");
    if (!f) return 0;

    fscanf(f, "%s", stored);
    fclose(f);

    return strcmp(stored, password) == 0;
}

// Percorre a diretoria e devolve o next-eid
int get_next_eid(void) {
    static int current_eid = 1;
    char path[128];
    struct stat st;

    while (current_eid <= 999) {
        snprintf(path, sizeof(path), "EVENTS/%03d", current_eid);

        if (stat(path, &st) != 0) {
            return current_eid++;
        }
        current_eid++;
    }

    return -1; // Nao ha EIDs disponiveis
}

int event_is_past(int eid) {
    char path[256];
    FILE *f;
    int day, month, year, hour, min;
    struct tm event_time;
    time_t now, event_t;

    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);

    f = fopen(path, "r");
    if (!f)
        return 0;

    // START format: UID name fname num_attendance date time
    char linebuf[128];
    int uid = 0;
    char name[64] = {0}, fname[64] = {0}, date[16] = {0}, time_s[8] = {0};
    int capacity = 0;
    if (!fgets(linebuf, sizeof(linebuf), f)) { fclose(f); return 0; }
    if (sscanf(linebuf, "%d %63s %63s %d %15s %7s", &uid, name, fname, &capacity, date, time_s) != 6) { fclose(f); return 0; }
    if (sscanf(date, "%2d-%2d-%4d", &day, &month, &year) != 3) { fclose(f); return 0; }
    if (sscanf(time_s, "%2d:%2d", &hour, &min) != 2) { fclose(f); return 0; }
    fclose(f);

    /* Construir struct tm */
    memset(&event_time, 0, sizeof(struct tm));
    event_time.tm_mday = day;
    event_time.tm_mon  = month - 1;     // meses: 0–11
    event_time.tm_year = year - 1900;   // anos desde 1900
    event_time.tm_hour = hour;
    event_time.tm_min  = min;

    event_t = mktime(&event_time); // Converte a data do evento para segundos desde 1970 
    now = time(NULL); // Obtém o tempo (em segundos)

    // Evento no passado
    return difftime(event_t, now) < 0;
}

int event_is_sold_out(int eid) {
    char path[256];
    int capacity = 0;
    int reserved = 0;

    // Read capacity from START_%03d.txt 
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    FILE *sf = fopen(path, "r");
    if (!sf) return 0; // can't read => not sold out by default
    char linebuf[128];
    int uid = 0;
    char name[64] = {0}, fname[64] = {0}, date[16] = {0}, time_s[8] = {0};
    if (!fgets(linebuf, sizeof(linebuf), sf)) { fclose(sf); return 0; }
    if (sscanf(linebuf, "%d %63s %63s %d %15s %7s", &uid, name, fname, &capacity, date, time_s) != 6) { fclose(sf); return 0; }
    fclose(sf);

    // Read reserved seats from RES_%03d.txt
    snprintf(path, sizeof(path), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    FILE *rf = fopen(path, "r");
    if (rf) {
        char line[256] = {0};
        long start_pos = ftell(rf);
        if (fgets(line, sizeof(line), rf)) {
            // Normalize first line by stripping CR/LF
            line[strcspn(line, "\r\n")] = '\0';
            if (strcmp(line, "0") == 0) {
                int c = fgetc(rf);
                if (c == EOF) {
                    // Only placeholder zero exists
                    reserved = 0;
                    fclose(rf);
                    return (capacity > 0) && (reserved >= capacity);
                }
                // Else: there are more lines (records) after the zero; we'll sum records below
            }

            // Sum all reservation records
            reserved = 0;
            fseek(rf, start_pos, SEEK_SET);
            while (fgets(line, sizeof(line), rf)) {
                // Skip pure placeholder zero line
                char tmp[256];
                strncpy(tmp, line, sizeof(tmp) - 1);
                tmp[sizeof(tmp) - 1] = '\0';
                tmp[strcspn(tmp, "\r\n")] = '\0';
                // Trim leading spaces
                char *p = tmp;
                while (isspace((unsigned char)*p)) p++;
                if (strcmp(p, "0") == 0) continue;

                int rec_uid = 0, rec_people = 0;
                char rec_date[16] = {0}, rec_time[16] = {0};
                if (sscanf(tmp, "%d %d %15s %15s", &rec_uid, &rec_people, rec_date, rec_time) == 4) {
                    if (rec_people > 0) reserved += rec_people;
                }
            }
        }
        fclose(rf);
    } else {
        reserved = 0; // if not present, assume none reserved
    }

    return (capacity > 0) && (reserved >= capacity);
}

int event_is_closed(int eid) {
    char path[256];

    snprintf(path, sizeof(path),
             "EVENTS/%03d/END_%03d.txt", eid, eid);

    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1; // fechado
    }

    return 0; // aberto
}

int get_event_remaining_seats(int eid) {
    char path[256];
    int capacity = 0;
    int reserved = 0;

    // Read total capacity from START_%03d.txt 
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    FILE *sf = fopen(path, "r");
    if (!sf) return 0;
    int uid = 0;
    char name[64] = {0}, fname[64] = {0}, date[16] = {0}, time_s[8] = {0};
    if (fscanf(sf, "%d %63s %63s %d %15s %7s", &uid, name, fname, &capacity, date, time_s) != 6) {
        fclose(sf);
        return 0;
    }
    fclose(sf);

    // Get reserved seats by reading RES_%03d.txt
    snprintf(path, sizeof(path), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    FILE *rf = fopen(path, "r");
    if (rf) {
        char line[256] = {0};
        long start_pos = ftell(rf);
        if (fgets(line, sizeof(line), rf)) {
            // Normalize first line and check if it's exactly "0" and file has no more content
            line[strcspn(line, "\r\n")] = '\0';
            int only_zero = (strcmp(line, "0") == 0);
            int nextc = fgetc(rf);
            if (only_zero && nextc == EOF) {
                reserved = 0;
            } else {
                // Sum all records
                reserved = 0;
                fseek(rf, start_pos, SEEK_SET);
                while (fgets(line, sizeof(line), rf)) {
                    // Strip CR/LF and trim
                    line[strcspn(line, "\r\n")] = '\0';
                    char *p = line;
                    while (isspace((unsigned char)*p)) p++;
                    if (strcmp(p, "0") == 0) continue; // skip placeholder zero line

                    int rec_uid = 0, rec_people = 0;
                    char rec_date[16] = {0}, rec_time[16] = {0};
                    if (sscanf(p, "%d %d %15s %15s", &rec_uid, &rec_people, rec_date, rec_time) == 4) {
                        if (rec_people > 0) reserved += rec_people;
                    }
                }
            }
        }
        fclose(rf);
    } else {
        reserved = 0; // If RES file is missing, assume 0
    }

    int remaining = capacity - reserved;
    if (remaining < 0) remaining = 0;
    return remaining;
}

static int dirent_name_cmp(const struct dirent **a, const struct dirent **b) {
    return strcmp((*a)->d_name, (*b)->d_name);
}

//////////////////////////////////////////////SET UP TCP/UDP////////////////////////////////////////////////////

// Function to initialize the UDP socket
int setup_udp_socket(struct addrinfo *hints, struct addrinfo **res) {
    int udp_fd;
    int optval = 1;

    memset(hints, 0, sizeof(*hints));
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_DGRAM;
    hints->ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    if (getaddrinfo(NULL, DEFAULT_PORT, hints, res) != 0) {
        perror("getaddrinfo error (UDP)");
        exit(1);
    }

    udp_fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (udp_fd == -1) {
        perror("UDP socket error");
        exit(1);
    }

    // Set SO_REUSEADDR to allow reuse of the port
    if (setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt error (SO_REUSEADDR)");
        exit(1);
    }

    if (bind(udp_fd, (*res)->ai_addr, (*res)->ai_addrlen) == -1) {
        fprintf(stderr, "bind error (UDP) on port %s\n", DEFAULT_PORT);
        exit(1);
    }

    freeaddrinfo(*res);

    printf("UDP socket successfully bound to port %s\n", DEFAULT_PORT);
    return udp_fd;
}


// Function to initialize the TCP socket
int setup_tcp_socket(struct addrinfo *hints, struct addrinfo **res) {
    int tcp_fd;
    int optval = 1; // Option value for SO_REUSEADDR

    memset(hints, 0, sizeof(*hints));
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    if (getaddrinfo(NULL, DEFAULT_PORT, hints, res) != 0) {
        perror("getaddrinfo error (TCP)");
        exit(1);
    }

    tcp_fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (tcp_fd == -1) {
        perror("TCP socket error");
        exit(1);
    }

    // Set SO_REUSEADDR to allow reuse of the port
    if (setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt error (SO_REUSEADDR)");
        exit(1);
    }

    if (bind(tcp_fd, (*res)->ai_addr, (*res)->ai_addrlen) == -1) {
        perror("bind error (TCP)");
        exit(1);
    }

    freeaddrinfo(*res);

    if (listen(tcp_fd, 5) == -1) {
        perror("listen error");
        exit(1);
    }

    printf("TCP socket bound to port %s and listening...\n", DEFAULT_PORT);

    return tcp_fd;
}

//////////////////////////////////////////////HANDLE UDP////////////////////////////////////////////////////

// Function to handle UDP requests
void handle_udp_requests(int udp_fd) {
    struct sockaddr_in client_addr; 
    socklen_t addrlen = sizeof(client_addr);

    char buffer[BUFFER_SIZE]; 
    int ret = recvfrom(udp_fd, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&client_addr, &addrlen);

    if(ret <= 0){ 
        perror("recvfrom"); 
        return; 
    }

    buffer[ret] = '\0';

    char *words[MAX_WORDS]; 
    int count = 0; 
    char *word = strtok(buffer," ");

    while(word != NULL && count < MAX_WORDS){ 
        trim_spaces(word); 
        words[count++] = word; 
        word = strtok(NULL," "); 
    }

    if(count == 0) return;

    int uid = count > 1 ? atoi(words[1]) : 0;

    if(strcmp(words[0],"LIN") == 0 && count>=3) {
        handle_login(udp_fd,client_addr,addrlen,uid,words[2]);
    }
    else if(strcmp(words[0],"LOU") == 0 && count >= 3) {
        handle_logout(udp_fd,client_addr,addrlen,uid,words[2]);
    }
    else if(strcmp(words[0],"UNR") == 0 && count >= 3) {
        handle_unregister(udp_fd,client_addr,addrlen,uid,words[2]);
    }
    else if(strcmp(words[0],"LME") == 0 && count >= 3) {
        handle_myevents(udp_fd,client_addr,addrlen,uid,words[2]);
    }
    else if(strcmp(words[0],"LMR") == 0 && count>=3) {
        handle_myreservations(udp_fd,client_addr,addrlen,uid,words[2]);
    }
    else {
        perror("recvfrom error");
    }
}

// COMMAND LOGIN
void handle_login(int udp_fd, struct sockaddr_in client_addr, socklen_t addrlen, int uid, char *password) {
    char pass_buffer[128];
    char response[32];

    char user_dir[128];
    snprintf(user_dir, sizeof(user_dir), "%s/%06d", BASE_DIR, uid);
    struct stat st;

    if (stat(user_dir, &st) != 0) {
        // Novo utilizador
        create_user_structure(uid);
        create_user_pass_file(uid, password);
        update_user_login(uid, 1);
        strcpy(response, "RLI REG");
    } else {
        if (!read_user_pass(uid, pass_buffer, sizeof(pass_buffer))) {
            // Novo registo
            create_user_pass_file(uid, password);
            update_user_login(uid, 1);
            strcpy(response, "RLI REG");
        } else if (strcmp(pass_buffer, password) == 0) {
            // Password correta
            update_user_login(uid, 1);
            strcpy(response, "RLI OK");
        } else {
            // Password incorreta
            strcpy(response, "RLI NOK");
        }
    }
    sendto(udp_fd, response, strlen(response), 0, (struct sockaddr*)&client_addr, addrlen);
}

// COMMAND LOGOUT
void handle_logout(int udp_fd, struct sockaddr_in client_addr, socklen_t addrlen, int uid, char *password) {
    char response[32];
    char pass_buffer[128];

    char user_dir[128];
    snprintf(user_dir, sizeof(user_dir), "%s/%06d", BASE_DIR, uid);
    struct stat st;
    if (stat(user_dir, &st) != 0) {
        strcpy(response, "RLO UNR");
    } else if (!read_user_pass(uid, pass_buffer, sizeof(pass_buffer))) {
        strcpy(response, "RLO WRP");
    } else if (strcmp(pass_buffer, password) != 0) {
        strcpy(response, "RLO WRP");
    } else if (!is_user_logged(uid)) {
        strcpy(response, "RLO NOK");
    } else {
        update_user_login(uid, 0);
        strcpy(response, "RLO OK");
    }
    sendto(udp_fd, response, strlen(response), 0, (struct sockaddr*)&client_addr, addrlen);
}

// COMMAND UNREGISTER
void handle_unregister(int udp_fd, struct sockaddr_in client_addr, socklen_t addrlen, int uid, char *password) {
    char response[32];
    char pass_buffer[128];

    char user_dir[128];
    snprintf(user_dir, sizeof(user_dir), "%s/%06d", BASE_DIR, uid);
    struct stat st;
    if (stat(user_dir, &st) != 0) {
        strcpy(response, "RUR UNR");
    } else if (!read_user_pass(uid, pass_buffer, sizeof(pass_buffer))) {
        strcpy(response, "RUR WRP");
    } else if (strcmp(pass_buffer, password) != 0) {
        strcpy(response, "RUR WRP");
    } else if (!is_user_logged(uid)) {
        strcpy(response, "RUR NOK");
    } else {
        // Remover apenas pass e login; preservar diretórios
        char pass_path[256], login_path[256];
        snprintf(pass_path, sizeof(pass_path), "%s/%06d/%06d_pass.txt", BASE_DIR, uid, uid);
        snprintf(login_path, sizeof(login_path), "%s/%06d/%06d_login.txt", BASE_DIR, uid, uid);
        if (unlink(pass_path) == -1 && errno != ENOENT) perror("unlink pass");
        if (unlink(login_path) == -1 && errno != ENOENT) perror("unlink login");
        strcpy(response, "RUR OK");
    }
    sendto(udp_fd, response, strlen(response), 0, (struct sockaddr*)&client_addr, addrlen);
}

// COMMAND MYEVENTS
void handle_myevents(int udp_fd, struct sockaddr_in addr, socklen_t addrlen,
                     int uid, const char *password) {

    char response[2048] = "RME ";
    char pass_stored[64];
    char path[256];

    // Verificar se utilizador existe
    snprintf(path, sizeof(path), "USERS/%06d", uid);
    if (access(path, F_OK) != 0) {
        strcat(response, "NOK");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }

    // Verificar password
    snprintf(path, sizeof(path),
             "USERS/%06d/%06d_pass.txt", uid, uid);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        strcat(response, "WRP");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }
    fscanf(fp, "%s", pass_stored);
    fclose(fp);

    if (strcmp(pass_stored, password) != 0) {
        strcat(response, "WRP");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }

    // Verificar se está logged in
    snprintf(path, sizeof(path),
             "USERS/%06d/%06d_login.txt", uid, uid);
    if (access(path, F_OK) != 0) {
        strcat(response, "NLG");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }

    // Listar eventos criados 
    snprintf(path, sizeof(path), "USERS/%06d/CREATED", uid);
    struct dirent **namelist = NULL;
    int n = scandir(path, &namelist, NULL, dirent_name_cmp);
    if (n < 0) {
        strcat(response, "NOK");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }

    int found = 0;
    for (int i = 0; i < n; i++) {
        struct dirent *entry = namelist[i];
        // Ignore "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { free(entry); continue; }
        // Expect files named like "001.txt"; ensure starts with digit
        if (!isdigit((unsigned char)entry->d_name[0])) { free(entry); continue; }

        // Extract EID 
        int eid = 0;
        {
            char eidstr[4] = {0};
            strncpy(eidstr, entry->d_name, 3);
            eidstr[3] = '\0';
            eid = atoi(eidstr);
        }

        // Compute state like handle_list_events
        int state = 1; // open by default

        // Paths under EVENTS
        char start_path[256];
        snprintf(start_path, sizeof(start_path), "EVENTS/%03d/START_%03d.txt", eid, eid);
        FILE *sf = fopen(start_path, "r");
        if (!sf) { free(entry); continue; }
        char linebuf[256];
        int owner_uid = 0;
        char name[64] = {0};
        char fname[128] = {0};
        int capacity = 0;
        char date[16] = {0};
        char time_s[8] = {0};
        if (!fgets(linebuf, sizeof(linebuf), sf)) { fclose(sf); free(entry); continue; }
        /* START format now: UID name fname num_attendance date time */
        if (sscanf(linebuf, "%d %63s %127s %d %15s %7s", &owner_uid, name, fname, &capacity, date, time_s) != 6) { fclose(sf); free(entry); continue; }
            fclose(sf);

        if (event_is_closed(eid)) {
            state = 3; // closed
        } else if (event_is_past(eid)) {
            state = 0; // past
        } else if (event_is_sold_out(eid)) {
            state = 2; // sold out
        }

        if (!found) {
            strcat(response, "OK");
        }
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " %03d %d", eid, state);
        strcat(response, tmp);
        found = 1;
        free(entry);
    }
    free(namelist);

    if (!found) {
        strcpy(response, "RME NOK\n");
    } else {
        strcat(response, "\n");
    }

    sendto(udp_fd, response, strlen(response), 0,
           (struct sockaddr *)&addr, addrlen);
}

// COMMAND MYRESEVATIONS
void handle_myreservations(int udp_fd, struct sockaddr_in addr, socklen_t addrlen,
                           int uid, const char *password) {

    char response[2048] = "RMR ";
    char pass_stored[64];
    char path[256];

    // Verificar se utilizador existe
    snprintf(path, sizeof(path), "USERS/%06d", uid);
    if (access(path, F_OK) != 0) {
        strcat(response, "NOK");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }

    // Verificar password
    snprintf(path, sizeof(path),
             "USERS/%06d/%06d_pass.txt", uid, uid);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        strcat(response, "WRP");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }
    fscanf(fp, "%s", pass_stored);
    fclose(fp);

    if (strcmp(pass_stored, password) != 0) {
        strcat(response, "WRP");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }

    // Verificar login
    snprintf(path, sizeof(path),
             "USERS/%06d/%06d_login.txt", uid, uid);
    if (access(path, F_OK) != 0) {
        strcat(response, "NLG");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }

    // Listar reservas
    snprintf(path, sizeof(path),
             "USERS/%06d/RESERVED", uid);
    DIR *dir = opendir(path);
    if (!dir) {
        strcat(response, "NOK");
        sendto(udp_fd, response, strlen(response), 0,
               (struct sockaddr *)&addr, addrlen);
        return;
    }

    struct dirent *entry;
    int found = 0;
    int count = 0;

    strcat(response, "OK");

    while ((entry = readdir(dir)) != NULL && count < 50) {
        // Ignore "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

    int eid = 0, value = 0;
    char date_out[11] = {0}, time_out[9] = {0};
    char date_token[32] = {0}, time_token[32] = {0};

        char file[512];
        snprintf(file, sizeof(file), "%s/%s", path, entry->d_name);

        fp = fopen(file, "r");
        if (!fp) continue;

        // Robust parse: expect four tokens: eid date time value
        char line[128] = {0};
        if (!fgets(line, sizeof(line), fp)) { fclose(fp); continue; }
        fclose(fp);

        if (sscanf(line, "%d %31s %31s %d", &eid, date_token, time_token, &value) != 4) {
            continue; // skip malformed entries
        }

        // Normalize date/time to DD-MM-YYYY and HH:MM:SS
        if (strchr(date_token, '-') && strchr(time_token, ':')) {
            // Already in desired format
            strncpy(date_out, date_token, sizeof(date_out) - 1);
            strncpy(time_out, time_token, sizeof(time_out) - 1);
        } else if (strlen(date_token) == 8 && strlen(time_token) == 6) {
            // Convert from YYYYMMDD and HHMMSS
            // Validate digits
            int ok = 1;
            for (int i = 0; i < 8; i++) if (!isdigit((unsigned char)date_token[i])) { ok = 0; break; }
            for (int i = 0; i < 6; i++) if (!isdigit((unsigned char)time_token[i])) { ok = 0; break; }
            if (!ok) continue;
            // Build DD-MM-YYYY
            snprintf(date_out, sizeof(date_out), "%c%c-%c%c-%c%c%c%c",
                     date_token[6], date_token[7], // DD
                     date_token[4], date_token[5], // MM
                     date_token[0], date_token[1], date_token[2], date_token[3]); // YYYY
            // Build HH:MM:SS
            snprintf(time_out, sizeof(time_out), "%c%c:%c%c:%c%c",
                     time_token[0], time_token[1],
                     time_token[2], time_token[3],
                     time_token[4], time_token[5]);
        } else {
            // Unknown format; skip
            continue;
        }

        char tmp[128];
    snprintf(tmp, sizeof(tmp), " %03d %s %s %d ", eid, date_out, time_out, value);
        strcat(response, tmp);

        found = 1;
        count++;
    }

    closedir(dir);

    if (!found)
        strcpy(response, "RMR NOK\n");
    else
        strcat(response, "\n");

    sendto(udp_fd, response, strlen(response), 0,
           (struct sockaddr *)&addr, addrlen);
}

//////////////////////////////////////////////HANDLE TCP////////////////////////////////////////////////////

// Accept a single TCP connection and dispatch based on the first line command
void handle_tcp_requests(int tcp_fd) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int conn_fd = accept(tcp_fd, (struct sockaddr*)&client_addr, &addrlen);
    if (conn_fd < 0) return;

    char raw[65536];
    ssize_t n = read(conn_fd, raw, sizeof(raw) - 1);
    if (n <= 0) { close(conn_fd); return; }
    raw[n] = '\0';

    // Extract header line (up to first '\n') safely
    char *nl = memchr(raw, '\n', (size_t)n);
    size_t header_len = nl ? (size_t)(nl - raw) : (size_t)n;
    char header[2048];
    size_t copy_len = header_len < sizeof(header) - 1 ? header_len : sizeof(header) - 1;
    memcpy(header, raw, copy_len);
    header[copy_len] = '\0';

    // Tokenize header
    char *words[MAX_WORDS];
    int count = 0;
    char *saveptr = NULL;
    char *tok = strtok_r(header, " ", &saveptr);
    while (tok && count < MAX_WORDS) {
        words[count++] = tok;
        tok = strtok_r(NULL, " ", &saveptr);
    }
    if (count == 0) { close(conn_fd); return; }

    if (strcmp(words[0], "LST") == 0) {
        handle_list_events(conn_fd);
    } else if (strcmp(words[0], "SED") == 0) {
        handle_show_event(words, count, conn_fd);
    } else if (strcmp(words[0], "RID") == 0) {
        handle_reserve_event(words, count, conn_fd);
    } else if (strcmp(words[0], "CPS") == 0) {
        handle_change_password(words, count, conn_fd);
    } else if (strcmp(words[0], "CLS") == 0) {
        handle_close_event(words, count, conn_fd);
    } else if (strcmp(words[0], "CRE") == 0) {
        handle_create_event(words, count, conn_fd, raw, (int)n);
    } else {
        // Unknown; close silently
    }

    close(conn_fd);
}

// COMMAND CLOSE EVENT
void handle_close_event(char *words[], int count, int conn_fd) {
    int uid, eid;
    char path[256];
    FILE *f;

    uid = atoi(words[1]);
    eid = atoi(words[3]);

    // Verifica se user existe 
    if (!user_exists(uid)) {
        write(conn_fd, "RCL NOK\n", 8);
        return;
    }

    // Verifica login
    if (!is_user_logged(uid)) {
        write(conn_fd, "RCL NLG\n", 8);
        return;
    }

    // Verifica password
    if (!check_user_password(uid, words[2])) {
        write(conn_fd, "RCL NOK\n", 8);
        return;
    }

    // Verifica se o evento existe 
    snprintf(path, sizeof(path), "EVENTS/%03d", eid);
    if (access(path, F_OK) != 0) {
        write(conn_fd, "RCL NOE\n", 8);
        return;
    }

    // Verifica se o evento foi criado por UID consultando USERS/uid/CREATED/eid.txt
    snprintf(path, sizeof(path), "USERS/%06d/CREATED/%03d.txt", uid, eid);
    if (access(path, F_OK) != 0) {
        write(conn_fd, "RCL EOW\n", 8);
        return;
    }

    // Evento já fechado
    snprintf(path, sizeof(path), "EVENTS/%03d/END_%03d.txt", eid, eid);
    f = fopen(path, "r");
    if (f) {
        fclose(f);
        write(conn_fd, "RCL CLO\n", 8);
        return;
    }

    // Ler START para obter capacidade, data e hora
    char name[64] = {0}, date[16] = {0}, time_s[8] = {0};
    int capacity = 0;
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    f = fopen(path, "r");
    if (!f) { write(conn_fd, "RCL NOK\n", 8); return; }
    char linebuf[128];
    if (!fgets(linebuf, sizeof(linebuf), f)) { fclose(f); write(conn_fd, "RCL NOK\n", 8); return; }
    if (sscanf(linebuf, "%63s %d %15s %7s", name, &capacity, date, time_s) != 4) { fclose(f); write(conn_fd, "RCL NOK\n", 8); return; }
    fclose(f);

    // Evento no passado?
    int d, m, y, h, min;
    if (sscanf(date, "%2d-%2d-%4d", &d, &m, &y) == 3 && sscanf(time_s, "%2d:%2d", &h, &min) == 2) {
        struct tm tm_event = {0};
        tm_event.tm_mday = d; tm_event.tm_mon = m - 1; tm_event.tm_year = y - 1900; tm_event.tm_hour = h; tm_event.tm_min = min;
        time_t t_event = mktime(&tm_event);
        if (difftime(t_event, time(NULL)) < 0) { write(conn_fd, "RCL PST\n", 8); return; }
    }

    // Evento sold out? Contar reservas
    int reservations = 0;
    snprintf(path, sizeof(path), "EVENTS/%03d/RESERVATIONS", eid);
    DIR *rd = opendir(path);
    if (rd) {
        struct dirent *re;
        while ((re = readdir(rd)) != NULL) {
            if (strcmp(re->d_name, ".") == 0 || strcmp(re->d_name, "..") == 0) continue;
            reservations++;
        }
        closedir(rd);
    }
    if (reservations >= capacity) { write(conn_fd, "RCL SLD\n", 8); return; }

    // Fechar evento: criar END_%03d.txt e registar data/hora de fecho
    snprintf(path, sizeof(path), "EVENTS/%03d/END_%03d.txt", eid, eid);
    f = fopen(path, "w");
    if (!f) {
        write(conn_fd, "RCL NOK\n", 8);
        return;
    }
    // Formato requerido: dd-mm-yyyy HH:MM:SS
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char ts[32];
    if (tm_now) {
        snprintf(ts, sizeof(ts), "%02d-%02d-%04d %02d:%02d:%02d",
                 tm_now->tm_mday, tm_now->tm_mon + 1, tm_now->tm_year + 1900,
                 tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
    } else {
        // Fallback unlikely, but ensures file has a value
        strcpy(ts, "01-01-1970 00:00:00");
    }
    fprintf(f, "%s\n", ts);
    fclose(f);

    write(conn_fd, "RCL OK\n", 7);
}

// COMMAND LIST EVENT

void handle_list_events(int conn_fd) {
    int found = 0;
    struct dirent **namelist;
    int n = scandir("EVENTS", &namelist, NULL, dirent_name_cmp);
    if (n < 0) {
        write(conn_fd, "RLS NOK\n", 8);
        return;
    }

    for (int i = 0; i < n; i++) {
        struct dirent *entry = namelist[i];
        // Only numeric directories are event IDs
        if (!isdigit((unsigned char)entry->d_name[0])) { free(entry); continue; }

        int eid = atoi(entry->d_name);

        // Read START_<eid>.txt: UID name fname num_attendance date time
        char start_path[256];
        snprintf(start_path, sizeof(start_path), "EVENTS/%03d/START_%03d.txt", eid, eid);
        FILE *sf = fopen(start_path, "r");
        if (!sf) continue; // skip malformed event

        char linebuf[512];
        int uid = 0;
        char name[64] = {0};
        char fname[256] = {0};
        int capacity = 0;
        char date[16] = {0};
        char time_s[8] = {0};
        if (!fgets(linebuf, sizeof(linebuf), sf)) { fclose(sf); continue; }
        // Parse exactly one line: <UID> <name> <fname> <capacity> <date> <time>
        if (sscanf(linebuf, "%d %63s %255s %d %15s %7s", &uid, name, fname, &capacity, date, time_s) != 6) {
            fclose(sf);
            continue;
        }
        fclose(sf);

        // Compute state using auxiliary functions: 3=closed, 0=past, 2=sold_out, 1=open
        int state = 1;
        if (event_is_closed(eid)) {
            state = 3;
        } else if (event_is_past(eid)) {
            state = 0;
        } else if (event_is_sold_out(eid)) {
            state = 2;
        }

        if (!found) {
            write(conn_fd, "RLS OK ", 7);
            found = 1;
        }

        char line[256];
        snprintf(line, sizeof(line), "%03d %s %d %s %s ", eid, name, state, date, time_s);
        write(conn_fd, line, strlen(line));
        free(entry);
    }
    free(namelist);

    // Nenhum evento encontrado 
    if (!found) {
        write(conn_fd, "RLS NOK\n", 8);
        return;
    }

    // Fim da lista
    write(conn_fd, "\n", 1);
}

// COMMAND SHOW EVENT
void handle_show_event(char *words[], int count, int conn_fd) {
    // Expected command: SED EID
    if (count != 2) { write(conn_fd, "RSE NOK\n", 8); return; }

    int eid = atoi(words[1]);

    // Verify event directory exists
    char path[512];
    snprintf(path, sizeof(path), "EVENTS/%03d", eid);
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        write(conn_fd, "RSE NOK\n", 8);
        return;
    }

    // Read START_%03d.txt -> UID name fname num_attendance date time
    char start_path[512];
    snprintf(start_path, sizeof(start_path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    FILE *sf = fopen(start_path, "r");
    if (!sf) { write(conn_fd, "RSE NOK\n", 8); return; }

    char linebuf[256];
    char name[64] = {0}, date[16] = {0}, time_s[8] = {0}, start_fname[256] = {0};
    int capacity = 0, uid = 0;
    if (!fgets(linebuf, sizeof(linebuf), sf)) { fclose(sf); write(conn_fd, "RSE NOK\n", 8); return; }
    if (sscanf(linebuf, "%d %63s %255s %d %15s %7s", &uid, name, start_fname, &capacity, date, time_s) != 6) {
        fclose(sf);
        write(conn_fd, "RSE NOK\n", 8);
        return;
    }
    fclose(sf);

    // Read reserved seats from RES_%03d.txt
    char res_path[512];
    snprintf(res_path, sizeof(res_path), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    FILE *rf = fopen(res_path, "r");
    int reserved = 0;
    if (rf) {
        if (fscanf(rf, "%d", &reserved) != 1) reserved = 0;
        fclose(rf);
    } else {
        reserved = 0;
    }

    struct stat fst;
    long fsize = 0;
    char fpath[768];
    snprintf(fpath, sizeof(fpath), "EVENTS/%03d/DESCRIPTION/%s", eid, start_fname);
    if (stat(fpath, &fst) != 0 || !S_ISREG(fst.st_mode)) { write(conn_fd, "RSE NOK\n", 8); return; }
    fsize = (long)fst.st_size;

    FILE *df = fopen(fpath, "rb");
    if (!df) { write(conn_fd, "RSE NOK\n", 8); return; }

    char header[1024];
    int hn = snprintf(header, sizeof(header),
                      "RSE OK %06d %s %s %s %d %d %s %ld ",
                      uid, name, date, time_s, capacity, reserved, start_fname, fsize);
    if (hn <= 0 || hn >= (int)sizeof(header)) {
        fclose(df);
        write(conn_fd, "RSE NOK\n", 8);
        return;
    }

    // Send header
    if (write(conn_fd, header, (size_t)hn) != (ssize_t)hn) {
        fclose(df);
        return;
    }

    // Send file data
    char buf[4096];
    size_t to_read;
    long remaining = fsize;
    while (remaining > 0 && (to_read = (size_t)((remaining > (long)sizeof(buf)) ? sizeof(buf) : (size_t)remaining)) > 0) {
        size_t r = fread(buf, 1, to_read, df);
        if (r == 0) break;
        ssize_t w = write(conn_fd, buf, r);
        if (w < 0) { break; }
        remaining -= (long)w;
    }
    fclose(df);
}

// COMMAND CREATE EVENT
void handle_create_event(char *words[], int count, int conn_fd, const char *rawbuf, int rawlen) {
    // Expected header tokens: CRE UID password name date time attendance_size Fname Fsize <space> Fdata\n
    if (count < 9) { write(conn_fd, "RCE NOK\n", 8); return; }

    int uid = atoi(words[1]);
    const char *password = words[2];
    const char *name = words[3];
    const char *date = words[4];
    const char *time_s = words[5];
    int capacity = atoi(words[6]);
    const char *fname = words[7];
    long fsize = strtol(words[8], NULL, 10);

    // Basic validations
    if (!user_exists(uid)) { write(conn_fd, "RCE NID\n", 8); return; }
    if (!is_user_logged(uid)) { write(conn_fd, "RCE NLG\n", 8); return; }
    if (!check_user_password(uid, password)) { write(conn_fd, "RCE WRP\n", 8); return; }
    if (capacity < 10 || capacity > 999) { write(conn_fd, "RCE NOK\n", 8); return; }
    if (!fname || *fname == '\0') { write(conn_fd, "RCE NOK\n", 8); return; }
    if (fsize < 0 || fsize > 10000000L) { write(conn_fd, "RCE NOK\n", 8); return; }

    // Determine data offset in rawbuf by reconstructing header prefix
    char prefix[1024];
    int pn = snprintf(prefix, sizeof(prefix), "CRE %s %s %s %s %s %s %s %s ",
                      words[1], words[2], words[3], words[4], words[5], words[6], words[7], words[8]);
    if (pn <= 0 || pn >= (int)sizeof(prefix)) { write(conn_fd, "RCE NOK\n", 8); return; }
    if (rawlen < pn || memcmp(rawbuf, prefix, (size_t)pn) != 0) {
        // If prefix mismatch, fail conservatively
        write(conn_fd, "RCE NOK\n", 8);
        return;
    }

    const char *data_ptr = rawbuf + pn;
    int pre_bytes = rawlen - pn; // bytes of file data already read with header
    if (pre_bytes < 0) pre_bytes = 0;

    // Allocate new EID and create event directories
    int eid = get_next_eid();
    if (eid < 0) { write(conn_fd, "RCE NOK\n", 8); return; }
    create_event_structure(eid);

    // Write START_%03d.txt with: UID event_name fname num_attendance date time
    char path[512];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    FILE *sf = fopen(path, "w");
    if (!sf) { write(conn_fd, "RCE NOK\n", 8); return; }
    fprintf(sf, "%06d %s %s %d %s %s\n", uid, name, fname, capacity, date, time_s);
    fclose(sf);

    // Initialize RES_%03d.txt with 0 reserved
    snprintf(path, sizeof(path), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    FILE *rf = fopen(path, "w");
    if (rf) { fprintf(rf, "0\n"); fclose(rf); }

    // Open DESCRIPTION file and write Fdata
    snprintf(path, sizeof(path), "EVENTS/%03d/DESCRIPTION/%s", eid, fname);
    FILE *df = fopen(path, "wb");
    if (!df) { write(conn_fd, "RCE NOK\n", 8); return; }

    long remaining = fsize;
    // Write the bytes already present in raw buffer
    if (pre_bytes > 0) {
        long to_write = remaining < pre_bytes ? remaining : pre_bytes;
        if (to_write > 0) {
            fwrite(data_ptr, 1, (size_t)to_write, df);
            remaining -= to_write;
        }
    }
    // Read remaining bytes from socket
    while (remaining > 0) {
        char buf[1024];
        int chunk = remaining > (long)sizeof(buf) ? (int)sizeof(buf) : (int)remaining;
        ssize_t r = read(conn_fd, buf, (size_t)chunk);
        if (r <= 0) break;
        fwrite(buf, 1, (size_t)r, df);
        remaining -= r;
    }
    fclose(df);

    // Copy START to USERS/uid/CREATED/eid.txt (metadata for owner)
    snprintf(path, sizeof(path), "USERS/%06d/CREATED/%03d.txt", uid, eid);
    FILE *oc = fopen(path, "w");
    if (oc) { fprintf(oc, "%06d %s %s %d %s %s\n", uid, name, fname, capacity, date, time_s); fclose(oc); }

    // Reply
    char resp[32]; snprintf(resp, sizeof(resp), "RCE OK %03d\n", eid);
    write(conn_fd, resp, strlen(resp));
}

void handle_reserve_event(char *words[], int count, int conn_fd) {
    int uid, eid, people;
    char path[256];
    int remaining;
    FILE *f;

    uid = atoi(words[1]);
    eid = atoi(words[3]);
    people = atoi(words[4]);

    // Verificar se user existe
    if (!user_exists(uid)) {
        write(conn_fd, "RRI NOK\n", 8);
        return;
    }

    //Verificar login 
    if (!is_user_logged(uid)) {
        write(conn_fd, "RRI NLG\n", 8);
        return;
    }

    // Verificar password 
    if (!check_user_password(uid, words[2])) {
        write(conn_fd, "RRI WRP\n", 8);
        return;
    }

    // Verificar se evento existe
    snprintf(path, sizeof(path), "EVENTS/%03d", eid);
    if (access(path, F_OK) != 0) {
        write(conn_fd, "RRI NOK\n", 8);
        return;
    }

    // Evento fechado 
    if (event_is_closed(eid)) {
        write(conn_fd, "RRI CLS\n", 8);
        return;
    }

    // Evento no passado 
    if (event_is_past(eid)) {
        write(conn_fd, "RRI PST\n", 8);
        return;
    }

    // Evento sold out 
    if (event_is_sold_out(eid)) {
        write(conn_fd, "RRI SLD\n", 8);
        return;
    }

    // Lugares restantes 
    remaining = get_event_remaining_seats(eid);  

    if (people > remaining) {
        char reply[64];
        snprintf(reply, sizeof(reply), "RRI REJ %d\n", remaining);
        write(conn_fd, reply, strlen(reply));
        return;
    }

    // Timestamp for filename: YYYYMMDD and HHMMSS
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char date_str[9] = {0}; // YYYYMMDD
    char time_str[7] = {0}; // HHMMSS
    char date_time[32] = {0}; // "YYYYMMDD HHMMSS"
    if (tm_now) {
        snprintf(date_str, sizeof(date_str), "%04d%02d%02d",
                 tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);
        snprintf(time_str, sizeof(time_str), "%02d%02d%02d",
                 tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
        snprintf(date_time, sizeof(date_time), "%s_%s", date_str, time_str);
    }

    // Create reservation file in event RESERVATIONS with name R-uid-YYYYMMDD HHMMSS.txt
    snprintf(path, sizeof(path),
             "EVENTS/%03d/RESERVATIONS/R-%06d-%s.txt", eid, uid, date_time);
    f = fopen(path, "w");
    if (f) fclose(f);

    // Create reservation file for the user; include details to support LMR
    snprintf(path, sizeof(path),
             "USERS/%06d/RESERVED/R-%06d-%s.txt", uid, uid, date_time);
    f = fopen(path, "w");
    if (f) {
        // File content: EID date time people
        fprintf(f, "%03d %s %s %d\n", eid, date_str, time_str, people);
        fclose(f);
    }

    // Update RES_%03d.txt to only store reservation records
    snprintf(path, sizeof(path), "EVENTS/%03d/RES_%03d.txt", eid, eid);

    int is_only_zero = 0;
    FILE *fcheck = fopen(path, "r");
    if (fcheck) {
        char firstline[64] = {0};
        if (fgets(firstline, sizeof(firstline), fcheck)) {
            // Consider variants: "0", "0\n", "0\r\n"
            if (strcmp(firstline, "0\n") == 0 || strcmp(firstline, "0\r\n") == 0 || strcmp(firstline, "0") == 0) {
                // Also ensure no other content exists
                int c = fgetc(fcheck);
                if (c == EOF) is_only_zero = 1;
            }
        }
        fclose(fcheck);
    }

    char correct_date[11];  // DD-MM-YYYY
    char correct_time[9];   // HH:MM:SS
    snprintf(correct_date, sizeof(correct_date), "%02d-%02d-%04d",
             tm_now ? tm_now->tm_mday : 0,
             tm_now ? (tm_now->tm_mon + 1) : 0,
             tm_now ? (tm_now->tm_year + 1900) : 0);
    snprintf(correct_time, sizeof(correct_time), "%02d:%02d:%02d",
             tm_now ? tm_now->tm_hour : 0,
             tm_now ? tm_now->tm_min : 0,
             tm_now ? tm_now->tm_sec : 0);

    // Open for overwrite if only zero, else append
    f = fopen(path, is_only_zero ? "w" : "a");
    if (f) {
        fprintf(f, "%06d %d %s %s\n", uid, people, correct_date, correct_time);
        fclose(f);
    }

    // After recording the reservation, if the event is now sold out, close it automatically
    if (!event_is_closed(eid) && event_is_sold_out(eid)) {
        // Create END_%03d.txt with current date-time: dd-mm-yyyy HH:MM:SS
        snprintf(path, sizeof(path), "EVENTS/%03d/END_%03d.txt", eid, eid);
        FILE *endf = fopen(path, "w");
        if (endf) {
            time_t close_now = time(NULL);
            struct tm *tm_close = localtime(&close_now);
            if (tm_close) {
                fprintf(endf, "%02d-%02d-%04d %02d:%02d:%02d\n",
                        tm_close->tm_mday,
                        tm_close->tm_mon + 1,
                        tm_close->tm_year + 1900,
                        tm_close->tm_hour,
                        tm_close->tm_min,
                        tm_close->tm_sec);
            } else {
                fprintf(endf, "01-01-1970 00:00:00\n");
            }
            fclose(endf);
        }
    }

    // Reply
    char reply[64];
    snprintf(reply, sizeof(reply), "RRI ACC %d\n", people);
    write(conn_fd, reply, strlen(reply));
}

void handle_change_password(char *words[], int count, int conn_fd) {
    int uid;
    char path[256];
    FILE *f;

    // CPS UID oldPassword newPassword
    if (count != 4) {
        write(conn_fd, "RCP NOK\n", 8);
        return;
    }

    uid = atoi(words[1]);

    //  Verificar se utilizador existe
    if (!user_exists(uid)) {
        write(conn_fd, "RCP NID\n", 8);
        return;
    }

    //  Verificar login 
    if (!is_user_logged(uid)) {
        write(conn_fd, "RCP NLG\n", 8);
        return;
    }

    //  Verificar password antiga
    if (!check_user_password(uid, words[2])) {
        write(conn_fd, "RCP NOK\n", 8);
        return;
    }

    // Guardar nova password 
    snprintf(path, sizeof(path), "USERS/%06d/%06d_pass.txt", uid, uid);

    f = fopen(path, "w");
    if (!f) {
        write(conn_fd, "RCP NOK\n", 8);
        return;
    }

    fprintf(f, "%s\n", words[3]);
    fclose(f);

    // Reply
    write(conn_fd, "RCP OK\n", 7);
}

