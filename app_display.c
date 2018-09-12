#include "app_display.h"
#include "drv_led_matrix.h"
#include "gfx_glue_layer.h"
#include "nrf_gfx.h"
#include "nrf_lcd.h"
#include "font.h"

static nrf_lcd_t m_led_matrix = GFX_LED_DRV_MATRIX;

void app_display_init(void)
{
    uint32_t err_code;
    static lcd_cb_t lcd_cb;
    lcd_cb.height = MATRIX_PIXEL_HEIGHT;
    lcd_cb.width = MATRIX_PIXEL_WIDTH;
    lcd_cb.rotation = NRF_LCD_ROTATE_0;
    lcd_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
    
    m_led_matrix.p_lcd_cb = &lcd_cb;
    err_code = nrf_gfx_init(&m_led_matrix);
    APP_ERROR_CHECK(err_code);
    
        nrf_gfx_point_t point = {.x = 10, .y = 10};
    err_code = nrf_gfx_print(&m_led_matrix, &point, CL_GREEN, "Hei", &orkney_8ptFontInfo, false);
}

void app_display_draw_paddles(uint32_t p1_pos, uint32_t p2_pos, uint32_t p1_color, uint32_t p2_color)
{
    static uint32_t p1_pos_last = 0xFFFF;
    static uint32_t p1_col_last = 0;
    static uint32_t p2_pos_last = 0xFFFF;
    static uint32_t p2_col_last = 0;
    
    if(p1_pos != p1_pos_last || p1_color != p1_col_last)
    {
        if(p1_pos < (PADDLE_HEIGHT / 2)) p1_pos = PADDLE_HEIGHT / 2;
        else if(p1_pos > (32 - (PADDLE_HEIGHT / 2))) p1_pos = 32 - (PADDLE_HEIGHT / 2);
        nrf_gfx_rect_t rect = {.x = 0, .y = 0, .width = 2, .height = 32};
        nrf_gfx_rect_draw(&m_led_matrix, &rect, 1, CL_BLACK, true);
        nrf_gfx_rect_t rect2 = {.x = 0, .y = p1_pos - (PADDLE_HEIGHT / 2), .width = 2, .height = PADDLE_HEIGHT};
        nrf_gfx_rect_draw(&m_led_matrix, &rect2, 1, p1_color, true);
        p1_pos_last = p1_pos;
        p1_col_last = p1_color;
    }
    
    if(p2_pos != p2_pos_last || p2_color != p2_col_last)
    {
        if(p2_pos < (PADDLE_HEIGHT / 2)) p2_pos = PADDLE_HEIGHT / 2;
        else if(p2_pos > (32 - (PADDLE_HEIGHT / 2))) p2_pos = 32 - (PADDLE_HEIGHT / 2);    
        nrf_gfx_rect_t rect3 = {.x = 62, .y = 0, .width = 2, .height = 32};
        nrf_gfx_rect_draw(&m_led_matrix, &rect3, 1, CL_BLACK, true);
        nrf_gfx_rect_t rect4 = {.x = 62, .y = p2_pos - (PADDLE_HEIGHT / 2), .width = 2, .height = PADDLE_HEIGHT};
        nrf_gfx_rect_draw(&m_led_matrix, &rect4, 1, p2_color, true);
        p2_pos_last = p2_pos;
        p2_col_last = p2_color;
    }
}

void app_display_draw_ball(uint32_t pos_x, uint32_t pos_y)
{
    static uint32_t previous_pos_x = 1, previous_pos_y = 1;
    static nrf_gfx_rect_t rect_ball_prev = {.x = 1, .y = 1, .width = 2, .height = 2};
    nrf_gfx_rect_t rect_ball =  {.x = pos_x + 2, .y = pos_y, .width = 2, .height = 2};
    nrf_gfx_rect_draw(&m_led_matrix, &rect_ball_prev, 1, CL_BLACK, true);
    nrf_gfx_rect_draw(&m_led_matrix, &rect_ball, 1, CL_WHITE, true);
    
    rect_ball_prev = rect_ball;
    previous_pos_x = pos_x;
    previous_pos_y = pos_y;
}

