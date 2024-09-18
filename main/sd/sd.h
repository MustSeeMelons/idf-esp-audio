#ifndef SD_H
#define SD_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>

void send_dummy_clocks(void);

void sd_init_card(void);

// Sends an SD SPI commabdm the whole 48 bits
esp_err_t sd_send_command(uint8_t cmd, uint32_t arg);

// Resiece a byte from SD SPI
esp_err_t sd_recieve_response(uint8_t* response);

void sd_init();

void perform();

#endif