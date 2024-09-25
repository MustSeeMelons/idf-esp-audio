#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "sd/sd.h"

#define BLINK_GPIO 2

static const char *TAG = "example";

static uint8_t s_led_state = 0;

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

void app_main(void)
{
    esp_err_t err = sd_init();

    if (err == ESP_OK)
    {
        uint8_t block[515] = {0};
        err = sd_read_block(0, block);

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read block.");
        }
        else
        {
            debug_512_block(block);
        }
    }

    configure_led();

    ESP_LOGI(TAG, "Now entering infinite loop.");
    while (1)
    {
        // ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}