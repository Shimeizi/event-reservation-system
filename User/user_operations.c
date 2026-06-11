#include "user_operations.h"

/************************* AUXILIARY FUNCTIONS FOR VALIDATION ************************/

int validate_field_count(char *input[], int expected_count) {
    for (int i = 0; i < expected_count; i++) {
        if (input[i] == NULL) {
            return NOT_VALID; // Missing required field
        }
    }
    return (input[expected_count] == NULL) ? VALID : NOT_VALID; // No extra fields allowed
}

int validate_UID(const char *UID) {
    if (strlen(UID) != 6) return NOT_VALID;
    for (int i = 0; i < 6; i++) {
        if (!isdigit(UID[i])) return NOT_VALID;
    }
    return VALID;
}

int validate_EID(const char *EID) {
    if (strlen(EID) != 3) return NOT_VALID;
    for (int i = 0; i < 3; i++) {
        if (!isdigit(EID[i])) return NOT_VALID;
    }
    return VALID;
}

int validate_password(const char *password) {
    if (strlen(password) != 8) return NOT_VALID;
    for (int i = 0; i < 8; i++) {
        if (!isalnum(password[i])) return NOT_VALID;
    }
    return VALID;
}

int validate_event_name(const char *event_name) {
    // Up to 10 alphanumeric characters
    size_t len = strlen(event_name);
    if (len < 1 || len > 10) return NOT_VALID;
    for (size_t i = 0; i < len; i++) {
        if (!isalnum(event_name[i])) return NOT_VALID;
    }
    return VALID;
}

