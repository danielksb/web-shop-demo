#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <libpq-fe.h>

#define MAX_ITEM_IDS 100

// Function to print error message and exit
void die(const char *message) {
    fprintf(stderr, "ERROR: %s\n", message);
    exit(1);
}

typedef struct {
    int id;
    int count;
} ItemCount;

/// @brief Item belonging to an order
typedef struct {
    int32_t     id;         // item id
    int32_t     count;      // amount of items ordered
    int32_t     price;      // price of a single item
    size_t      name_count; // length of name
    char        name[255];  // item name
} OrderItem;

/// @brief Result of database operation
typedef enum {
    SUCCESS = 0,            // action was successful
    ERROR_DATABASE = 1,     // a database error occurred
    ERROR_NO_RESULT = 2,    // no result available although one was expected
} DatabaseResult;


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
DatabaseResult db_get_price_from_item(PGconn *conn, int32_t item_id, int32_t *price) {
    char param_item_id[11] = {0};
    snprintf(param_item_id, 11, "%d", item_id);
    const char *select_params[] = {param_item_id};
    PGresult *res = PQexecParams(conn, "SELECT price FROM items WHERE item_id = $1",
            1, NULL, select_params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        PQclear(res);
        return ERROR_DATABASE;
    }
    if (PQntuples(res) == 0) {
        fprintf(stderr, "ERROR: item not found %d\r\n", item_id);
        PQclear(res);
        return ERROR_NO_RESULT;
    }

    *price = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    return SUCCESS;
}


/// @brief Inserts a new order and returns the order id
/// @param conn Connection to the database
/// @param order_id address to save the order ID of the new order
/// @return Error object
DatabaseResult db_insert_order(PGconn *conn, int32_t *order_id) {
    PGresult *res = PQexec(conn, "INSERT INTO orders (state_id) VALUES (1) RETURNING order_id");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        PQclear(res);
        return ERROR_DATABASE;
    }
    *order_id = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    return SUCCESS;
}

/// @brief Adds an item to an existing order
/// @param conn Connection to the database
/// @param order_id ID of the order
/// @param item_id  ID of the item
/// @param item_quantity amount of items to add
/// @param price price of the item
/// @return Error object
DatabaseResult db_add_item_to_order(PGconn *conn, int32_t order_id, int32_t item_id, int32_t item_quantity, int32_t price) {
    char param_order_id[11];
    snprintf(param_order_id, 11, "%d", order_id);

    char param_item_id[11];
    snprintf(param_item_id, 11, "%d", item_id);

    char param_quantity[11];
    snprintf(param_quantity, 11, "%d", item_quantity);

    char param_unit_price[11];
    snprintf(param_unit_price, 11, "%d", price);

    const char *insert_params[] = {
        param_order_id,
        param_item_id,
        param_quantity,
        param_unit_price
    };
    // TODO: if the item was already added to the order we need to update the amount
    PGresult *res = PQexecParams(conn, "INSERT INTO order_items (order_id, item_id, quantity, unit_price) VALUES ($1, $2, $3, $4)", 4, NULL, insert_params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        PQclear(res);
        return ERROR_DATABASE;
    }
    PQclear(res);
    return SUCCESS;
}

DatabaseResult db_begin_transaction(PGconn *conn) {
    PGresult *res = PQexec(conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        PQclear(res);
        return ERROR_DATABASE;
    }
    PQclear(res);
    return SUCCESS;
}

DatabaseResult db_commit_transaction(PGconn *conn) {
    PGresult *res = PQexec(conn, "COMMIT");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        return ERROR_DATABASE;
    }
    PQclear(res);
    return SUCCESS;
}

/// @brief Get all items of an order.
/// @param conn Connection to the database
/// @param order_id ID of the order
/// @param order_items address of an array to store order items
/// @param order_items_length address to save the amount of resulting order items
/// @param max_order_items maximum amount of order items to copy into the destination array
/// @return Error object
DatabaseResult db_get_order_item_by_order_id(PGconn *conn, int32_t order_id, OrderItem *order_items, int *order_items_length, int max_order_items) {
    char param_order_id[11];
    snprintf(param_order_id, 11, "%d", order_id);

    const char *select_query_params[] = { param_order_id };

    PGresult *res = PQexecParams(conn, "SELECT oi.item_id, i.name, oi.quantity, i.price FROM order_items oi JOIN items i ON oi.item_id = i.item_id WHERE oi.order_id = $1", 1, NULL, select_query_params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "ERROR: %s\r\n", PQresultErrorMessage(res));
        PQclear(res);
        return ERROR_DATABASE;
    }
    int result_count = PQntuples(res);
    max_order_items = max_order_items <= result_count ? max_order_items : result_count;
    for (int i = 0; i < max_order_items; i++) {
        order_items[i].id = atol(PQgetvalue(res, i, 0));
        size_t name_count = snprintf(order_items[i].name, 255, "%s", PQgetvalue(res, i, 1));
        order_items[i].name_count = name_count;
        order_items[i].count = atol(PQgetvalue(res, i, 2));
        order_items[i].price = atol(PQgetvalue(res, i, 3));
    }
    *order_items_length = max_order_items;
    PQclear(res);
    return SUCCESS;
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
        die("cannot create connection");
    }

    if (db_begin_transaction(conn)) {
        die("cannot begin transaction");
    }

    // Create new order
    int32_t order_id;
    if (db_insert_order(conn, &order_id)) {
        die("cannot insert new order");
    }

    // Create order items
    for (int i = 0; i < item_counts_length; i++) {
        ItemCount item = item_counts[i];

        // check if order item exists
        int32_t price;
        if (db_get_price_from_item(conn, item.id, &price)) {
            die("cannot get price");
        }

        // create order item
        if (db_add_item_to_order(conn, order_id, item.id, item.count, price)) {
            char msg[512];
            snprintf(msg, 512, "cannot add item \"%d\" to order \"%d\"", item.id, order_id);
            die(msg);
        }
    }

    if (db_commit_transaction(conn)) {
        die("cannot commit transaction");
    }

    // Print order
    printf("Order ID: %d\n", order_id);
    OrderItem order_items[50];
    int order_items_count;
    if (db_get_order_item_by_order_id(conn, order_id, order_items, &order_items_count, 50)) {
        die("cannot get order items");
    }
    printf("Items:\n");
    for (int i = 0; i < order_items_count; i++) {
        printf("  %s (%d x %d)\n", order_items[i].name, order_items[i].count, order_items[i].price);
    }

    // Clean up
    PQfinish(conn);
    return 0;
}
