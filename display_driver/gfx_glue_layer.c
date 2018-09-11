
#include <stdbool.h>
#include <stdint.h>
#include "nrf_assert.h"
#include "nrf_gpio.h"
#include "sdk_config.h"
#include "sdk_errors.h"
#include "nrf_lcd.h"
#include "drv_led_matrix.h"
#include "gfx_glue_layer.h"

// Function for initializing the LCD controller.
ret_code_t gfx_glue_lcd_init(void)
{
    return drv_led_matrix_init();
}

// Function for uninitializing the LCD controller.
void gfx_glue_lcd_uninit(void)
{
    // No implementation
}

// Function for drawing a single pixel.
void gfx_glue_lcd_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    drv_led_matrix_draw_pixel(x, y, color);
}

// Function for drawing a filled rectangle.
void gfx_glue_lcd_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    drv_led_matrix_draw_rectangle(x, y, width, height, color);
}

// Function for displaying data from an internal frame buffer. 
void gfx_glue_lcd_display(void)
{
    // No implementation
}

// Function for rotating the screen.
void gfx_glue_lcd_rotation_set(nrf_lcd_rotation_t rotation)
{
    // No implementation
}

// Function for setting inversion of colors on the screen.
void gfx_glue_lcd_display_invert(bool invert)
{
    // No implementation
}


