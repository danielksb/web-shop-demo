#ifndef __SERVER_H_
#define __SERVER_H_

#include <pthread.h>

#define THREADS_COUNT   100

typedef struct {
    int       server_socket;              // socket for listening for new clients
    pthread_t threads[THREADS_COUNT];     // thread pool
    pthread_mutex_t mlock;                // lock for mutual access to listen()
    void (*client_cb)(int client_socket); // callback for talking to clients
    void (*error_cb)(const char *msg);    // callback for fatal errors
} Server;

/// @brief Initializes the server
/// @param server  pointer to server struct
/// @param client_cb callback function for talking to clients, must not be null
/// @return 0 on success
int server_init(Server *server, void (*client_cb)(int client_socket));

/// @brief Starts server main loop. The function never exits unless a signal is sent to interrupt the process.
/// @param server initalized server struct
/// @param server_port listing port
void server_loop(Server *server, int server_port);

#endif