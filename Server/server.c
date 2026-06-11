#include "operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "operations.h"


int udp_fd = -1;
int tcp_fd = -1;

/* Fecha sockets ao terminar */
void cleanup(int sig) {
    if (udp_fd != -1) close(udp_fd);
    if (tcp_fd != -1) close(tcp_fd);
    printf("\nServidor encerrado.\n");
    exit(0);
}

int main(int argc, char *argv[]) {

    int udp_fd, tcp_fd, max_fd;
    fd_set inputs, testfds;
    struct timeval timeout;
    struct addrinfo hints, *res;

    udp_fd = setup_udp_socket(&hints, &res);
    tcp_fd = setup_tcp_socket(&hints, &res);

    FD_ZERO(&inputs);
    FD_SET(udp_fd, &inputs);
    FD_SET(tcp_fd, &inputs);
    max_fd = (udp_fd > tcp_fd) ? udp_fd : tcp_fd;

    printf("Server is running on UDP and TCP port %s\n", DEFAULT_PORT);

    while (1) {
        testfds = inputs;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;

        int ret = select(max_fd + 1, &testfds, NULL, NULL, &timeout);
        if (ret == -1) {
            perror("select error");
            exit(1);
        } else if (ret == 0) {
            printf("Timeout: No activity detected.\n");
            continue;
        }

        if (FD_ISSET(udp_fd, &testfds)) {
            handle_udp_requests(udp_fd);
        }

        if (FD_ISSET(tcp_fd, &testfds)) {
            handle_tcp_requests(tcp_fd);
        }
    }
    close(udp_fd);
    close(tcp_fd);
    return 0;
}
