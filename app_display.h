#ifndef __APP_DISPLAY_H
#define __APP_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "drv_led_matrix.h"
#include "app_pong.h"

#define CL_BLACK    0
#define CL_BLUE     BLUE
#define CL_RED      RED
#define CL_GREEN    GREEN
#define CL_PURPLE   (RED | BLUE)
#define CL_TEAL     (GREEN | BLUE)
#define CL_YELLOW   (RED | GREEN)
#define CL_WHITE    (RED | GREEN | BLUE)

#define PADDLE_HEIGHT   8

#define TEXT_VIEW_MAX_NUM   20

#define POS_INVALID     0xFFFFFF

typedef enum {ALIGNMENT_LEFT, ALIGNMENT_CENTRE, ALIGNMENT_RIGHT} text_alignment_t;

typedef struct
{
    // Data fields
    char            *string;
    uint32_t         pos_x;
    uint32_t         pos_y;
    uint32_t         color;
    text_alignment_t alignment;
    
    // Internal fields
    bool             invalidate;
    int32_t          pos_offset_x;
    int32_t          pos_offset_y;
    int32_t          pos_last_drawn_x;
    int32_t          pos_last_drawn_y;
} app_display_text_view_t;

void app_display_init(void);

void app_display_draw_paddles(uint32_t p1_pos, uint32_t p2_pos, uint32_t p1_color, uint32_t p2_color, bool invalidate);

void app_display_draw_ball(uint32_t pos_x, uint32_t pos_y);

void app_display_score_state(pong_gamestate_t *gamestate);

void app_display_text_view_add(app_display_text_view_t *text_view);

void app_display_text_view_set_color(app_display_text_view_t *text_view, uint32_t color);

void app_display_text_view_set_text(app_display_text_view_t *text_view, char *text);

void app_display_text_view_set_pos(app_display_text_view_t *text_view, uint32_t x, uint32_t y);

void app_display_text_view_set_offset(app_display_text_view_t *text_view, int32_t offset_x, int32_t offset_y);

void app_display_text_view_invalidate_all(void);

void app_display_text_view_draw(app_display_text_view_t *text_view, bool clear_last_drawn);

void app_display_draw_text(char *text, uint32_t x, uint32_t y, uint32_t color, text_alignment_t alignment);

void app_display_draw_int(int32_t value, uint32_t x, uint32_t y, uint32_t color, text_alignment_t alignment);

void app_display_draw_square(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

void app_display_clear_screen(void);

#endif
