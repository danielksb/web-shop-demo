#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

#include "api.h"
#include "types.h"

// TODO: configure server connection
#define SERVER "localhost"
#define PORT 8080

/// @brief Executes a request
/// @param req_header address of the request header to be sent
/// @param res_header adress of the response header which will be set on success
/// @return EXIT_SUCCESS on success
int exec_request(RequestHeader *req_header, ResponseHeader *res_header) {
    // create socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
    {
        perror("ERROR: creating socket");
        return EXIT_FAILURE;
    }
    // Connect to server
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    if (connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("ERROR: connecting to server");
        close(client_fd);
        return EXIT_FAILURE;
    }

    // Send message to server
    if (send(client_fd, req_header, sizeof(req_header), 0) < 0)
    {
        perror("ERROR: sending message");
        close(client_fd);
        return EXIT_FAILURE;
    }

    // Receive message from server
    int n;
    if ((n = recv(client_fd, res_header, sizeof(res_header), 0)) > 0)
    {
        if (n != sizeof(res_header))
        {
            fprintf(stderr, "ERROR: received incorrect data, expected %ld bytes, but got %d\r\n", sizeof(res_header), n);
            close(client_fd);
            return EXIT_FAILURE;
        }
        if (res_header->magicnum != API_MAGIC_NUM)
        {
            fprintf(stderr, "ERROR: received invalid magic number, expected %d, but got %d\r\n", API_MAGIC_NUM, res_header->magicnum);
            close(client_fd);
            return EXIT_FAILURE;
        }
        printf("DEBUG: response id: %d\r\n", res_header->response_id);
        printf("DEBUG: response payload: %d\r\n", res_header->payload_size);
        if (res_header->response_id == RESPONSE_ERROR) {
            fprintf(stderr, "ERROR: received error response from server\r\n");
            if (res_header->payload_size > 0) {
                char server_err_msg[res_header->payload_size / sizeof(char)];
                if ((n = recv(client_fd, server_err_msg, res_header->payload_size, 0)) > 0)
                {
                    fprintf(stderr, "ERROR: %s\r\n", server_err_msg);
                }
            }
            close(client_fd);
            return EXIT_FAILURE;
        } else if (res_header->response_id == RESPONSE_DISPLAY_ORDERS) {
            printf("DEBUG: received display orders response\r\n");
            if (res_header->payload_size > 0) {
                printf("DEBUG: payload size %d\r\n", res_header->payload_size);
                size_t order_items_count = res_header->payload_size / sizeof(FullOrderItem); 
                FullOrderItem order_items[order_items_count];
                if ((n = recv(client_fd, order_items, res_header->payload_size, 0)) < 0)
                {
                    perror("ERROR: cannot receive display orders payload");
                }
                printf("%-20s", "order_id");
                printf("%-20s", "order_date");
                printf("%-20s", "order_status");
                printf("%-20s", "order_item_id");
                printf("%-20s", "item_name");
                printf("%-20s", "quantity");
                printf("\n");
                for (size_t i = 0; i < order_items_count; i++) {
                    printf("%-20d", order_items[i].order.id);
                    printf("%-20s", order_items[i].order.date);
                    printf("%-20s", order_items[i].order.status);
                    printf("%-20d", order_items[i].order_item.id);
                    printf("%-20s", order_items[i].order_item.name);
                    printf("%-20d", order_items[i].order_item.price);
                    printf("\n");
                }
            }
            close(client_fd);
            return EXIT_FAILURE;
        }
    }

    close(client_fd);
    return EXIT_SUCCESS;
}

int send_display_order_request()
{
    RequestHeader req_header = {
        .magicnum = API_MAGIC_NUM,
        .version = 1,
        .request_id = REQUEST_DISPLAY_ORDERS,
        .payload_size = 0
    };
    ResponseHeader res_header = {0};
    if (exec_request(&req_header, &res_header) != EXIT_SUCCESS) {
        fprintf(stderr, "ERROR: display order request failed\r\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int send_invalid_request() {
    RequestHeader req_header = {
        .magicnum = 0, // invalid magic number
        .version = 1,
        .request_id = REQUEST_DISPLAY_ORDERS,
        .payload_size = 0
    };
    ResponseHeader res_header = {0};
    if (exec_request(&req_header, &res_header) != EXIT_SUCCESS) {
        fprintf(stderr, "ERROR: request failed\r\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("Usage: [order, error, help]\r\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "order") == 0)
    {
        if (argc <= 2)
        {
            printf("Usage: order [list]\r\n");
            return EXIT_FAILURE;
        }
        if (argc > 2)
        {
            if (strcmp(argv[2], "list") == 0)
            {
                return send_display_order_request();
            }
            else
            {
                fprintf(stderr, "ERROR: unknown order command \"%s\"\r\n", argv[2]);
                return EXIT_FAILURE;
            }
        }
    }
    else if (strcmp(argv[1], "error") == 0) 
    {
        return send_invalid_request();
    }
    else if (strcmp(argv[1], "help") == 0) {
        printf("== Help ==\r\n");
        printf("%s order - CRUD operations for orders\r\n", argv[0]);
        printf("%s error - execute an invalid request\r\n", argv[0]);
        printf("%s help  - usage information\r\n", argv[0]);
        return EXIT_SUCCESS;
    }
    else
    {
        fprintf(stderr, "ERROR: unknown command \"%s\"\r\n", argv[1]);
        return EXIT_FAILURE;
    }
}