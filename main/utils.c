#include "utils.h"

bool utils_retry(bool_ptr_func to_retry)
{
    uint8_t retries = 5;
    while (retries > 0)
    {
        if (to_retry())
        {
            return true;
        }

        retries--;
    }

    return false;
}