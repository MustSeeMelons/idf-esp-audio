#ifndef UTILS_H
#define UTILS_H

#include "stdbool.h"
#include "stdint.h"

// Function pointer that returns a bool
typedef bool (*bool_ptr_func)();

bool utils_retry(bool_ptr_func to_retry);

#endif