
#include "server.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_SERVER_PORT 8080


void handle_client_echo(int client_socket) {
    char input[512];

    /*
     * Setup a timeout on recv() on the client socket
     * */
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    while (1) {
        ssize_t n = recv(client_socket, input, sizeof(input), 0);

        if (n < 0) {
            char errmsg[512];
            strerror_r(errno, errmsg, sizeof(errmsg));
            fprintf(stderr, "ERROR recv: %s\r\n", errmsg);
            close(client_socket);
            return;
        } else if (n == 0) {
            printf("DEBUG: client closed connection\r\n");
            close(client_socket);
            return;
        }
        printf("DEBUG: received %ld bytes from client\r\n", n);
        send(client_socket, input, n, 0);
    }
}

int main(int argc, char *argv[])
{
    int server_port;

    if (argc > 1)
      server_port = atoi(argv[1]);
    else
      server_port = DEFAULT_SERVER_PORT;

    Server server;
    if (server_init(&server, handle_client_echo) != 0) {
        printf("ERROR: cannot initalize mutex\r\n");
        return 1;
    }
    server_loop(&server, server_port);
}