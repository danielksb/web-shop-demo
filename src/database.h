#ifndef __DATABASE_H_
#define __DATABASE_H_

#include <inttypes.h>
#include <libpq-fe.h>

#include "types.h"

/// @brief Result of database operation
typedef enum {
    SUCCESS = 0,            // action was successful
    ERROR_DATABASE = 1,     // a database error occurred
    ERROR_NO_RESULT = 2,    // no result available although one was expected
} DatabaseResult;

/// @brief Returns price from an item
/// @param conn Connection to the database
/// @param item_id ID of an item
/// @param price address to save the resulting price
/// @return Error object
DatabaseResult db_get_price_from_item(PGconn *conn, int32_t item_id, int32_t *price);

/// @brief Inserts a new order and returns the order id
/// @param conn Connection to the database
/// @param order_id address to save the order ID of the new order
/// @return Error object
DatabaseResult db_insert_order(PGconn *conn, int32_t *order_id);

/// @brief Adds an item to an existing order
/// @param conn Connection to the database
/// @param order_id ID of the order
/// @param item_id  ID of the item
/// @param item_quantity amount of items to add
/// @param price price of the item
/// @return Error object
DatabaseResult db_add_item_to_order(PGconn *conn, int32_t order_id, int32_t item_id, int32_t item_quantity, int32_t price);

DatabaseResult db_begin_transaction(PGconn *conn);

DatabaseResult db_commit_transaction(PGconn *conn);

/// @brief Get all items of an order.
/// @param conn Connection to the database
/// @param order_id ID of the order
/// @param order_items address of an array to store order items
/// @param order_items_length address to save the amount of resulting order items
/// @param max_order_items maximum amount of order items to copy into the destination array
/// @return Error object
DatabaseResult db_get_order_item_by_order_id(PGconn *conn, int32_t order_id, OrderItem *order_items, int *order_items_length, int max_order_items);

#endif