int validate_data_hora(const char *data_hora) {
    int dia, mes, ano, hora, min;

    if (!data_hora || strlen(data_hora) != 16) return NOT_VALID;

    if (sscanf(data_hora, "%2d-%2d-%4d %2d:%2d", &dia, &mes, &ano, &hora, &min) != 5) return NOT_VALID;

    if (data_hora[2] != '-' || data_hora[5] != '-' || data_hora[10] != ' ' || data_hora[13] != ':') return NOT_VALID;

    if (ano < 2025 || ano > 9999) return NOT_VALID;
    if (mes < 1 || mes > 12) return NOT_VALID;

    int dias_no_mes[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (mes == 2) {
        if ((ano % 4 == 0 && ano % 100 != 0) || (ano % 400 == 0)) dias_no_mes[2] = 29;
    }

    if (dia < 1 || dia > dias_no_mes[mes]) return NOT_VALID;
    if (hora < 0 || hora > 23) return NOT_VALID;
    if (min < 0 || min > 59) return NOT_VALID;

    return VALID;
}

int validate_num_lugares(const char *num) {
    int num_lugares = atoi(num);
    if (num_lugares >= 10 && num_lugares <= 999) return VALID;
    return NOT_VALID;
}

char **parse_input(char *input, int *count) {
    char **words = malloc(MAX_WORDS * sizeof(char *));
    char *word = strtok(input, " ");
    while (word != NULL) {
        words[*count] = strdup(word);
        (*count)++;
        word = strtok(NULL, " ");
    }
    words[*count] = NULL;
    return words;
}

void free_words(char **words, int count) {
    for (int i = 0; i < count; i++) {
        free(words[i]);
    }
    free(words);
}

void reset_context(Context *context) {
    context->uid = -1; // DEFAULT UID
    context->logged_in = 0;
    context->my_eids_count = 0;
}

// Map status codes to human-readable messages
static const char* status_to_message(const char *status) {
    if (!status) return "Unknown status";
    // Common statuses
    if (strcmp(status, "OK") == 0) return "Success";
    if (strcmp(status, "NOK") == 0) return "Operation failed";
    if (strcmp(status, "REG") == 0) return "Registered";
    if (strcmp(status, "NLG") == 0) return "User not logged in";
    if (strcmp(status, "WRP") == 0) return "Wrong password";
    if (strcmp(status, "UNR") == 0) return "User not registered";
    if (strcmp(status, "NID") == 0) return "Unknown user";
    if (strcmp(status, "FUL") == 0) return "Event full";
    if (strcmp(status, "NST") == 0) return "Event not started or not available";
    if (strcmp(status, "ACC") == 0) return "Reservation accepted";
    // Fallback: if numeric code, present generic message
    int all_digits = 1; for (const char *p = status; *p; ++p) { if (!isdigit((unsigned char)*p)) { all_digits = 0; break; } }
    if (all_digits) return "Server status code received";
    return "Unknown status";
}

// Map event state codes in server payload to human-readable text
static const char* event_state_to_text(const char *state) {
    if (!state) return "unknown";
    // If it's numeric, translate known values
    int all_digits = 1; for (const char *p = state; *p; ++p) { if (!isdigit((unsigned char)*p)) { all_digits = 0; break; } }
    if (all_digits) {
        if (strcmp(state, "0") == 0) return "finished";   // assumption: event created, not yet open
        if (strcmp(state, "1") == 0) return "open";        // accepting reservations
        if (strcmp(state, "2") == 0) return "sold out";      // no longer accepting reservations
        if (strcmp(state, "3") == 0) return "closed";    // concluded
        return "unknown"; // other numeric codes
    }
    // Non-numeric state, pass-through
    return state;
}

/***************************** CHECK COMMAND FUNCTIONS ****************************/

// LIN UID password
int check_command_login(char *input[]) {
    if (validate_field_count(input, 3) == NOT_VALID) return NOT_VALID;
    if (validate_UID(input[1]) == NOT_VALID) return NOT_VALID;
    if (validate_password(input[2]) == NOT_VALID) return NOT_VALID;
    return VALID;
}

// LOU UID password
int check_command_logout(char *input[]) {
    return validate_field_count(input, 1);
}

// CPS UID OldPassword NewPassword
int check_command_change_password(char *input[]) {
    if (validate_field_count(input, 3) == NOT_VALID) return NOT_VALID;
    if (validate_password(input[1]) == NOT_VALID) return NOT_VALID;
    if (validate_password(input[2]) == NOT_VALID) return NOT_VALID;
    return VALID;
}

// UNR
int check_command_unregister(char *input[]) {
    return validate_field_count(input, 1);
}

// CRE <event_name> <file_name> <data_hora> <num_lugares>
int check_command_create(char *input[]) {
    // Expect exactly 6 tokens: [create, name, fname, date, time, num]
    if (validate_field_count(input, 6) == NOT_VALID) return NOT_VALID;
    if (validate_event_name(input[1]) == NOT_VALID) return NOT_VALID;
    // Build combined date time string to validate format DD-MM-YYYY HH:MM
    char data_hora[17]; // "DD-MM-YYYY HH:MM" + \0
    snprintf(data_hora, sizeof(data_hora), "%s %s", input[3], input[4]);
    if (validate_data_hora(data_hora) == NOT_VALID) return NOT_VALID;
    if (validate_num_lugares(input[5]) == NOT_VALID) return NOT_VALID;
    return VALID;
}

// CLS UID password EID
int check_command_close(char *input[]) {
    if (validate_field_count(input, 2) == NOT_VALID) return NOT_VALID;
    if (validate_EID(input[1]) == NOT_VALID) return NOT_VALID;
    return VALID;
}

// LME UID password
int check_command_myevents(char *input[]) {
    return validate_field_count(input, 1);
}

// LST
int check_command_list(char *input[]) {
    return validate_field_count(input, 1);
}

// SED EID
int check_command_show(char *input[]) {
    if (validate_field_count(input, 2) == NOT_VALID) return NOT_VALID;
    if (validate_EID(input[1]) == NOT_VALID) return NOT_VALID;
    return VALID;
}

// RID UID password EID people
int check_command_reserve(char *input[]) {
    // Local CLI: reserve <EID> <people>
    if (validate_field_count(input, 3) == NOT_VALID) return NOT_VALID;
    if (validate_EID(input[1]) == NOT_VALID) return NOT_VALID;
    // People must be a positive integer (>=1). Upper bound checked server-side against attendance_size.
    int ppl = atoi(input[2]);
    if (ppl <= 0) return NOT_VALID;
    return VALID;
}

// LMR UID password
int check_command_myreservations(char *input[]) {
    // Local CLI: myreservations (no args)
    return validate_field_count(input, 1);
}

/***************************** COMMAND HANDLERS ****************************/

// COMMAND LOGIN

void handle_login(char **words, Context *context) {

    if (!check_command_login(words)) {
        printf("Invalid arguments for command login, usage: login UID password\n");
        return;
    }

    int uid = atoi(words[1]);
    char *password = words[2];

    char *response = send_login_request(context, uid, password);

    handle_login_response(context, uid, password, response);

    free(response);
}


char* send_login_request(Context *context, int uid, char *password) {
    char message[MAX_LINE_LEN];
    sprintf(message, "LIN %d %s\n", uid, password);

    char *dynamic_response =
        send_request_UDP(context->ES_IP, context->ES_port, message);

    dynamic_response[strcspn(dynamic_response, "\n")] = '\0'; 

    return dynamic_response;
}



void handle_login_response(Context *context, int uid, const char *password, char *response) {

    char *word = strtok(response, " ");
    int word_count = 0;

    while (word != NULL) {
        word_count++;

        if (word_count == 1) {
            if (strcmp(word, "RLI") != 0) {
                printf("Unexpected response: %s\n", word);
                return;
            }
        }
        else if (word_count == 2) {
            if (strcmp(word, "OK") == 0) {
                context->uid = uid;
                context->logged_in = 1;
                strncpy(context->password, password, 8);
                context->password[8] = '\0';
                printf("User %d logged in\n", uid);
            }
            else if (strcmp(word, "NOK") == 0) {
                printf("Incorrect password for user %d\n", uid);
            }
            else if (strcmp(word, "REG") == 0) {
                context->uid = uid;
                context->logged_in = 1;
                strncpy(context->password, password, 8);
                context->password[8] = '\0';
                printf("New user %d registered and logged in\n", uid);
            }
            else {
                printf("%s\n", status_to_message(word));
            }
            return;
        }

        word = strtok(NULL, " ");
    }
}

// COMMAND CHANGEPASS
void handle_changepass(char **words, Context *context) {

    if (check_command_change_password(words) == NOT_VALID) {
        printf("Invalid arguments for command changepass\n");
        return;
    }

    if (!context->logged_in) {
        printf("No user is currently logged in\n");
        return;
    }
    char *response = send_changepass_request(words, context);
    if (response == NULL) {
        printf("Failed to receive response from server\n");
        return;
    }

    // Trim trailing newline if present
    response[strcspn(response, "\n")] = '\0';

    handle_changepass_response(context, words[2], response);
    free(response);
}

char* send_changepass_request(char **words, Context *context) {
    char message[MAX_LINE_LEN];

    // CPS UID oldPassword newPassword\n
    sprintf(message, "CPS %06d %s %s\n", context->uid, words[1], words[2]);

    // Use TCP
    char *response = send_request_TCP(
        context->ES_IP ? context->ES_IP : DEFAULT_LOCAL_IP,
        context->ES_port ? context->ES_port : DEFAULT_LOCAL_PORT,
        message
    );

    return response;
}

void handle_changepass_response(Context *context, const char *new_password, char *response) {
    // Expected: RCP status
    char *word = strtok(response, " ");
    if (word == NULL || strcmp(word, CHANGEPASS_RESPONSE) != 0) {
        printf("Unexpected response from server\n");
        return;
    }

    char *status = strtok(NULL, " ");
    if (status == NULL) {
        printf("Invalid response format\n");
        return;
    }

    if (strcmp(status, STATUS_OK) == 0) {
        printf("Password changed successfully\n");
        // Update context password to the new one
        strncpy(context->password, new_password, 8);
        context->password[8] = '\0';
    }
    else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in\n");
    }
    else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Incorrect password\n");
    }
    else if (strcmp(status, "NID") == 0) {
        printf("Unknown user\n");
    }
    else {
        printf("%s\n", status_to_message(status));
    }
}

