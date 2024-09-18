#include "sd.h"

#include "esp_log.h"

#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

static const char *TAG = "SD";
static spi_device_handle_t spi;

esp_err_t sd_send_command(uint8_t cmd, uint32_t arg)
{
    uint8_t command[6];
    // 0x40 is 01 that are the MSB of a command
    command[0] = 0x40 | cmd;
    command[1] = (arg >> 24) & 0xFF;
    command[2] = (arg >> 16) & 0xFF;
    command[3] = (arg >> 8) & 0xFF;
    command[4] = arg & 0xFF;
    command[5] = cmd == 0 ? 0x95 : 0x97; // CRC, valid only for CMD0 and CMD8

    spi_transaction_t t = {
        .length = 6 * 8,
        .tx_buffer = command,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));

    return ESP_OK;
}

esp_err_t sd_recieve_response(uint8_t *response)
{
    uint8_t dummy = 0xFF;

    spi_transaction_t r = {
        .length = 8, // these are bits
        .tx_buffer = &dummy,
        .rx_buffer = response,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, &r));

    return ESP_OK;
}

void sd_init_card(void)
{
    send_dummy_clocks();

    // Send CMD0 to reset the card
    uint8_t response = 0xff;
    sd_send_command(0, 0);

    uint8_t retries = 3;
    response = 0xff;
    while (response != 0x01)
    {
        sd_recieve_response(&response);
        ESP_LOGI(TAG, "Reset response: %d", response);
        retries--;

        if (retries == 0)
        {
            ESP_LOGE(TAG, "Failed to reset card!");
            return;
        }
    }

    ESP_LOGI(TAG, "Card is in idle state.");

    // Send CMD8 to check voltage range
    uint8_t cmd8_response[5] = {};
    // XXX CRC is wrong here
    sd_send_command(8, 0x1AA);

    sd_recieve_response((uint8_t *)(cmd8_response + 0));
    sd_recieve_response((uint8_t *)(cmd8_response + 1));
    sd_recieve_response((uint8_t *)(cmd8_response + 2));
    sd_recieve_response((uint8_t *)(cmd8_response + 3));
    sd_recieve_response((uint8_t *)(cmd8_response + 4));

    if (cmd8_response[0] != 0x01)
    {
        ESP_LOGE(TAG, "CMD8 failed!");
        return;
    }

    ESP_LOGI(TAG, "CMD8 successful.");
}

void send_dummy_clocks(void)
{
    uint8_t dummy = 0xFF;
    for (int i = 0; i < 10; i++)
    {
        spi_transaction_t t = {
            .length = 8, // 1 byte
            .tx_buffer = &dummy,
        };
        spi_device_transmit(spi, &t);
    }
}

void sd_init()
{
    gpio_pullup_en(SD_MOSI);
    gpio_pullup_en(SD_MISO);
    gpio_pullup_en(SD_SCK);

    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // SPI device configuration
    spi_device_interface_config_t dev_cfg = {
        .mode = 0, // SPI mode 0
        .spics_io_num = SD_CS,
        .clock_speed_hz = 100000, // Initialize at low clock speed (100 kHz)
        .queue_size = 1,
    };

    // Attach the SD card to the SPI bus
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi));

    sd_init_card();
}

void perform() {}