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
    uint32_t per_line = 8;
    uint32_t line_count = 512 / per_line;

    for (uint32_t i = 0; i < line_count; i++)
    {
        uint32_t offset = 0;
        char buffer[MAX_LOG_BUFFER_SIZE];

        for (uint32_t j = 0; j < per_line; j++)
        {
            offset += snprintf(buffer + offset, MAX_LOG_BUFFER_SIZE - offset, "%02X ", block[j + (i * per_line)]);
        }

        ESP_LOGI(TAG, "%s", buffer);
    }
}

// Extracts an uint32 from an uint8 array, treating the array in little endian
uint32_t extract_uint32_le(uint8_t *arr, uint32_t index)
{
    return (arr[index + 3] << 24) | (arr[index + 2] << 16) | (arr[index + 1] << 8) | arr[index];
}

uint16_t extract_uint16_le(uint8_t *arr, uint32_t index)
{
    return (arr[index + 1] << 8) | arr[index];
}

uint8_t extract_uint8_le(uint8_t *arr, uint32_t index)
{
    return arr[index];
}

void log_uint8_array(const char *tag, const uint8_t *array, uint32_t size)
{
    char buffer[MAX_LOG_BUFFER_SIZE];
    uint32_t offset = 0;

    // Add the prefix to the buffer
    offset += snprintf(buffer + offset, MAX_LOG_BUFFER_SIZE - offset, "[");

    // Loop through the array and add each element to the buffer
    for (uint32_t i = 0; i < size && offset < MAX_LOG_BUFFER_SIZE; i++)
    {
        offset += snprintf(buffer + offset, MAX_LOG_BUFFER_SIZE - offset, "0x%02X%s",
                           array[i], (i < size - 1) ? ", " : "");
    }

    // Close the array in the log message
    snprintf(buffer + offset, MAX_LOG_BUFFER_SIZE - offset, "]");

    // Log the entire buffer at once
    ESP_LOGI(tag, "%s", buffer);
}