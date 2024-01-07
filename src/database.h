#ifndef __DATABASE_H_
#define __DATABASE_H_

#include <inttypes.h>
#include <libpq-fe.h>

#include "types.h"
#include "error.h"

/// @brief Returns price from an item
/// @param conn Connection to the database
/// @param item_id ID of an item
/// @param price address to save the resulting price
/// @param error address of error object to set an error message on failure
/// @return EXIT_SUCCESS on success
int db_get_price_from_item(PGconn *conn, int32_t item_id, int32_t *price, Error *error);

/// @brief Inserts a new order and returns the order id
/// @param conn Connection to the database
/// @param order_id address to save the order ID of the new order
/// @param error address of error object to set an error message on failure
/// @return EXIT_SUCCESS on success
int db_insert_order(PGconn *conn, int32_t *order_id, Error *error);

/// @brief Adds an item to an existing order
/// @param conn Connection to the database
/// @param order_id ID of the order
/// @param item_id  ID of the item
/// @param item_quantity amount of items to add
/// @param price price of the item
/// @param error address of error object to set an error message on failure
/// @return EXIT_SUCCESS on success
int db_add_item_to_order(PGconn *conn, int32_t order_id, int32_t item_id, int32_t item_quantity, int32_t price, Error *error);

int db_begin_transaction(PGconn *conn, Error *error);

int db_commit_transaction(PGconn *conn, Error *error);

/// @brief Get all items of an order.
/// @param conn Connection to the database
/// @param order_id ID of the order
/// @param order_items address of an array to store order items
/// @param order_items_length address to save the amount of resulting order items
/// @param max_order_items maximum amount of order items to copy into the destination array
/// @param error address of error object to set an error message on failure
/// @return EXIT_SUCCESS on success
int db_get_order_item_by_order_id(PGconn *conn, int32_t order_id, OrderItem *order_items, int *order_items_length, int max_order_items, Error *error);

/// @brief Get the latest order items and their orders.
/// @param conn Connection to the database
/// @param order_items address of an array to store order items
/// @param max_order_items maximum amount of order items to copy into the destination array
/// @param error address of error object to set an error message on failure
/// @return -1 on error or actual number of order items written to the order_items
int db_get_order_items_latest(PGconn *conn, FullOrderItem *order_items, int max_order_items, Error *error);

#endif