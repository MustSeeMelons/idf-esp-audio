#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "sd/sd.h"
#include "fat/fat.h"

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
    gpio_set_level(BLINK_GPIO, s_led_state);
}

void app_main(void)
{
    esp_err_t op_status = sd_init();

    if (op_status == ESP_OK)
    {
        fat_init();
    }

    configure_led();
    
    while (1)
    {
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}