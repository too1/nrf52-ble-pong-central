#include "app_display.h"
#include "drv_led_matrix.h"
#include "gfx_glue_layer.h"
#include "nrf_gfx.h"
#include "nrf_lcd.h"
#include "font.h"
#include "string.h"
#include <stdio.h> 
#include <stdlib.h> 

static nrf_lcd_t m_led_matrix = GFX_LED_DRV_MATRIX;

static app_display_text_view_t * m_text_view_list[TEXT_VIEW_MAX_NUM];
static uint32_t                  m_text_view_num = 0;

static uint16_t                  m_rnd_screen_fade_buffer[MATRIX_PIXEL_WIDTH*MATRIX_PIXEL_HEIGHT];

void app_display_init(void)
{
    uint32_t err_code;
    static lcd_cb_t lcd_cb;
    lcd_cb.height = MATRIX_PIXEL_HEIGHT;
    lcd_cb.width = MATRIX_PIXEL_WIDTH;
    lcd_cb.rotation = NRF_LCD_ROTATE_0;
    lcd_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
    
    // Initialize random fade buffer
    uint16_t tmp, rnd_index;
    for(int i = 0; i < (MATRIX_PIXEL_WIDTH*MATRIX_PIXEL_HEIGHT); i++) m_rnd_screen_fade_buffer[i] = i;
    for(int i = 0; i < (MATRIX_PIXEL_WIDTH*MATRIX_PIXEL_HEIGHT); i++)
    {
        rnd_index = rand() % (MATRIX_PIXEL_WIDTH*MATRIX_PIXEL_HEIGHT);
        tmp = m_rnd_screen_fade_buffer[rnd_index];
        m_rnd_screen_fade_buffer[rnd_index] = m_rnd_screen_fade_buffer[i];
        m_rnd_screen_fade_buffer[i] = tmp;
    }

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

void app_display_text_view_add(app_display_text_view_t *text_view)
{
    if(m_text_view_num < TEXT_VIEW_MAX_NUM)
    {
        text_view->invalidate = true;
        text_view->pos_offset_x = 0;
        text_view->pos_offset_y = 0;
        text_view->pos_last_drawn_x = POS_INVALID;
        m_text_view_list[m_text_view_num++] = text_view;
    }
}


void app_display_text_view_set_color(app_display_text_view_t *text_view, uint32_t color)
{
    if(color != text_view->color)
    {
        text_view->color = color;
        text_view->invalidate = true;
    }
}

void app_display_text_view_set_text(app_display_text_view_t *text_view, char *text)
{
    //if(strcmp(text_view->string, text) != 0)
    {
        text_view->string = text;
        text_view->invalidate = true;
    }
}

void app_display_text_view_set_pos(app_display_text_view_t *text_view, uint32_t x, uint32_t y)
{
    if(text_view->pos_x != x || text_view->pos_y != y)
    {
        text_view->pos_x = x;
        text_view->pos_y = y;
        text_view->invalidate = true;
    }
}

void app_display_text_view_set_offset(app_display_text_view_t *text_view, int32_t offset_x, int32_t offset_y)
{
    if(text_view->pos_offset_x != offset_x || text_view->pos_offset_y != offset_y)
    {
        text_view->pos_offset_x = offset_x;
        text_view->pos_offset_y = offset_y;
        text_view->invalidate = true;
    }
}

void app_display_text_view_invalidate_all(void)
{
    for(int i = 0; i < m_text_view_num; i++)
    {
        m_text_view_list[i]->invalidate = true;
    }
}

void app_display_text_view_draw(app_display_text_view_t *text_view, bool clear_last_drawn)
{
    if(text_view->invalidate)
    {
        //uint32_t new_x = (uint32_t)x;
        //uint32_t new_y = (uint32_t)y;
        text_view->invalidate = false;
        if(clear_last_drawn && text_view->pos_last_drawn_x != POS_INVALID)
        {
            app_display_draw_text(text_view->last_drawn_string, text_view->pos_last_drawn_x, text_view->pos_last_drawn_y, CL_BLACK, text_view->alignment);
        }
        app_display_draw_text(text_view->string, text_view->pos_x + text_view->pos_offset_x, text_view->pos_y + text_view->pos_offset_y, text_view->color, text_view->alignment);
        text_view->pos_last_drawn_x = text_view->pos_x + text_view->pos_offset_x;
        text_view->pos_last_drawn_y = text_view->pos_y + text_view->pos_offset_y;
        text_view->last_drawn_string = text_view->string;
    }
}

void app_display_text_view_draw_w_shadow(app_display_text_view_t *text_view)
{
    if(text_view->invalidate)
    {
        text_view->invalidate = false;
        app_display_draw_text(text_view->string, text_view->pos_x + text_view->pos_offset_x - 1, text_view->pos_y + text_view->pos_offset_y - 1, CL_WHITE, text_view->alignment);
        app_display_draw_text(text_view->string, text_view->pos_x + text_view->pos_offset_x + 1, text_view->pos_y + text_view->pos_offset_y + 1, CL_BLACK, text_view->alignment);
        app_display_draw_text(text_view->string, text_view->pos_x + text_view->pos_offset_x, text_view->pos_y + text_view->pos_offset_y, text_view->color, text_view->alignment);
        text_view->pos_last_drawn_x = text_view->pos_x + text_view->pos_offset_x;
        text_view->pos_last_drawn_y = text_view->pos_y + text_view->pos_offset_y;
        text_view->last_drawn_string = text_view->string;
    }
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

        case ALIGNMENT_RIGHT:
            point.x = x - get_text_width(text, m_font);
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

void app_display_draw_bmp232(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t *img_buf)
{
    nrf_gfx_rect_t rect = {.x = x, .y = y, .width = w, .height = h};
    nrf_gfx_bmp565_draw(&m_led_matrix, &rect, img_buf);
}

void app_display_draw_image(uint32_t x, uint32_t y, pong_image_info_t *image)
{
    if(image != 0)
    {
        nrf_gfx_rect_t rect = {.x = x, .y = y, .width = image->width, .height = image->height};
        nrf_gfx_bmp565_draw(&m_led_matrix, &rect, image->data_ptr);
    }
}

void app_display_fade_to_black(uint32_t index)
{
    static uint32_t counter = 0;
    static nrf_gfx_point_t point;
    if(index == 0) counter = 0;
    else if(counter <= (MATRIX_PIXEL_WIDTH*MATRIX_PIXEL_HEIGHT-index))
    {
        for(int i = 0; i < index; i++)
        {
            point.x = m_rnd_screen_fade_buffer[i + counter] % 64;
            point.y = m_rnd_screen_fade_buffer[i + counter] / 64;
            nrf_gfx_point_draw(&m_led_matrix, &point, CL_BLACK);
        }
        counter += index;
    }

}

void app_display_fade_to_image(uint32_t x, uint32_t y, pong_image_info_t *image, uint32_t index)
{
    static uint32_t counter = 0;
    static nrf_gfx_point_t point;
    uint32_t pixel_color;
    uint8_t current_pixel;
    if(index == 0) counter = 0;
    else if(counter <= (MATRIX_PIXEL_WIDTH*MATRIX_PIXEL_HEIGHT-index))
    {
        for(int i = 0; i < index; i++)
        {
            point.x = m_rnd_screen_fade_buffer[i + counter] % 64;
            point.y = m_rnd_screen_fade_buffer[i + counter] / 64;
            current_pixel = image->data_ptr[point.x + (image->height - 1 - point.y) * image->width];
            pixel_color =  (current_pixel & 0x03) << (6 + 16);            
            pixel_color |= (current_pixel & 0x0C) <<  (4 + 8);
            pixel_color |= (current_pixel & 0x30) <<  (2 + 0);
            nrf_gfx_point_draw(&m_led_matrix, &point, pixel_color);
        }
        counter += index;
    }
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

