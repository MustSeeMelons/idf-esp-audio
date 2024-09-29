#ifndef UTILS_H
#define UTILS_H

#include "stdbool.h"
#include "stdint.h"
#include <stdio.h>
#include "esp_log.h"

#define MAX_LOG_BUFFER_SIZE 256

// Function pointer that returns a bool
typedef bool (*bool_ptr_func)();

// Call supplied function 5 times until throwing in the towel
bool utils_retry(bool_ptr_func to_retry);

// Call supplied function for a specific amount of times un till call it quitz
bool utils_retry_times(bool_ptr_func to_retry, uint8_t times);

// Print out a SD block in a niceish fashion
void debug_512_block(uint8_t *block);

uint32_t extract_uint32_le(uint8_t *arr, uint32_t index);

uint16_t extract_uint16_le(uint8_t *arr, uint32_t index);

uint8_t extract_uint8_le(uint8_t *arr, uint32_t index);

void log_uint8_array(const char *tag, const uint8_t *array, uint32_t size);

#endif