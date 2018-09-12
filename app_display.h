#ifndef __APP_DISPLAY_H
#define __APP_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define CL_BLACK    0
#define CL_BLUE     BLUE
#define CL_RED      RED
#define CL_GREEN    GREEN
#define CL_PURPLE   (RED | BLUE)

#define PADDLE_HEIGHT   8


void app_display_init(void);

void app_display_draw_paddles(uint32_t p1_pos, uint32_t p2_pos);

void app_display_draw_ball(uint32_t pos_x, uint32_t pos_y);

#endif
