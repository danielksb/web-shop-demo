#include <stdlib.h>

#include "database.h"
#include "error.h"

static inline void db_set_error(Error *error, PGresult *res) {
    error_write(error, "%s", PQresultErrorMessage(res));
}

int db_get_price_from_item(PGconn *conn, int32_t item_id, int32_t *price, Error *error) {
    char param_item_id[11] = {0};
    snprintf(param_item_id, 11, "%d", item_id);
    const char *select_params[] = {param_item_id};
    PGresult *res = PQexecParams(conn, "SELECT price FROM items WHERE item_id = $1",
            1, NULL, select_params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_set_error(error, res);
        PQclear(res);
        return EXIT_FAILURE;
    }
    if (PQntuples(res) == 0) {
        error_write(error, "item not found %d", item_id);
        PQclear(res);
        return EXIT_FAILURE;
    }

    *price = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    return EXIT_SUCCESS;
}

int db_insert_order(PGconn *conn, int32_t *order_id, Error *error) {
    PGresult *res = PQexec(conn, "INSERT INTO orders (state_id) VALUES (1) RETURNING order_id");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_set_error(error, res);
        PQclear(res);
        return EXIT_FAILURE;
    }
    *order_id = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    return EXIT_SUCCESS;
}

int db_add_item_to_order(PGconn *conn, int32_t order_id, int32_t item_id, int32_t item_quantity, int32_t price, Error *error) {
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
        db_set_error(error, res);
        PQclear(res);
        return EXIT_FAILURE;
    }
    PQclear(res);
    return EXIT_SUCCESS;
}

int db_begin_transaction(PGconn *conn, Error *error) {
    PGresult *res = PQexec(conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_set_error(error, res);
        PQclear(res);
        return EXIT_FAILURE;
    }
    PQclear(res);
    return EXIT_SUCCESS;
}

int db_commit_transaction(PGconn *conn, Error *error) {
    PGresult *res = PQexec(conn, "COMMIT");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_set_error(error, res);
        return EXIT_FAILURE;
    }
    PQclear(res);
    return EXIT_SUCCESS;
}

int db_get_order_item_by_order_id(PGconn *conn, int32_t order_id, OrderItem *order_items, int *order_items_length, int max_order_items, Error *error) {
    char param_order_id[11];
    snprintf(param_order_id, 11, "%d", order_id);

    const char *select_query_params[] = { param_order_id };

    PGresult *res = PQexecParams(conn, "SELECT oi.item_id, i.name, oi.quantity, i.price FROM order_items oi JOIN items i ON oi.item_id = i.item_id WHERE oi.order_id = $1", 1, NULL, select_query_params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_set_error(error, res);
        PQclear(res);
        return EXIT_FAILURE;
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
    return EXIT_SUCCESS;
}

int db_get_order_items_latest(PGconn *conn, FullOrderItem *items, int *items_length, int max_items, Error *error) {
    char stmt[512];
    snprintf(stmt, 512, "SELECT"
        "  o.order_id,"
        "  o.order_date,"
        "  os.state_name AS order_status,"
        "  oi.order_item_id,"
        "  i.name AS item_name,"
        "  oi.quantity,"
        "  oi.unit_price"
        " FROM orders o"
        " JOIN order_items oi ON oi.order_id = o.order_id"
        " JOIN order_states os ON os.state_id = o.state_id"
        " JOIN items i ON i.item_id = oi.item_id"
        " ORDER BY o.order_date DESC"
        " LIMIT %d;", max_items);
    PGresult *res = PQexec(conn, stmt);

    // Check if the query was successful
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_set_error(error, res);
        PQclear(res);
        return EXIT_FAILURE;
    }

    int rows = PQntuples(res);

    for (int i = 0; i < rows; i++) {
        items[i].order.id = atol(PQgetvalue(res, i, 0));
        snprintf(items[i].order.date, 32, "%s", PQgetvalue(res, i, 1));
        snprintf(items[i].order.status, 50, "%s", PQgetvalue(res, i, 2));
        items[i].order_item.id = atol(PQgetvalue(res, i, 3));
        size_t name_count = snprintf(items[i].order_item.name, 255, "%s", PQgetvalue(res, i, 4));
        items[i].order_item.name_count = name_count;
        items[i].order_item.count = atol(PQgetvalue(res, i, 5));
        items[i].order_item.price = atol(PQgetvalue(res, i, 6));
    }
    *items_length = rows;

    PQclear(res);
    return EXIT_SUCCESS;
}