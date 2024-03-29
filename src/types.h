#ifndef __TYPES_H_
#define __TYPES_H_

#include <inttypes.h>
#include <stddef.h>

/// @brief Item belonging to an order
typedef struct {
    int32_t     id;         // item id
    int32_t     count;      // amount of items ordered
    int32_t     price;      // price of a single item
    size_t      name_count; // length of name
    char        name[255];  // item name
} OrderItem;

/// @brief Single Order
typedef struct {
    int32_t     id;         // order id
    char        status[50]; // name of the status of the order
    char        date[32];   // date of the last change of the order
} Order;

/// @brief Order item combined with the information about it's order
typedef struct {
    Order order;
    OrderItem order_item;
} FullOrderItem;

#endif