// COMMAND CREATE
void handle_create(char **words, Context *context) {
    if (!check_command_create(words)) {
        printf("Invalid arguments for command create\n");
        return;
    }

    if (!context->logged_in) {
        printf("No user is currently logged in\n");
        return;
    }

    const char *name = words[1];
    const char *fname = words[2];
    const char *date = words[3];
    const char *time = words[4];
    int num_attendees = atoi(words[5]);

    char *response = send_create_request(context, name, date, time, num_attendees, fname);
    if (!response) {
        printf("Failed to receive response from server\n");
        return;
    }

    response[strcspn(response, "\n")] = '\0';
    handle_create_response(context, response);
    free(response);
}

char* send_create_request(Context *context, const char *name, const char *date, const char *time, int num_attendees, const char *fname) {
    
    FILE *fp = fopen(fname, "rb");
    if (!fp) {
        perror("fopen");
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NULL; }
    long fsize = ftell(fp);
    if (fsize < 0 || fsize > 10000000L) { fclose(fp); printf("File too large (>10MB)\n"); return NULL; }
    rewind(fp);
    char *fdata = (char*)malloc((size_t)fsize);
    if (!fdata) { fclose(fp); return NULL; }
    size_t rd = fread(fdata, 1, (size_t)fsize, fp);
    fclose(fp);
    if (rd != (size_t)fsize) { free(fdata); return NULL; }

    char header[512];
    int hn = snprintf(header, sizeof(header), "CRE %06d %s %s %s %s %d %s %ld ",
                      context->uid, context->password, name, date, time, num_attendees, fname, fsize);
    if (hn <= 0 || hn >= (int)sizeof(header)) { free(fdata); return NULL; }

    int fd, errcode;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    errcode = getaddrinfo(context->ES_IP ? context->ES_IP : DEFAULT_LOCAL_IP,
                          context->ES_port ? context->ES_port : DEFAULT_LOCAL_PORT,
                          &hints, &res);
    if (errcode != 0) { free(fdata); return NULL; }
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { free(fdata); freeaddrinfo(res); return NULL; }
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) { free(fdata); freeaddrinfo(res); close(fd); return NULL; }

    // Send header, raw file bytes, then newline terminator
    ssize_t wn;
    wn = write(fd, header, strlen(header));
    if (wn == -1) { perror("write header"); free(fdata); freeaddrinfo(res); close(fd); return NULL; }
    if (fsize > 0) {
        wn = write(fd, fdata, (size_t)fsize);
        if (wn == -1) { perror("write file data"); free(fdata); freeaddrinfo(res); close(fd); return NULL; }
    }
    wn = write(fd, "\n", 1);
    if (wn == -1) { perror("write newline"); free(fdata); freeaddrinfo(res); close(fd); return NULL; }
    free(fdata);

    // Read single-line response
    char *buffer = (char*)malloc(MAX_RESPONSE_SIZE);
    if (!buffer) { freeaddrinfo(res); close(fd); return NULL; }
    ssize_t n = read(fd, buffer, MAX_RESPONSE_SIZE - 1);
    if (n <= 0) { free(buffer); freeaddrinfo(res); close(fd); return NULL; }
    buffer[n] = '\0';

    freeaddrinfo(res);
    close(fd);
    return buffer;
}

