#ifndef __ERROR_H_
#define __ERROR_H_

typedef struct {
    char msg[512];      // null terminated error message
} Error;

/// @brief Writes an text into the error message. Ensures the error message has the correct size
/// @param errMsg error message which holds the error text
/// @param message error text which should be set
/// @return amount of bytes actually written into the error message field
#define error_write(error, text, ...) snprintf(error->msg, sizeof(error->msg), text, __VA_ARGS__)

#endif