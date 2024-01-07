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
#include <errno.h>
#include <string.h>

/// @brief Callback for receiving a signal (default: SIGINT) to quit the server.
/// @param signo Signal which was received.
static void server_exit(int signo) {
    printf("server received shutdown signal %d\r\n", signo);
    exit(0);
}

/// @brief Accepts new incoming requests
///        and calls the client callback of the server for each new connection.
///        This functions is executing by each thread in the thread pool.
///        In case of a failure the thread is terminated.
/// @param arg pointer to the Server instance running the thread pool
/// @return EXIT_SUCCESS on success, EXIT_FAILURE on a failure
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
            char errmsg[512];
            strerror_r(errno, errmsg, sizeof(errmsg));
            fprintf(stderr, "ERROR recv: %s\r\n", errmsg);
            return 0;
        }
        pthread_mutex_unlock(&server->mlock);

        server->client_cb(client_socket);
    }
    return 0;
}

/// @brief Bind to the given port and starts listening for new connections.
///        The newly created socket FD is stored in the address of listeningSocket
/// @param port port number to listen on
/// @param listeningSocket address to store the socket file descriptor
/// @param error Error structure to store the error message in case of an failure
/// @return EXIT_SUCCESS on success, EXIT_FAILURE on a failure
static int setup_listening_socket(int port, int *listeningSocket, Error *error) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        strerror_r(errno, error->msg, sizeof(error->msg));
        return EXIT_FAILURE;
    }

    int enable = 1;
    if (setsockopt(sock,
                   SOL_SOCKET, SO_REUSEADDR,
                   &enable, sizeof(int)) < 0) {
        strerror_r(errno, error->msg, sizeof(error->msg));
        return EXIT_FAILURE;
    }

    struct sockaddr_in srv_addr;
    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock,
             (const struct sockaddr *)&srv_addr,
             sizeof(srv_addr)) < 0) {
        strerror_r(errno, error->msg, sizeof(error->msg));
        return EXIT_FAILURE;
    }     

    if (listen(sock, 10) < 0) {
        strerror_r(errno, error->msg, sizeof(error->msg));
        return EXIT_FAILURE;
    }

    *listeningSocket = sock;
    return EXIT_SUCCESS;
}
 
int server_init(Server *server, void (*client_cb)(int client_socket)) {
    if (!client_cb) {
        return EXIT_FAILURE;
    }
    server->server_socket = 0;
    server->client_cb = client_cb;
    return pthread_mutex_init(&server->mlock, 0);
}

Error server_loop(Server *server, int server_port) {
    Error error = {0};
    signal(SIGINT, server_exit);

    signal(SIGPIPE, SIG_IGN);

    printf("server listening on port %d\n", server_port);
    if (setup_listening_socket(server_port, &server->server_socket, &error) != EXIT_SUCCESS) {
        return error;
    }
    for (int i = 0; i < THREADS_COUNT; ++i) {
        if (pthread_create(&server->threads[i], NULL, &handle_request_loop, server) != 0) {
            strerror_r(errno, error.msg, sizeof(error.msg));
            return error;
        }
    }

    for (;;)
        pause();
}
