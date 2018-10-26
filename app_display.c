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
}

void app_display_draw_paddles(uint32_t p1_pos, uint32_t p2_pos, uint32_t p1_color, uint32_t p2_color, bool invalidate)
{
    static uint32_t p1_pos_last = 0xFFFF;
    static uint32_t p1_col_last = 0;
    static uint32_t p2_pos_last = 0xFFFF;
    static uint32_t p2_col_last = 0;
    uint32_t y1, y2;

    if(invalidate || p1_pos != p1_pos_last || p1_color != p1_col_last)
    {
        if(p1_pos < (PADDLE_HEIGHT / 2))
        {
            y1 = 0;
            y2 = p1_pos + (PADDLE_HEIGHT / 2);
        }
        else if(p1_pos > (32 - (PADDLE_HEIGHT / 2)))
        {
            y1 = p1_pos - (PADDLE_HEIGHT / 2);
            y2 = 32;
        }
        else 
        {
             y1 = p1_pos - (PADDLE_HEIGHT / 2);     
             y2 = p1_pos + (PADDLE_HEIGHT / 2);      
        }
        nrf_gfx_rect_t rect = {.x = 0, .y = 0, .width = 2, .height = 32};
        nrf_gfx_rect_draw(&m_led_matrix, &rect, 1, CL_BLACK, true);
        nrf_gfx_rect_t rect2 = {.x = 0, .y = y1, .width = 2, .height = (y2 - y1)};
        nrf_gfx_rect_draw(&m_led_matrix, &rect2, 1, p1_color, true);
        p1_pos_last = p1_pos;
        p1_col_last = p1_color;
    }
    
    if(invalidate || p2_pos != p2_pos_last || p2_color != p2_col_last)
    {
        if(p2_pos < (PADDLE_HEIGHT / 2))
        {
            y1 = 0;
            y2 = p2_pos + (PADDLE_HEIGHT / 2);
        }
        else if(p2_pos > (32 - (PADDLE_HEIGHT / 2)))
        {
            y1 = p2_pos - (PADDLE_HEIGHT / 2);
            y2 = 32;
        }
        else 
        {
             y1 = p2_pos - (PADDLE_HEIGHT / 2);     
             y2 = p2_pos + (PADDLE_HEIGHT / 2);      
        }
         
        nrf_gfx_rect_t rect3 = {.x = 62, .y = 0, .width = 2, .height = 32};
        nrf_gfx_rect_draw(&m_led_matrix, &rect3, 1, CL_BLACK, true);
        nrf_gfx_rect_t rect4 = {.x = 62, .y = y1, .width = 2, .height = (y2 - y1)};
        nrf_gfx_rect_draw(&m_led_matrix, &rect4, 1, p2_color, true);
        p2_pos_last = p2_pos;
        p2_col_last = p2_color;
    }
}

void app_display_draw_ball(uint32_t pos_x, uint32_t pos_y)
{
    static nrf_gfx_rect_t rect_ball_prev = {.x = 1, .y = 1, .width = 2, .height = 2};
    nrf_gfx_rect_t rect_ball =  {.x = pos_x + 2, .y = pos_y, .width = 2, .height = 2};
    nrf_gfx_rect_draw(&m_led_matrix, &rect_ball_prev, 1, CL_BLACK, true);
    nrf_gfx_rect_draw(&m_led_matrix, &rect_ball, 1, CL_WHITE, true);
    
    rect_ball_prev = rect_ball;
}

uint32_t get_text_width(char *text, const nrf_gfx_font_desc_t * p_font)
{
    uint32_t text_width = 0;
    while(*text)
    {
        text_width += (uint32_t)(p_font->charInfo[*text - p_font->startChar].widthBits + p_font->spacePixels);
        text++;
    }
    return text_width;
}

void app_display_draw_text(char *text, uint32_t x, uint32_t y, uint32_t color, text_alignment_t alignment)
{
    const nrf_gfx_font_desc_t *m_font = &arialNarrow_8ptFontInfo;
    nrf_gfx_point_t point;
    switch(alignment)
    {
        case ALIGNMENT_LEFT:
            point.x = x;
            point.y = y;
            break;
            
        case ALIGNMENT_CENTRE:
            point.x = x - (get_text_width(text, m_font) / 2);
            point.y = y;        
            break;
    }
    //color = (color & 0xF8) << 8 | (color & 0xFC00) >> 5 | (color & 0xF80000) >> 19;
    uint32_t err_code = nrf_gfx_print(&m_led_matrix, &point, color, text, m_font, false);    
}

void app_display_draw_int(int32_t value, uint32_t x, uint32_t y, uint32_t color, text_alignment_t alignment)
{
    static char str_buf[9];
    int i = 0;
    if(value < 0)
    {
        str_buf[i++] = '-';
        value *= -1;
    }
    if(value == 0)
    {
        str_buf[0] = '0';
        str_buf[1] = 0;
    }
    else
    {
       for(; i < 8 && value > 0; i++)
       {
           str_buf[i] = (value % 10) + '0';
           value /= 10;
       }
       str_buf[i] = 0;
    }
    app_display_draw_text(str_buf, x, y, color, alignment);
}

void app_display_draw_square(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    nrf_gfx_rect_t rect = {.x = x, .y = y, .width = w, .height = h};
    nrf_gfx_rect_draw(&m_led_matrix, &rect, 1, color, true);
}

void app_display_clear_screen(void)
{
    nrf_gfx_screen_fill(&m_led_matrix, CL_BLACK);
}


void app_display_score_state(pong_gamestate_t *gamestate)
{
    app_display_draw_int(gamestate->player[0].score, 16, 20, gamestate->player[0].color, ALIGNMENT_CENTRE);
    app_display_draw_int(gamestate->player[1].score, 48, 20, gamestate->player[1].color, ALIGNMENT_CENTRE);
}

