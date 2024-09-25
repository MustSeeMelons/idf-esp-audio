#include "utils.h"

static const char *TAG = "Debug";

bool utils_retry_times(bool_ptr_func to_retry, uint8_t times)
{
    while (times > 0)
    {
        if (to_retry())
        {
            return true;
        }

        times--;
    }

    return false;
}

bool utils_retry(bool_ptr_func to_retry)
{
    return utils_retry_times(to_retry, 5);
}

void debug_512_block(uint8_t *block)
{
    uint32_t index = 0;

    ESP_LOGI(TAG, "Start token: %02x", *(block + index));
    index++;

    for (uint32_t i = 0; i < 64; i++)
    {
        ESP_LOGI(TAG, "%02x %02x %02x %02x    %02x %02x %02x %02x", *(block + index), *(block + (index + 1)), *(block + (index + 2)), *(block + (index + 3)), *(block + (index + 4)), *(block + (index + 5)), *(block + (index + 6)), *(block + (index + 7)));
        index += 8;
    }

    ESP_LOGI(TAG, "CRC16: %02x %02x", *(block + index), *(block + (index + 1)));
}