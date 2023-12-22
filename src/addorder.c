#include <stdio.h>
#include <stdlib.h>
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
        char param_item_id[11];
        const int param_item_id_length = snprintf(param_item_id, 11, "%d", item.id);
        const char *select_params[] = {param_item_id};
        const int select_params_lengths[] = {param_item_id_length};
        res = PQexecParams(conn, "SELECT price FROM items WHERE item_id = $1", 1, NULL, select_params, select_params_lengths, NULL, 0);
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
            die("SELECT failed");
        }
        if (PQntuples(res) == 0) {
            char msg[512];
            snprintf(msg, 512, "Item not found %d", item.id);
            die(msg);
        }

        char unit_price[11];
        const int unit_price_length = snprintf(unit_price, 11, "%.10s", PQgetvalue(res, 0, 0));
        PQclear(res);
        

        // create order item
        char param_order_id[11];
        const int param_order_id_length = snprintf(param_order_id, 11, "%d", item.id);

        char param_quantity[11];
        const int param_quantity_length = snprintf(param_quantity, 11, "%d", item.count);

        const char *insert_params[] = {
            param_order_id,
            param_item_id,
            param_quantity,
            unit_price
        };
        const int insert_params_lengths[] = {
            param_order_id_length,
            param_item_id_length,
            param_quantity_length,
            unit_price_length
        };
        res = PQexecParams(conn, "INSERT INTO order_items (order_id, item_id, quantity, unit_price) VALUES ($1, $2, $3, $4)", 4, NULL, insert_params, insert_params_lengths, NULL, 0);
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
