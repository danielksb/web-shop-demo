#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <libpq-fe.h>

#include "types.h"
#include "database.h"

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
