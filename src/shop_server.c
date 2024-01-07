#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>

#include "server.h"
#include "api.h"
#include "error.h"
#include "database.h"

#define DEFAULT_SERVER_PORT 8080

/// @brief sends a error response to the client
/// @param client_socket socket to send response
/// @param err_msg null terminated error message
/// @return 0 on success
int send_error_response(int client_socket, char *err_msg)
{
    printf("INFO: send error response \"%s\"\r\n", err_msg);
    ResponseHeader res_header = {
        .magicnum = API_MAGIC_NUM,
        .version = 1,
        .response_id = RESPONSE_ERROR,
        .payload_size = (strlen(err_msg) + 1) * sizeof(char)   // TODO: handle possible overflow
    };
    if (send(client_socket, &res_header, sizeof(res_header), 0) < 0)
    {
        char systemcall_err_msg[512];
        strerror_r(errno, systemcall_err_msg, sizeof(systemcall_err_msg));
        fprintf(stderr, "ERROR: cannot send error response %s\r\n", systemcall_err_msg);
        return EXIT_FAILURE;
    }
    if (res_header.payload_size > 0)
    {
        if (send(client_socket, err_msg, res_header.payload_size, 0) < 0)
        {
            char systemcall_err_msg[512];
            strerror_r(errno, systemcall_err_msg, sizeof(systemcall_err_msg));
            fprintf(stderr, "ERROR: cannot send error response %s\r\n", systemcall_err_msg);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

/// @brief sends a error response to the client
/// @param client_socket socket to send response
/// @return 0 on success
int send_display_order_response(int client_socket)
{
    Error error = {0};
    printf("DEBUG: display orders\r\n");
    // TODO: read connection data from configuration
    PGconn *conn = PQconnectdb("dbname=shopdb user=shopuser password=shopuser host=localhost port=5432");
    if (PQstatus(conn) != CONNECTION_OK) {
        send_error_response(client_socket, "internal server error");
        return EXIT_FAILURE;
    }
    int order_item_count = 10;
    FullOrderItem order_items[order_item_count];
    order_item_count = db_get_order_items_latest(conn, order_items, order_item_count, &error);
    if (order_item_count < 0) {
        fprintf(stderr, "ERROR: failed getting latest order items: %s\r\n", error.msg);
        send_error_response(client_socket, "internal server error");
        PQfinish(conn);
        return EXIT_FAILURE;
    }
    PQfinish(conn);
    printf("DEBUG: found %d order items\r\n", order_item_count);
    ResponseHeader res_header = {
        .magicnum = API_MAGIC_NUM,
        .version = 1,
        .response_id = RESPONSE_DISPLAY_ORDERS,
        .payload_size = sizeof(order_items[0]) * order_item_count
    };
    if (send(client_socket, &res_header, sizeof(res_header), 0) < 0)
    {
        char systemcall_err_msg[512];
        strerror_r(errno, systemcall_err_msg, sizeof(systemcall_err_msg));
        fprintf(stderr, "ERROR: cannot send 'display order' response header %s\r\n", systemcall_err_msg);
        return EXIT_FAILURE;
    }
    if (order_item_count > 0) {
        if (send(client_socket, order_items, res_header.payload_size, 0) < 0) {
            char systemcall_err_msg[512];
            strerror_r(errno, systemcall_err_msg, sizeof(systemcall_err_msg));
            fprintf(stderr, "ERROR: cannot send 'display order' response payload: %s\r\n", systemcall_err_msg);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

void handle_shop_request(int client_socket)
{
    RequestHeader req_header = {0};

    /*
     * Setup a timeout on recv() on the client socket
     * */
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

    while (1)
    {
        ssize_t n = recv(client_socket, &req_header, sizeof(req_header), 0);
        // check if receive was successful
        if (n < 0)
        {
            char errmsg[512];
            strerror_r(errno, errmsg, sizeof(errmsg));
            fprintf(stderr, "ERROR recv: %s\r\n", errmsg);
            close(client_socket);
            return;
        }
        else if (n == 0)
        {
            printf("DEBUG: client closed connection\r\n");
            close(client_socket);
            return;
        }
        // check if receiving data is correct
        if (req_header.magicnum != API_MAGIC_NUM)
        {
            send_error_response(client_socket, "Invalid magic number");
            return;
        }
        if (req_header.version != 1)
        {
            char err_msg[32];
            snprintf(err_msg, 32, "Invalid request version %d", req_header.version);
            send_error_response(client_socket, err_msg);
            return;
        }

        char err_msg[32];
        switch (req_header.request_id)
        {
        case REQUEST_DISPLAY_ORDERS:
            if (send_display_order_response(client_socket) != EXIT_SUCCESS)
            {
                return;
            }
            break;

        default:
            snprintf(err_msg, 32, "Unknown request id %d", req_header.request_id);
            send_error_response(client_socket, err_msg);
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    u_int16_t server_port;

    if (argc > 1)
        server_port = (u_int16_t)atoi(argv[1]); // TODO: handle possible overflow
    else
        server_port = DEFAULT_SERVER_PORT;

    Server server;
    if (server_init(&server, handle_shop_request) != 0)
    {
        printf("ERROR: cannot initalize mutex\r\n");
        return 1;
    }
    Error error = server_loop(&server, server_port);
    fprintf(stderr, "ERROR: cannot enter server loop: %s\r\n", error.msg);
    return 1;
}