void handle_create_response(Context *context, char *response) {
    // Expected: RCE status [EID]
    char *tok = strtok(response, " ");
    if (!tok || strcmp(tok, CREATE_RESPONSE) != 0) {
        printf("Unexpected response from server\n");
        return;
    }
    char *status = strtok(NULL, " ");
    if (!status) { printf("Invalid response format\n"); return; }

    if (strcmp(status, STATUS_OK) == 0) {
        char *eid_str = strtok(NULL, " ");
        if (eid_str) {
            printf("Event created with EID %s\n", eid_str);
            // Store EID in context list if capacity allows
            if (context) {
                int eid = atoi(eid_str);
                if (eid >= 0) {
                    if (context->my_eids_count < (int)(sizeof(context->my_eids) / sizeof(context->my_eids[0]))) {
                        context->my_eids[context->my_eids_count++] = eid;
                    } else {
                        printf("Note: local event list full; EID not stored.\n");
                    }
                }
            }
        } else {
            printf("Event created\n");
        }
    } else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in\n");
    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password\n");
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Could not create event\n");
    } else {
        printf("%s\n", status_to_message(status));
    }
}


// COMMAND SHOW (SED)
void handle_show(char **words, Context *context) {
    if (check_command_show(words) == NOT_VALID) {
        printf("Invalid arguments for command show, usage: show <EID>\n");
        return;
    }

    int eid = atoi(words[1]);
    char message[MAX_LINE_LEN];
    snprintf(message, sizeof(message), "SED %03d\n", eid);

    int fd, errcode;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    errcode = getaddrinfo(
        context->ES_IP ? context->ES_IP : DEFAULT_LOCAL_IP,
        context->ES_port ? context->ES_port : DEFAULT_LOCAL_PORT,
        &hints, &res
    );
    if (errcode != 0) { printf("Network error\n"); return; }
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { freeaddrinfo(res); perror("socket"); return; }
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) { freeaddrinfo(res); close(fd); perror("connect"); return; }
    ssize_t n = write(fd, message, strlen(message));
    if (n == -1) { freeaddrinfo(res); close(fd); perror("write"); return; }

    // Read into dynamic buffer
    size_t total = 0, cap = 4096;
    char *buf = malloc(cap);
    if (!buf) { freeaddrinfo(res); close(fd); return; }

    // Read loop until we can parse header and have Fdata
    long fsize = -1;
    size_t data_start = 0;
    char fname[256] = {0};
    int header_ok = 0;
    for (;;) {
        if (total + 2048 > cap) {
            cap *= 2;
            char *nb = realloc(buf, cap);
            if (!nb) { free(buf); freeaddrinfo(res); close(fd); return; }
            buf = nb;
        }
        n = read(fd, buf + total, 2048);
        if (n == -1) { perror("read"); free(buf); freeaddrinfo(res); close(fd); return; }
        if (n == 0) break;
        total += (size_t)n;

        // Try to parse once we have some data
        if (!header_ok) {
            // Find tokens by spaces in the readable header part
            // We search sequentially: RSE OK ... Fname Fsize then Fdata
            size_t i = 0;
            // Ensure we have at least some minimal length
            if (total < 16) continue;

            // Verify starts with RSE
            if (total < 3) continue;
            if (!(buf[0]=='R' && buf[1]=='S' && buf[2]=='E' && buf[3]==' ')) continue;
            // Skip "RSE "
            i = 4;
            // Collect tokens until we reach Fname and Fsize
            int tokens = 0;
            char *token_ptrs[16];
            size_t token_lens[16];
            while (i < total && tokens < 16) {
                // find next space delimiter
                size_t start = i;
                while (i < total && buf[i] != ' ') i++;
                if (i >= total) break;
                token_ptrs[tokens] = buf + start;
                token_lens[tokens] = i - start;
                tokens++;
                i++; // skip space
                // Stop early if we have enough tokens
            }
            // We expect tokens: status, (EID/name/state/date/time or date+time), attendance_size, Seats_reserved, Fname, Fsize
            if (tokens < 9) continue;
            // status must be OK
            if (token_lens[0] != 2 || strncmp(token_ptrs[0], "OK", 2) != 0) {
                if (token_lens[0] == 3 && strncmp(token_ptrs[0], "NOK", 3) == 0) {
                    printf("Event not found\n");
                } else {
                    printf("Unexpected status\n");
                }
                free(buf); freeaddrinfo(res); close(fd); return;
            }
            // Use protocol positions: after "RSE ", we expect tokens:
            // 0: OK, 1: UID(or EID), 2: name, 3: date, 4: time, 5: capacity,
            // 6: reserved, 7: fname, 8: fsize, then a space and raw data
            char *fname_tok = token_ptrs[7];
            size_t fname_len = token_lens[7];
            char *fsize_tok = token_ptrs[8];
            size_t fsize_len = token_lens[8];
            // Parse fsize
            char fsize_buf[32];
            size_t copy_len = fsize_len < sizeof(fsize_buf)-1 ? fsize_len : sizeof(fsize_buf)-1;
            memcpy(fsize_buf, fsize_tok, copy_len); fsize_buf[copy_len] = '\0';
            fsize = strtol(fsize_buf, NULL, 10);
            if (fsize < 0) { printf("Invalid file size\n"); free(buf); freeaddrinfo(res); close(fd); return; }
            // Copy fname
            size_t fname_copy = fname_len < sizeof(fname)-1 ? fname_len : sizeof(fname)-1;
            memcpy(fname, fname_tok, fname_copy); fname[fname_copy] = '\0';
            // Data starts right after the space following fsize token
            // Find fsize token end position
            size_t fsize_end_pos = (size_t)(fsize_tok - buf) + fsize_len;
            if (fsize_end_pos >= total) continue; // need more data
            if (buf[fsize_end_pos] != ' ') {
                // If newline after fsize, data may start immediately
                if (buf[fsize_end_pos] == '\n' || buf[fsize_end_pos] == '\r') {
                    data_start = fsize_end_pos + 1;
                } else {
                    // Unexpected delimiter, still treat next byte as start
                    data_start = fsize_end_pos + 1;
                }
            } else {
                data_start = fsize_end_pos + 1;
            }
            header_ok = 1;
        }

        if (header_ok) {
            // If we have enough bytes for data, we can save
            if (total >= data_start + (size_t)fsize) {
                FILE *out = fopen(fname, "wb");
                if (!out) { perror("fopen"); free(buf); freeaddrinfo(res); close(fd); return; }
                size_t written = fwrite(buf + data_start, 1, (size_t)fsize, out);
                fclose(out);
                if (written != (size_t)fsize) { printf("Failed to write all data\n"); }
                printf("Stored file: %s (%ld bytes)\n", fname, fsize);
                break;
            }
        }
    }

    free(buf);
    freeaddrinfo(res);
    close(fd);
}

