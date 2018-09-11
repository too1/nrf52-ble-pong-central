
#ifndef GFX_GLUE_LAYER_H__
#define GFX_GLUE_LAYER_H__

#include "nrf_lcd.h"

#define GFX_LED_DRV_MATRIX  \
{   \
    .lcd_display          = gfx_glue_lcd_display,  \
    .lcd_display_invert   = gfx_glue_lcd_display_invert,  \
    .lcd_init             = gfx_glue_lcd_init,  \
    .lcd_pixel_draw       = gfx_glue_lcd_pixel_draw,  \
    .lcd_rect_draw        = gfx_glue_lcd_rect_draw,  \
    .lcd_rotation_set     = gfx_glue_lcd_rotation_set,  \
    .lcd_uninit           = gfx_glue_lcd_uninit,  \
}

// Function for initializing the LCD controller.
ret_code_t gfx_glue_lcd_init(void);

// Function for uninitializing the LCD controller.
void gfx_glue_lcd_uninit(void);

// Function for drawing a single pixel.
void gfx_glue_lcd_pixel_draw(uint16_t x, uint16_t y, uint32_t color);

// Function for drawing a filled rectangle.
void gfx_glue_lcd_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

// Function for displaying data from an internal frame buffer. 
void gfx_glue_lcd_display(void);

// Function for rotating the screen.
void gfx_glue_lcd_rotation_set(nrf_lcd_rotation_t rotation);

// Function for setting inversion of colors on the screen.
void gfx_glue_lcd_display_invert(bool invert);

#endif // GFX_GLUE_LAYER_H__
