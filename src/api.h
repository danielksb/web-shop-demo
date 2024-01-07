#ifndef __API_H_
#define __API_H_

#include <stdint.h>

#define API_MAGIC_NUM 64
#define MAX_PAYLOAD_SIZE 1024*1024*20

typedef struct
{
    uint8_t magicnum;
    uint8_t version;
    uint16_t request_id;
    uint32_t payload_size;
} RequestHeader;

typedef enum
{
    REQUEST_DISPLAY_ORDERS
} RequestId;

typedef struct
{
    uint8_t magicnum;
    uint8_t version;
    uint16_t response_id;
    uint32_t payload_size;
} ResponseHeader;

typedef enum
{
    RESPONSE_ERROR,
    RESPONSE_DISPLAY_ORDERS,
} ResponseId;

#endif