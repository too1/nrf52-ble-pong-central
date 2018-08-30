#ifndef __APP_PONG_H
#define __APP_PONG_H

#include <stdint.h>
#include <stdbool.h>
#include "app_timer.h"

#define GAME_LOOP_UPDATE_MS     25
#define PONG_NUM_PLAYERS        2
#define LEVEL_SIZE_X            1000
#define LEVEL_SIZE_Y            1000

typedef struct
{
    uint32_t paddle_pos_y;
    uint32_t score;
}pong_player_state_t;

typedef struct
{
    pong_player_state_t player[PONG_NUM_PLAYERS];
    
    int32_t pong_pos_x;
    int32_t pong_pos_y;
    int32_t pong_speed_x;
    int32_t pong_speed_y;
}pong_gamestate_t;

typedef struct
{
    uint32_t paddle_x;
}pong_controller_state_t;

typedef struct
{
    pong_controller_state_t player[PONG_NUM_PLAYERS];
}pong_global_control_state_t;

typedef enum {PONG_EVENT_GAMESTATE_UPDATE}pong_event_type_t;
    
typedef struct
{
    pong_event_type_t evt_type;
    pong_gamestate_t *game_state;
}pong_event_t;

typedef void (*pong_callback_t)(pong_event_t *event);

typedef struct
{
     pong_callback_t event_handler;
}pong_config_t;

uint32_t app_pong_init(pong_config_t *config);

uint32_t app_pong_start_game(void);

void app_pong_set_global_control_state(pong_global_control_state_t *control_state);

pong_gamestate_t *app_pong_get_gamestate(void);

#endif