// COMMAND MYEVENTS (mye)
void handle_myevents(char **words, Context *context) {
    if (check_command_myevents(words) == NOT_VALID) {
        printf("Invalid arguments for command mye, usage: mye\n");
        return;
    }

    if (!context->logged_in) {
        printf("No user is currently logged in\n");
        return;
    }

    char *response = send_myevents_request(context);
    if (response == NULL) {
        printf("Failed to receive response from server\n");
        return;
    }
    response[strcspn(response, "\n")] = '\0';
    handle_myevents_response(context, response);
    free(response);
}


char* send_myevents_request(Context *context) {
    char message[MAX_LINE_LEN];
    // LME UID password\n (UDP)
    snprintf(message, sizeof(message), "LME %06d %s\n", context->uid, context->password);
    char *response = send_request_UDP(
        context->ES_IP ? context->ES_IP : DEFAULT_LOCAL_IP,
        context->ES_port ? context->ES_port : DEFAULT_LOCAL_PORT,
        message
    );
    return response;
}

void handle_myevents_response(Context *context, char *response) {
    // Expected: RME status [EID]*
    char *tok = strtok(response, " ");
    if (!tok || strcmp(tok, "RME") != 0) {
        printf("Unexpected response from server\n");
        return;
    }
    char *status = strtok(NULL, " ");
    if (!status) { printf("Invalid response format\n"); return; }

    if (strcmp(status, STATUS_OK) == 0) {
        // Print each event EID and state on the same line; also refresh local context list
        // Expected payload repeating pairs: EID STATE EID STATE ...
        int any = 0;
        context->my_eids_count = 0;
        while (1) {
            char *eid = strtok(NULL, " ");
            if (!eid) break;
            char *state = strtok(NULL, " ");
            if (!state) break;
            any = 1;
            // Store EID locally
            int eid_val = atoi(eid);
            if (eid_val >= 0 && context->my_eids_count < (int)(sizeof(context->my_eids)/sizeof(context->my_eids[0]))) {
                context->my_eids[context->my_eids_count++] = eid_val;
            }
            // Print EID and human-readable state in one line
            printf("%s %s\n", eid, event_state_to_text(state));
        }
        if (!any) {
            printf("(none)\n");
        }
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("No events\n");
        context->my_eids_count = 0;
    } else if (strcmp(status, "NLG") == 0) {
        printf("%s\n", status_to_message(status));
    } else if (strcmp(status, "WRP") == 0) {
        printf("%s\n", status_to_message(status));
    } else if (strcmp(status, "UNR") == 0) {
        printf("%s\n", status_to_message(status));
    } else if (strcmp(status, "NID") == 0) {
        printf("%s\n", status_to_message(status));
    } else {
        printf("%s\n", status_to_message(status));
    }
}

