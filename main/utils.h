#ifndef UTILS_H
#define UTILS_H

#include "stdbool.h"
#include "stdint.h"
#include "esp_log.h"

// Function pointer that returns a bool
typedef bool (*bool_ptr_func)();

// Call supplied function 5 times until throwing in the towel
bool utils_retry(bool_ptr_func to_retry);

// Call supplied function for a specific amount of times un till call it quitz
bool utils_retry_times(bool_ptr_func to_retry, uint8_t times);

// Print out a SD block in a niceish fashion
void debug_512_block(uint8_t *block);

#endif