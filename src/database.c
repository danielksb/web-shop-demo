#include <stdlib.h>

#include "database.h"


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