// COMMAND LIST
void handle_list(char **words, Context *context) {
    if (check_command_list(words) == NOT_VALID) {
        printf("Invalid arguments for command list, usage: list\n");
        return;
    }

    char *response = send_list_request(context);
    if (response == NULL) {
        printf("Failed to receive response from server\n");
        return;
    }
    
    handle_list_response(response);
    free(response);
}

char* send_list_request(Context *context) {
    char message[MAX_LINE_LEN];
    snprintf(message, sizeof(message), "LST\n");
    char *response = send_request_TCP(
        context->ES_IP ? context->ES_IP : DEFAULT_LOCAL_IP,
        context->ES_port ? context->ES_port : DEFAULT_LOCAL_PORT,
        message
    );
    return response;
}

void handle_list_response(char *response) {
    // Expected: RLS status [EID name state event_date]* (single line terminated by \n)
    // We'll tokenize by spaces and print in a readable table.
    // Make a local copy for strtok as response may be dynamically sized.
    char *buf = strdup(response);
    if (!buf) { printf("Memory error\n"); return; }
    // Trim trailing newlines
    buf[strcspn(buf, "\r\n")] = '\0';

    char *tok = strtok(buf, " ");
    if (!tok || strcmp(tok, LIST_RESPONSE) != 0) {
        printf("Unexpected response from server\n");
        free(buf);
        return;
    }
    char *status = strtok(NULL, " ");
    if (!status) { printf("Invalid response format\n"); free(buf); return; }

    if (strcmp(status, STATUS_OK) == 0) {
        // Each event is expected to be: EID name state DD-MM-YYYY HH:MM
        // Print one event per line with fields separated by spaces
        int printed_any = 0;
        while (1) {
            char *eid = strtok(NULL, " ");
            if (!eid) break;
            char *name = strtok(NULL, " ");
            if (!name) break;
            char *state = strtok(NULL, " ");
            if (!state) break;
            char *date = strtok(NULL, " ");
            if (!date) break;
            char *time = strtok(NULL, " ");
            if (!time) break;

            printed_any = 1;
            printf("%s %s %s %s %s\n", eid, name, event_state_to_text(state), date, time);
        }
        if (!printed_any) {
            printf("(none)\n");
        }
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("No events available\n");
    } else {
        printf("%s\n", status_to_message(status));
    }
    free(buf);
}

// COMMAND CLOSE
void handle_close(char **words, Context *context) {
    if (check_command_close(words) == NOT_VALID) {
        printf("Invalid arguments for command close, usage: close <EID>\n");
        return;
    }

    if (!context->logged_in) {
        printf("No user is currently logged in\n");
        return;
    }

    int eid = atoi(words[1]);
    char *response = send_close_request(context, eid);
    if (response == NULL) {
        printf("Failed to receive response from server\n");
        return;
    }
    response[strcspn(response, "\n")] = '\0';
    handle_close_response(context, response);
    free(response);
}

char* send_close_request(Context *context, int eid) {
    char message[MAX_LINE_LEN];
    // CLS UID password EID\n
    snprintf(message, sizeof(message), "CLS %06d %s %03d\n", context->uid, context->password, eid);

    // Use TCP per protocol
    char *response = send_request_TCP(
        context->ES_IP ? context->ES_IP : DEFAULT_LOCAL_IP,
        context->ES_port ? context->ES_port : DEFAULT_LOCAL_PORT,
        message
    );
    return response;
}

void handle_close_response(Context *context, char *response) {
    // Expected: RCL status
    char *word = strtok(response, " ");
    if (word == NULL || strcmp(word, "RCL") != 0) {
        printf("Unexpected response from server\n");
        return;
    }

    char *status = strtok(NULL, " ");
    if (status == NULL) {
        printf("Invalid response format\n");
        return;
    }

    if (strcmp(status, STATUS_OK) == 0) {
        printf("Event closed successfully\n");
        // Optional: remove EID from context->my_eids if present
        char *eid_tok = strtok(NULL, " ");
        if (eid_tok) {
            int eid = atoi(eid_tok);
            for (int i = 0; i < context->my_eids_count; ++i) {
                if (context->my_eids[i] == eid) {
                    // shift left
                    for (int j = i + 1; j < context->my_eids_count; ++j) {
                        context->my_eids[j - 1] = context->my_eids[j];
                    }
                    context->my_eids_count--;
                    break;
                }
            }
        }
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Could not close event\n");
    } else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in\n");
    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password\n");
    } else if (strcmp(status, "UNR") == 0) {
        printf("User not registered\n");
    } else if (strcmp(status, "NID") == 0) {
        printf("Unknown user\n");
    } else {
        printf("%s\n", status_to_message(status));
    }
}

