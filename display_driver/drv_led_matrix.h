
#ifndef DRV_DRV_LED_MATRIX_H__
#define DRV_DRV_LED_MATRIX_H__

#include <stdbool.h>
#include <stdint.h>
#include "sdk_errors.h"
#include "nrf_assert.h"
#include "nrf_gpio.h"
#include "sdk_config.h"
//#include "nrf_drv_common.h"

#define MATRIX_PIXEL_WIDTH      64
#define MATRIX_PIXEL_HEIGHT     32
#define MATRIX_BUFFER_WIDTH     MATRIX_PIXEL_WIDTH / 8
#define MATRIX_BUFFER_HEIGHT    MATRIX_PIXEL_HEIGHT

#define MATRIX_MULTI_DRAW       3

#define CONFIG_PIN_AS_OUTPUT_AND_CLEAR(pin) {nrf_gpio_cfg_output(pin); nrf_gpio_pin_clear(pin);}
#define CONFIG_PIN_AS_OUTPUT_AND_SET(pin) {nrf_gpio_cfg_output(pin); nrf_gpio_pin_set(pin);}


typedef enum
{
    RED = 0xFF0000, 
    GREEN = 0xFF00,
    BLUE = 0xFF
}rgb_color_t;


typedef struct
{
    uint64_t r[MATRIX_BUFFER_HEIGHT];
    uint64_t g[MATRIX_BUFFER_HEIGHT];
    uint64_t b[MATRIX_BUFFER_HEIGHT];
}led_matrix_buffer_t;


/**********************
 * PIN DEFINES. THE MATRIX USES 13 DIGITAL PINS
 *********************/

/**
 *Pins R1, G1 and B1 deliver data to the top half of the display.
 */
#define PIN_R_1 26
#define PIN_B_1 02
#define PIN_G_1 27

/**
 * Pins R2, G2 and B2 deliver data to the bottom half of the display
 */
#define PIN_R_2 25
#define PIN_B_2 23
#define PIN_G_2 24

/**
 * Pins A, B, C and D select which two rows of the display are currently lit.
 * (32x16 matrices don’t have a “D” pin — it’s connected to ground instead.)
 */
#define PIN_LA 16
#define PIN_LB 15
#define PIN_LC 14
#define PIN_LD 13 

/**
 * The LAT (latch) signal marks the end of a row of data.
 */
#define PIN_LAT 4

/**
 * The CLK (clock) signal marks the arrival of each bit of data.
 */
#define PIN_CLK 3
 
/**
 * OE (output enable) switches the LEDs off when transitioning from one row to the next
 */
#define PIN_OE 28


uint32_t drv_led_matrix_init(void);

uint32_t drv_led_matrix_draw_pixel_rgb(uint8_t x, uint8_t y, uint8_t color_r, uint8_t color_g, uint8_t color_b);

uint32_t drv_led_matrix_draw_pixel(uint8_t x, uint8_t y, uint32_t color);

uint32_t drv_led_matrix_draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

void led_matrix_clear_screen(void);

 
#endif // DRV_DRV_LED_MATRIX_H__
