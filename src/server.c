#include "server.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <pthread.h>
#include <unistd.h>

static void fatal_error(const char *msg)
{
    perror(msg);
    exit(1);
}

static void server_exit(int signo) {
    printf("server received shutdown signal %d\r\n", signo);
    exit(0);
}

static void *handle_request_loop(void *arg)
{
    Server *server = arg;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1)
    {
        pthread_mutex_lock(&server->mlock);
        int client_socket = accept(
                server->server_socket,
                (struct sockaddr *)&client_addr,
                &client_addr_len);
        if (client_socket == -1) {
            fatal_error("accept()");
        }
        pthread_mutex_unlock(&server->mlock);

        server->client_cb(client_socket);
    }
    return 0;
}

static int setup_listening_socket(int port) {
    int sock;
    struct sockaddr_in srv_addr;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        fatal_error("socket()");

    int enable = 1;
    if (setsockopt(sock,
                   SOL_SOCKET, SO_REUSEADDR,
                   &enable, sizeof(int)) < 0)
        fatal_error("setsockopt(SO_REUSEADDR)");

    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* We bind to a port and turn this socket into a listening
     * socket.
     * */
    if (bind(sock,
             (const struct sockaddr *)&srv_addr,
             sizeof(srv_addr)) < 0)
        fatal_error("bind()");

    if (listen(sock, 10) < 0)
        fatal_error("listen()");

    return (sock);
}

int server_init(Server *server, void (*client_cb)(int client_socket)) {
    if (!client_cb) {
        return EXIT_FAILURE;
    }
    server->server_socket = 0;
    server->client_cb = client_cb;
    server->error_cb = fatal_error;
    return pthread_mutex_init(&server->mlock, 0);
}

void server_loop(Server *server, int server_port) {
    signal(SIGINT, server_exit);

    signal(SIGPIPE, SIG_IGN);

    printf("server listening on port %d\n", server_port);
    server->server_socket = setup_listening_socket(server_port);
    for (int i = 0; i < THREADS_COUNT; ++i) {
        pthread_create(&server->threads[i], NULL, &handle_request_loop, server);
    }

    for (;;)
        pause();
}