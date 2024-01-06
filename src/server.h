#ifndef __SERVER_H_
#define __SERVER_H_

#include <pthread.h>

#define THREADS_COUNT   100

typedef struct {
    int       server_socket;              // socket for listening for new clients
    pthread_t threads[THREADS_COUNT];     // thread pool
    pthread_mutex_t mlock;                // lock for mutual access to listen()
    void (*client_cb)(int client_socket); // callback for talking to clients
} Server;

typedef struct {
    char msg[512];      // null terminated error message
} ServerError;

/// @brief Initializes the server
/// @param server  pointer to server struct
/// @param client_cb callback function for talking to clients, must not be null
/// @return 0 on success
int server_init(Server *server, void (*client_cb)(int client_socket));

/// @brief Starts server main loop. The function only returns in case of an error which the server
///        cannot recover from. If no failure occurs the server loop is executed unless a signal
///        is sent to interrupt the whole process.
/// @param server initalized server struct
/// @param server_port listing port
/// @return Error description
ServerError server_loop(Server *server, int server_port);

#endif