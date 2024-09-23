#ifndef SD_H
#define SD_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>

typedef struct 
{
    volatile uint16_t reserved : 15;
    volatile uint8_t v27_28 : 1;
    volatile uint8_t v28_29 : 1;
    volatile uint8_t v29_30 : 1;
    volatile uint8_t v30_31 : 1;
    volatile uint8_t v31_32 : 1;
    volatile uint8_t v32_33 : 1;
    volatile uint8_t v33_34 : 1;
    volatile uint8_t v34_35 : 1;
    volatile uint8_t v35_36 : 1;
    volatile uint8_t s18a : 1; // Switching to 1.8V accepted
    volatile uint8_t reserved_2 : 4;
    volatile uint8_t card_status : 1;
    volatile uint8_t card_capacity_status : 1;
    volatile uint8_t card_power_up_status_bit : 1; // Busy
} CMD58_OCR;

void send_dummy_clocks(void);

void sd_init_card(void);

// Sends an SD SPI commabdm the whole 48 bits
esp_err_t sd_send_command(uint8_t cmd, uint32_t arg);

// Resiece a byte from SD SPI
esp_err_t sd_recieve_response(uint8_t *response);

void sd_read_bytes(uint8_t *target, uint8_t count);

/**
 * // XXX might want to create an enum of all posibilities, for now only interested in idle state
 * Returns true if R1 suggests device is in idle state.
 *
 * R1 is 8 bytes:
 * 0 - idle state
 * 1 - erease reset
 * 2 - illegal
 * 3 - CRC error
 * 4 - erease seq error
 * 5 - address error
 * 6 - param error
 * 7 - not used
 */
bool sd_is_r1_ok(uint8_t *response);

/**
 * Extracts the 32 trailing data bits from an R3/R7 response.
 */
uint32_t get_ocr(uint8_t *response);

bool sd_is_valid_voltage(uint32_t ocr);

void sd_init();

#endif