#ifndef USER_OPERATIONS_H
#define USER_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "command.h"

#define MAX_WORDS 10
#define MAX_LINE_LEN 128
#define MAX_RESPONSE_SIZE 1024

#define DEFAULT_PORT "58035"
#define DEFAULT_IP "tejo.tecnico.ulisboa.pt"
#define DEFAULT_LOCAL_IP "127.0.0.1"
#define DEFAULT_LOCAL_PORT "58035"

#define VALID 1
#define NOT_VALID 0

#define BUFFER_SIZE 128

// Context structure for storing user-related state
typedef struct {
    int uid;  // user ID
    int logged_in;
    char *ES_IP;
    char *ES_port;
    char password[9]; // store current user's password (8 chars + null)
    // Track user's created events (store up to 100 EIDs locally)
    int my_eids[100];
    int my_eids_count;
} Context;

// Command dispatch table
typedef struct {
    char *command;
    void (*handler)(char **, Context *);  // handler now takes a Context pointer
} Command;

// Helper functions
int validate_field_count(char *input[], int expected_count);
int validate_UID(const char *UID);
int validate_EID(const char *EID);
int validate_password(const char *password);
int validate_event_name(const char *event_name);
int validate_data_hora(const char *data_hora);
int validate_num_lugares(const char *num_lugares);
char **parse_input(char *input, int *count);
void free_words(char **words, int count);

// Check functions
int check_command_login(char *input[]);
int check_command_logout(char *input[]);
int check_command_change_password(char *input[]);
int check_command_unregister(char *input[]);
int check_command_create(char *input[]);
int check_command_close(char *input[]) ;
int check_command_myevents(char *input[]);
int check_command_list(char *input[]);
int check_command_show(char *input[]);
int check_command_reserve(char *input[]);
int check_command_myreservations(char *input[]);

// Command handler prototypes
void handle_login(char **words, Context *context);
char* send_login_request(Context *context, int uid, char *password);
void handle_login_response(Context *context, int uid, const char *password, char *response);

void handle_logout(char **words, Context *context);
char* send_logout_request(Context *context);
void handle_logout_response(Context *context, char *response);

void handle_unregister(char **words, Context *context);
char* send_unregister_request(Context *context);
void handle_unregister_response(Context *context, char *response);

// change password (TCP)
void handle_changepass(char **words, Context *context);
char* send_changepass_request(char **words, Context *context);
void handle_changepass_response(Context *context, const char *new_password, char *response);

// exit (local)
void handle_exit(char **words, Context *context);

void handle_create(char **words, Context *context);
void handle_create_response(Context *context, char *response);
char* send_create_request(Context *context, const char *name, const char *date, const char *time, int num_attendees, const char *fname);

void handle_close(char **words, Context *context);
char* send_close_request(Context *context, int eid);
void handle_close_response(Context *context, char *response);

void handle_myevents(char **words, Context *context);
char* send_myevents_request(Context *context);
void handle_myevents_response(Context *context, char *response);

void handle_list(char **words, Context *context);
char* send_list_request(Context *context);
void handle_list_response(char *response);

void handle_show(char **words, Context *context);

// reserve (TCP)
void handle_reserve(char **words, Context *context);
char* send_reserve_request(Context *context, int eid, int people);
void handle_reserve_response(char *response);

// myreservations (UDP)
void handle_myreservations(char **words, Context *context);
char* send_myreservations_request(Context *context);
void handle_myreservations_response(char *response);

// UDP / TCP
char* send_request_UDP(char* GSIP, char* GSport, char message[MAX_LINE_LEN]);
char* send_request_TCP(char* GSIP, char* GSport, char message[MAX_LINE_LEN]);

#endif
