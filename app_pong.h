#ifndef __APP_PONG_H
#define __APP_PONG_H

#include <stdint.h>
#include <stdbool.h>
#include "app_timer.h"

#define COLOR_RGB(r, g, b)              (((r) & 0xFF) << 16 | ((g) & 0xFF) << 8 | ((b) & 0xFF))
#define GAME_LOOP_UPDATE_MS             25
#define GAME_LOOP_UPDATE_FREQ           (1000 / GAME_LOOP_UPDATE_MS)
#define PONG_NUM_PLAYERS                2

#define PADDLE_CONTROLLER_RANGE         100

#define LEVEL_SIZE_X                    1000
#define LEVEL_SIZE_Y                    500
#define PADDLE_SIZE_Y                   ((LEVEL_SIZE_Y / 4) + 30)
#define PADDLE_HALFSIZE_Y               (PADDLE_SIZE_Y / 2)

#define PI                              3.141592f
#define PI_1_2                          1.570796f
#define PI_3_2                          4.712389f
#define PONG_DIRECTION_RIGHT            0.0f
#define PONG_DIRECTION_DOWN             PI_1_2
#define PONG_DIRECTION_LEFT             PI
#define PONG_DIRECTION_UP               PI_3_2

#define PONG_PREDELAY_TIME_S            4
#define PONG_POINT_SCORED_SHOW_TIME_S   3
#define PONG_SCORE_LIMIT                3
#define PONG_SPEED_INC_INTERVAL         10
#define PONG_SPEED_REDUCTION_PR_BALL    1
#define PONG_PADDLE_SPEED_TO_BALL_RATIO 30
#define PONG_BALL_START_SPEED_X         8
#define PONG_BALL_START_SPEED_Y         5
#define PONG_BALL_START_SPEED           8.0f
#define PONG_BALL_SPEED_INC_AMOUNT      1.2f
#define PONG_BALL_Y_SPEED_RED_DIVIDER   8
#define PONG_BALL_ROTATION_DECAY_FACTOR 0.95f

typedef enum {SOUND_SAMPLE_COLLECT_POINT_A,
              SOUND_SAMPLE_COLLECT_POINT_B,
              SOUND_SAMPLE_EXPLOSION_A,
              SOUND_SAMPLE_EXPLOSION_B,
              SOUND_SAMPLE_HIT,
              SOUND_SAMPLE_PICKUP_A,
              SOUND_SAMPLE_PICKUP_B,
              SOUND_SAMPLE_SHOOT_A,
              SOUND_SAMPLE_SHOOT_B
              }pong_sound_sample_t;

typedef enum {CONSTATE_DISCONNECTED,    // Thingy disconnected
              CONSTATE_CONNECTED,       // Thingy connected, services disabled
              CONSTATE_ACTIVE           // Thingy connected and services enabled
             }controller_state_t;
  
typedef enum {STATE_INVALID, 
              STATE_WAITING_FOR_PLAYERS,
              STATE_GAME_TOURNAMENT_ROUND_STARTING,
              STATE_GAME_START_NEW_GAME,
              STATE_GAME_START_PREDELAY,
              STATE_GAME_RUNNING,
              STATE_GAME_POINT_SCORED,
              STATE_GAME_SCORE_LIMIT_REACHED,
      
             }pong_main_state_t;

typedef struct
{
    uint16_t connected_state;   // The state of the Thingy associated with this player
    uint16_t con_handle;        // The connection handle of the Thingy associated with this player
    uint32_t paddle_pos_y;      // Current paddle position
    int32_t  paddle_pos_y_delta;
    uint32_t score;             // Current score of the player
    uint32_t color;             // The GUI color of the player
    uint8_t  *name;             // Pointer to a string with the name of the player
}pong_player_state_t;

typedef struct
{
    pong_player_state_t player[PONG_NUM_PLAYERS];
    
    int32_t pong_pos_x;
    int32_t pong_pos_y;
    float   ball_speed;
    float   ball_direction;
    float   ball_rotation;
    float   ball_pos_x;
    float   ball_pos_y;
    uint32_t time_since_last_speed_increment;
    float   ball_speed_inc_factor;
}pong_gamestate_t;

typedef struct
{
    uint32_t paddle_x;
    int32_t  paddle_x_delta;
    bool     button_pressed;
    uint8_t  battery_level;
}pong_controller_state_t;

typedef struct
{
    pong_controller_state_t player[PONG_NUM_PLAYERS];
    bool                    mobile_app_connected;
}pong_global_control_state_t;

typedef enum {PONG_EVENT_GAMESTATE_UPDATE, 
              PONG_EVENT_CON_SET_COLOR, 
              PONG_EVENT_PLAY_SOUND,
              PONG_EVENT_GAME_STARTED,
              PONG_EVENT_POINT_SCORED,
              PONG_EVENT_GAME_OVER}pong_event_type_t;
    
typedef struct 
{
    uint32_t color;
    uint16_t conn_handle;
}pong_event_param_con_set_color_t;

typedef struct
{
    uint8_t  sample_id;
    uint16_t controller_index;
}pong_event_param_play_sound_t;

typedef struct
{
    uint8_t player_index;
}pong_event_param_point_scored_t;

typedef struct
{
    uint8_t winner_index;
}pong_event_param_game_over_t;

typedef struct
{
    pong_event_type_t evt_type;
    pong_gamestate_t *game_state;
    union
    {
        pong_event_param_con_set_color_t con_set_color;
        pong_event_param_play_sound_t    play_sound;
        pong_event_param_point_scored_t  point_scored;
        pong_event_param_game_over_t     game_over;
    }params;
}pong_event_t;

typedef void (*pong_callback_t)(pong_event_t *event);

typedef struct
{
     pong_callback_t event_handler;
}pong_config_t;

uint32_t app_pong_init(pong_config_t *config);

uint32_t app_pong_start_game(void);

uint32_t app_pong_start_tournament_round(uint8_t *init_buffer, uint32_t init_buffer_length);

pong_controller_state_t *app_pong_get_controller(uint32_t index);

void app_pong_controller_status_change(uint16_t conn_handle, controller_state_t state);

void app_pong_set_mobile_app_connected_state(bool connected);

pong_gamestate_t *app_pong_get_gamestate(void);

void app_pong_draw_display(void);

#endif
