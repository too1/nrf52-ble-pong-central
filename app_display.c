#include "app_display.h"
#include "drv_led_matrix.h"


void app_display_init(void)
{
    drv_led_matrix_init();
}

void app_display_draw_paddles(uint32_t p1_pos, uint32_t p2_pos, uint32_t p1_color, uint32_t p2_color)
{
    if(p1_pos < (PADDLE_HEIGHT / 2)) p1_pos = PADDLE_HEIGHT / 2;
    else if(p1_pos > (32 - (PADDLE_HEIGHT / 2))) p1_pos = 32 - (PADDLE_HEIGHT / 2);
    if(p2_pos < (PADDLE_HEIGHT / 2)) p2_pos = PADDLE_HEIGHT / 2;
    else if(p2_pos > (32 - (PADDLE_HEIGHT / 2))) p2_pos = 32 - (PADDLE_HEIGHT / 2);

    drv_led_matrix_draw_rectangle(0, 0, 2, 32, CL_BLACK);
    drv_led_matrix_draw_rectangle(0, p1_pos - (PADDLE_HEIGHT / 2), 2, PADDLE_HEIGHT, p1_color);
    
    drv_led_matrix_draw_rectangle(62, 0, 2, 32, CL_BLACK);
    drv_led_matrix_draw_rectangle(62, p2_pos - (PADDLE_HEIGHT / 2), 2, PADDLE_HEIGHT, p2_color);
}

void app_display_draw_ball(uint32_t pos_x, uint32_t pos_y)
{
    static uint32_t previous_pos_x = 1, previous_pos_y = 1;
    
    drv_led_matrix_draw_rectangle(previous_pos_x + 2, previous_pos_y, 2, 2, CL_BLACK);
    drv_led_matrix_draw_rectangle(pos_x + 2, pos_y, 2, 2, CL_WHITE);    
    
    previous_pos_x = pos_x;
    previous_pos_y = pos_y;
}