// COMMAND LOGOUT
void handle_logout(char **words, Context *context) {

    if (!check_command_logout(words)) {
        printf("Invalid arguments for command logout\n");
        return;
    }

    if (!context->logged_in) {
        printf("No user is currently logged in\n");
        return;
    }

    char *response = send_logout_request(context);
    handle_logout_response(context, response);
    free(response);
}

char* send_logout_request(Context *context) {
    char message[MAX_LINE_LEN];

    sprintf(message, "LOU %06d %s\n", context->uid, context->password);

    char *response = send_request_UDP(context->ES_IP, context->ES_port, message);

    if (response != NULL)
        response[strcspn(response, "\n")] = '\0';

    return response;
}

void handle_logout_response(Context *context, char *response) {

    char *word = strtok(response, " ");

    // Verifica comando de resposta
    if (word == NULL || strcmp(word, "RLO") != 0) {
        printf("Unexpected response from server\n");
        return;
    }

    char *status = strtok(NULL, " ");

    if (status == NULL) {
        printf("Invalid response format\n");
        return;
    }

    if (strcmp(status, "OK") == 0) {
        printf("Logout successful\n");
        context->logged_in = 0;
        context->uid = -1;
    }
    else if (strcmp(status, "NOK") == 0) {
        printf("User is not logged in\n");
    }
    else if (strcmp(status, "UNR") == 0) {
        printf("User not registered\n");
    }
    else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password\n");
    }
    else {
        printf("%s\n", status_to_message(status));
    }
}


// COMMAND UNREGISTER

void handle_unregister(char **words, Context *context) {

    // Validação sintática: UNR UID password
    if (check_command_unregister(words) == NOT_VALID) {
        printf("Invalid arguments for command unregister\n");
        return;
    }

    // O utilizador tem de estar logado
    if (!context->logged_in) {
        printf("No user is currently logged in\n");
        return;
    }

    // O UID tem de coincidir com o utilizador logado
    char *response = send_unregister_request(context);

    if (response == NULL) {
        printf("Failed to receive response from server\n");
        return;
    }

    handle_unregister_response(context, response);
    free(response);
}

char* send_unregister_request(Context *context) {
    char message[MAX_LINE_LEN];

    sprintf(message, "UNR %06d %s\n", context->uid, context->password);

    char *response = send_request_UDP(
        context->ES_IP,
        context->ES_port,
        message
    );

    if (response != NULL)
        response[strcspn(response, "\n")] = '\0';

    return response;
}

void handle_unregister_response(Context *context, char *response) {

    char *word = strtok(response, " ");

    if (word == NULL || strcmp(word, "RUR") != 0) {
        printf("Unexpected response from server\n");
        return;
    }

    char *status = strtok(NULL, " ");

    if (strcmp(status, "OK") == 0) {
        printf("User successfully unregistered\n");
        reset_context(context);   // deixa de estar logado e sem UID
    }
    else if (strcmp(status, "NOK") == 0) {
        printf("User is not logged in\n");
    }
    else if (strcmp(status, "UNR") == 0) {
        printf("User not registered\n");
    }
    else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password\n");
    }
    else {
        printf("%s\n", status_to_message(status));
    }
}

// COMMAND RESERVE
void handle_reserve(char **words, Context *context) {
    if (check_command_reserve(words) == NOT_VALID) {
        printf("Invalid arguments for command reserve, usage: reserve <EID> <people>\n");
        return;
    }
    if (!context->logged_in) {
        printf("No user is currently logged in\n");
        return;
    }

    int eid = atoi(words[1]);
    int people = atoi(words[2]);
    if (people <= 0) {
        printf("Invalid number of people\n");
        return;
    }

    char *response = send_reserve_request(context, eid, people);
    if (!response) {
        printf("Failed to receive response from server\n");
        return;
    }
    response[strcspn(response, "\n")] = '\0';
    handle_reserve_response(response);
    free(response);
}

char* send_reserve_request(Context *context, int eid, int people) {
    char message[MAX_LINE_LEN];
    // RID UID password EID people\n
    snprintf(message, sizeof(message), "RID %06d %s %03d %d\n", context->uid, context->password, eid, people);
    char *response = send_request_TCP(
        context->ES_IP ? context->ES_IP : DEFAULT_LOCAL_IP,
        context->ES_port ? context->ES_port : DEFAULT_LOCAL_PORT,
        message
    );
    return response;
}

void handle_reserve_response(char *response) {
    // Expected: RRI status [optional info]
    char *word = strtok(response, " ");
    if (word == NULL || strcmp(word, "RRI") != 0) {
        printf("Unexpected response from server\n");
        return;
    }
    char *status = strtok(NULL, " ");
    if (!status) { printf("Invalid response format\n"); return; }

    if (strcmp(status, STATUS_OK) == 0 || strcmp(status, "ACC") == 0) {
        // Optionally consume additional fields like reservation code
        char *info = strtok(NULL, " ");
        if (info) printf("Reservation successful: %s\n", info);
        else printf("Reservation successful\n");
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Reservation failed\n");
    } else if (strcmp(status, "REJ") == 0) {
        printf("Event is full\n");
    } else if (strcmp(status, "NST") == 0) {
        printf("Event not started or not available\n");
    } else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in\n");
    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password\n");
    } else if (strcmp(status, "NID") == 0) {
        printf("Unknown user\n");
    } else if (strcmp(status, "CLS") == 0) {
        printf("Event closed\n");
    } else if (strcmp(status, "SLD") == 0) {
        printf("Sold out\n");
    } else if (strcmp(status, "PST") == 0) {
        printf("Date has already passed\n");
    } else {
        printf("%s\n", status_to_message(status));
    }
}


