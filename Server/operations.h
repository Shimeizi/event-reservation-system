#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <dirent.h>

#include <time.h>
#include <stdio.h>

#define BASE_DIR "USERS"
#define EVENTS_DIR "EVENTS"
#define BUFFER_SIZE 512
#define MAX_WORDS 10
#define MAX_RESERVATIONS 50
#define MAX_PEOPLE 999

#define STATUS_OPEN "OPEN"
#define STATUS_SOLD_OUT "SOLD_OUT"
#define STATUS_CLOSED "CLOSED"
#define STATUS_PAST "PAST"

#define VALID 1
#define NOT_VALID 0

#define DEFAULT_PORT "58035"
#define DEFAULT_IP "tejo.tecnico.ulisboa.pt"

// A directoria de trabalho do servidor ES
void create_directory(const char *path);
void create_es_structure(void);
void create_user_structure(int uid);
void create_user_pass_file(int uid, const char *password);
void create_user_login_file(int uid);
void create_event_structure(int eid);
void create_event_files(int eid);
void create_reservation(int uid, int eid, const char *date);

// CHECK COMMANDS
int check_command_UID_Password(char *input[], int count);

// AUXILIARY FUNCTIONS
void update_user_login(int uid, int status);
int read_user_pass(int uid, char *buffer, size_t size);
int is_user_logged(int uid);
void trim_spaces(char *str);
int is_all_digits(const char *str);
int user_exists(int uid);
int check_user_password(int uid, const char *password);
int get_next_eid(void);
int event_is_past(int eid);
int event_is_sold_out(int eid);
int event_is_closed(int eid);
int get_event_remaining_seats(int eid);


// SET UP TCP/UDP
int setup_udp_socket(struct addrinfo *hints, struct addrinfo **res);
int setup_tcp_socket(struct addrinfo *hints, struct addrinfo **res);


// HANDLE UDP
void handle_udp_requests(int udp_fd);
void handle_login(int udp_fd, struct sockaddr_in client_addr, socklen_t addrlen, int uid, char *password);
void handle_logout(int udp_fd, struct sockaddr_in client_addr, socklen_t addrlen, int uid, char *password);
void handle_unregister(int udp_fd, struct sockaddr_in client_addr, socklen_t addrlen, int uid, char *password);
void handle_myevents(int udp_fd, struct sockaddr_in addr, socklen_t addrlen, int uid, const char *password);
void handle_myreservations(int udp_fd, struct sockaddr_in addr, socklen_t addrlen, int uid, const char *password);

// HANDLE TCP
void handle_tcp_requests(int tcp_fd);
void handle_create_event(char *words[], int count, int conn_fd, const char *rawbuf, int rawlen);
void handle_close_event(char *words[], int count, int conn_fd);
void handle_list_events(int conn_fd);
void handle_show_event(char *words[], int count, int conn_fd);
void handle_reserve_event(char *words[], int count, int conn_fd);
void handle_change_password(char *words[], int count, int conn_fd);

#endif
