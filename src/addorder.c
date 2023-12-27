#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <libpq-fe.h>

#define MAX_ITEM_IDS 100

// Function to print error message and exit
void die(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

typedef struct {
    int id;
    int count;
} ItemCount;

typedef enum {
    NOERROR = 0,
    DATABASE_ERROR = 1,
    ID_NOT_FOUND = 2
} Error;


int find_item_count(int target, ItemCount lst[], int len) {
    for (int i = 0; i < len; i++) {
        if (target == lst[i].id) {
            return i;
        }
    }
    return -1;
}

void count_item_ids(ItemCount* counts, int *num_counts, int max_counts, int item_ids[], int num_items) {
    int max_iter = num_items < max_counts ? num_items : max_counts;
    int current_item_count_length = 0;
    for (int i = 0; i < max_iter; i++) {
        int id = item_ids[i];
        int index = find_item_count(id, counts, current_item_count_length);
        if (index >= 0) {
            counts[index].count++;
        } else {
            index = current_item_count_length++; 
            counts[index].id = id;
            counts[index].count = 1;
        }
    }
    *num_counts = current_item_count_length;
}

/// @brief Returns price from an item
/// @param conn Connection to the database
/// @param item_id ID of an item
/// @param price address to save the resulting price
/// @return Error object
Error db_get_price_from_item(PGconn *conn, int32_t item_id, int32_t *price) {
    char param_item_id[11] = {0};
    snprintf(param_item_id, 11, "%d", item_id);
    const char *select_params[] = {param_item_id};
    PGresult *res = PQexecParams(conn, "SELECT price FROM items WHERE item_id = $1",
            1, NULL, select_params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        PQclear(res);
        return DATABASE_ERROR;
    }
    if (PQntuples(res) == 0) {
        PQclear(res);
        return ID_NOT_FOUND;
    }

    *price = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    return NOERROR;
}

int main(int argc, char **argv) {

    // Parse item IDs from command line arguments
    int item_ids[MAX_ITEM_IDS];
    int num_items = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--item") == 0 || strcmp(argv[i], "-i") == 0) {
            if (num_items >= MAX_ITEM_IDS) {
                die("Too many items");
            }
            int item_id = atoi(argv[++i]);
            item_ids[num_items++] = item_id;
        }
    }

    ItemCount item_counts[MAX_ITEM_IDS] = {0};
    int item_counts_length = 0;
    count_item_ids(item_counts, &item_counts_length, MAX_ITEM_IDS, item_ids, num_items);

    PGconn *conn = PQconnectdb("dbname=shopdb user=shopuser password=shopuser host=localhost port=5432");
    if (PQstatus(conn) != CONNECTION_OK) {
        die("Connection failed");
    }

    // Start transaction
    PGresult *res = PQexec(conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        die("BEGIN failed");
    }
    PQclear(res);

    // Create new order
    res = PQexec(conn, "INSERT INTO orders (state_id) VALUES (1) RETURNING order_id");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        die("INSERT failed");
    }
    char order_id[255];
    const int order_id_length = snprintf(order_id, 254, "%.254s", PQgetvalue(res, 0, 0));
    PQclear(res);

    // Create order items
    for (int i = 0; i < item_counts_length; i++) {
        ItemCount item = item_counts[i];

        // check if order item exists
        int32_t price;
        Error result = db_get_price_from_item(conn, item.id, &price);
        if (result == DATABASE_ERROR) {
            die("ERROR: Failed getting price");
        } else if (result == ID_NOT_FOUND) {
            char msg[512];
            snprintf(msg, 512, "ERROR: Item not found %d", item.id);
            die(msg);
        } else if (result != NOERROR) {
            die("ERROR: Unknown error when getting price");
        }

        char unit_price[11];
        snprintf(unit_price, 11, "%d", price);
        

        // create order item
        char param_item_id[11] = {0};
        snprintf(param_item_id, 11, "%d", item.id);

        char param_quantity[11];
        snprintf(param_quantity, 11, "%d", item.count);

        const char *insert_params[] = {
            order_id,
            param_item_id,
            param_quantity,
            unit_price
        };
        res = PQexecParams(conn, "INSERT INTO order_items (order_id, item_id, quantity, unit_price) VALUES ($1, $2, $3, $4)", 4, NULL, insert_params, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
            die("INSERT failed");
        }
        PQclear(res);
    }

    // Commit transaction
    res = PQexec(conn, "COMMIT");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        die("COMMIT failed");
    }
    PQclear(res);

    // Print order
    printf("Order ID: %s\n", order_id);
    const char *select_query_params[] = { order_id };
    const int select_query_lengths[] = { order_id_length };
    res = PQexecParams(conn, "SELECT i.name, oi.quantity, i.price FROM order_items oi JOIN items i ON oi.item_id = i.item_id WHERE oi.order_id = $1", 1, NULL, select_query_params, select_query_lengths, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        die("SELECT failed");
    }
    printf("Items:\n");
    for (int i = 0; i < PQntuples(res); i++) {
        printf("  %s (%d x %d)\n", PQgetvalue(res, i, 0), atoi(PQgetvalue(res, i, 1)), atoi(PQgetvalue(res, i, 2)));
    }

    // Clean up
    PQclear(res);
    PQfinish(conn);
    return 0;
}