// COMMAND MYRESERVATIONS (LMR/RMR)
void handle_myreservations(char **words, Context *context) {
    if (check_command_myreservations(words) == NOT_VALID) {
        printf("Invalid arguments for command myreservations, usage: myreservations\n");
        return;
    }
    if (!context->logged_in) {
        printf("No user is currently logged in\n");
        return;
    }

    char *response = send_myreservations_request(context);
    if (response == NULL) {
        printf("Failed to receive response from server\n");
        return;
    }
    response[strcspn(response, "\n")] = '\0';
    handle_myreservations_response(response);
    free(response);
}

char* send_myreservations_request(Context *context) {
    char message[MAX_LINE_LEN];
    // LMR UID password\n (UDP)
    snprintf(message, sizeof(message), "LMR %06d %s\n", context->uid, context->password);
    char *response = send_request_UDP(
        context->ES_IP ? context->ES_IP : DEFAULT_LOCAL_IP,
        context->ES_port ? context->ES_port : DEFAULT_LOCAL_PORT,
        message
    );
    return response;
}

void handle_myreservations_response(char *response) {
    // Expected: RMR status [EID date time people]*
    char *tok = strtok(response, " ");
    if (!tok || strcmp(tok, "RMR") != 0) {
        printf("Unexpected response from server\n");
        return;
    }
    char *status = strtok(NULL, " ");
    if (!status) { printf("Invalid response format\n"); return; }

    if (strcmp(status, STATUS_OK) == 0) {
        int any = 0;
        while (1) {
            char *eid = strtok(NULL, " ");
            if (!eid) break;
            char *date = strtok(NULL, " ");
            if (!date) break;
            char *time = strtok(NULL, " ");
            if (!time) break;
            char *people = strtok(NULL, " ");
            if (!people) break;
            any = 1;
            // Print EID, date, time and number of people on the same line
            printf("%s %s %s %s\n", eid, date, time, people);
        }
        if (!any) printf("(none)\n");
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("No reservations\n");
    } else if (strcmp(status, "NLG") == 0 || strcmp(status, "WRP") == 0 || strcmp(status, "UNR") == 0 || strcmp(status, "NID") == 0) {
        printf("%s\n", status_to_message(status));
    } else {
        printf("%s\n", status_to_message(status));
    }
}

// COMMAND EXIT
void handle_exit(char **words, Context *context) {
    // Expect only the command keyword
    if (validate_field_count(words, 1) == NOT_VALID) {
        printf("Invalid arguments for command exit\n");
        return;
    }
    if (context->logged_in) {
        printf("Please logout before exiting\n");
        return;
    }
    // If not logged in, terminate application
    exit(0);
}

/************************************ UDP / TCP ***********************************/

char* send_request_UDP(char* GSIP, char* GSport, char message[MAX_LINE_LEN]) {
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char *buffer = (char*) malloc(MAX_LINE_LEN * sizeof(char));
    socklen_t addrlen;

    fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (fd == -1) exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(GSIP, GSport, &hints, &res);
    if (errcode != 0) exit(1);

    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
    if (n == -1) exit(1);

    //write(1, "echo: ", 6);
    //write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);

    return buffer;
}

char* send_request_TCP(char* GSIP, char* GSport, char message[MAX_LINE_LEN]) {
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(GSIP, GSport, &hints, &res);
    if (errcode != 0) exit(1);

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);

    n = write(fd, message, strlen(message));
    if (n == -1) exit(1);

    char *buffer = (char*) malloc(MAX_LINE_LEN * sizeof(char));

    // Dynamically allocate a growing buffer to hold the full message
    size_t total_size = 0;
    char *full_message = NULL;

    while (1) {
        // Read from the socket
        n = read(fd, buffer, BUFFER_SIZE - 1); // Leave space for null terminator
        if (n == -1) {
            perror("Error reading from socket");
            free(full_message);
            exit(1);
        }

        // Check for end of data
        if (n == 0) {
            break; // Socket closed by the other side
        }

        // Null-terminate the read buffer for safety
        buffer[n] = '\0';

        // Reallocate the full message buffer to fit new data
        full_message = realloc(full_message, total_size + n + 1);
        if (full_message == NULL) {
            perror("Memory allocation failed");
            exit(1);
        }

        // Append the new data to the full message buffer
        memcpy(full_message + total_size, buffer, n + 1);
        total_size += n;
    }    

    freeaddrinfo(res);
    close(fd);

    return full_message;
}