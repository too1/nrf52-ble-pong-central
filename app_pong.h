#ifndef __APP_PONG_H
#define __APP_PONG_H

#include <stdint.h>
#include <stdbool.h>
#include "app_timer.h"

#define GAME_LOOP_UPDATE_MS     25
#define GAME_LOOP_UPDATE_FREQ   (1000 / GAME_LOOP_UPDATE_MS)
#define PONG_NUM_PLAYERS        2

#define PADDLE_CONTROLLER_RANGE 100

#define LEVEL_SIZE_X            1000
#define LEVEL_SIZE_Y            500
#define PADDLE_SIZE_Y           LEVEL_SIZE_Y / 4
#define PADDLE_HALFSIZE_Y       (PADDLE_SIZE_Y / 2)

#define PONG_PREDELAY_TIME_S    5
#define PONG_SCORE_LIMIT        5
#define PONG_SPEED_INC_INTERVAL 10

typedef enum {CONSTATE_DISCONNECTED,    // Thingy disconnected
              CONSTATE_CONNECTED,       // Thingy connected, services disabled
              CONSTATE_ACTIVE           // Thingy connected and services enabled
             }controller_state_t;
  
typedef enum {STATE_WAITING_FOR_PLAYERS,
              STATE_GAME_START_PREDELAY,
              STATE_GAME_RUNNING,
              STATE_GAME_SCORE_LIMIT_REACHED,
      
             }pong_main_state_t;

typedef struct
{
    uint16_t connected_state;   // The state of the Thingy associated with this player
    uint16_t con_handle;        // The connection handle of the Thingy associated with this player
    uint32_t paddle_pos_y;      // Current paddle position
    uint32_t score;             // Current score of the player
    uint32_t color;             // The GUI color of the player
}pong_player_state_t;

typedef struct
{
    pong_player_state_t player[PONG_NUM_PLAYERS];
    
    int32_t pong_pos_x;
    int32_t pong_pos_y;
    int32_t pong_speed_x;
    int32_t pong_speed_y;
    uint32_t time_since_last_speed_increment;
    uint32_t speed_multiplier_factor;
}pong_gamestate_t;

typedef struct
{
    uint32_t paddle_x;
}pong_controller_state_t;

typedef struct
{
    pong_controller_state_t player[PONG_NUM_PLAYERS];
}pong_global_control_state_t;

typedef enum {PONG_EVENT_GAMESTATE_UPDATE, PONG_EVENT_CON_SET_COLOR}pong_event_type_t;
    
typedef struct 
{
    uint32_t color;
    uint16_t conn_handle;
}pong_event_param_con_set_color_t;

typedef struct
{
    pong_event_type_t evt_type;
    pong_gamestate_t *game_state;
    union
    {
        pong_event_param_con_set_color_t con_set_color;
    }params;
}pong_event_t;

typedef void (*pong_callback_t)(pong_event_t *event);

typedef struct
{
     pong_callback_t event_handler;
}pong_config_t;

uint32_t app_pong_init(pong_config_t *config);

uint32_t app_pong_start_game(void);

void app_pong_set_global_control_state(pong_global_control_state_t *control_state);

void app_pong_controller_status_change(uint16_t conn_handle, controller_state_t state);

pong_gamestate_t *app_pong_get_gamestate(void);

void app_pong_draw_display(void);

#endif
