#include "user_operations.h"

// main function
int main(int argc, char *argv[]) {
    char *ES_IP = NULL;
    char *ES_port = DEFAULT_PORT;

    int opt;
    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                ES_IP = optarg;
                break;
            case 'p':
                ES_port = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n ESIP] [-p ESport]\n", argv[0]);
                return 1;
        }
    }

    Context context = {
        .logged_in = 0,
        .uid = -1,          // DEFAULT_ID 
        .ES_IP = ES_IP,
        .ES_port = ES_port
    };

    char input[MAX_LINE_LEN];

    Command commands[] = {
        {"login", handle_login},
        {"logout", handle_logout},
        {"changePass", handle_changepass},
        {"unregister", handle_unregister},
        {"exit", handle_exit},
        {"create", handle_create},
        {"close", handle_close},
        {"myevents", handle_myevents},
        {"mye", handle_myevents},
        {"list", handle_list},
        {"show", handle_show},
        {"reserve", handle_reserve},
        {"myreservations", handle_myreservations},
        {"myr", handle_myreservations},
        {NULL, NULL}
    };

    while (1) {
        printf("> ");
        if (fgets(input, MAX_LINE_LEN, stdin) == NULL) break;

        input[strcspn(input, "\n")] = '\0';

        int word_count = 0;
        char **words = parse_input(input, &word_count);
        if (word_count == 0) {
            free_words(words, word_count);
            continue;
        }

        int executed = 0;
        for (int i = 0; commands[i].command != NULL; i++) {
            if (strcmp(words[0], commands[i].command) == 0) {
                commands[i].handler(words, &context);
                executed = 1;
                break;
            }
        }

        if (!executed) {
            printf("Unknown command: %s\n", words[0]);
        }

        free_words(words, word_count);
    }

    return 0;
}
