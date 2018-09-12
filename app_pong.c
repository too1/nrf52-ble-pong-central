#include "app_pong.h"

APP_TIMER_DEF(m_game_loop_timer_id);

static bool                         m_game_initialized = false;
static pong_callback_t              m_event_callback;

static pong_gamestate_t             m_gamestate;
static pong_global_control_state_t  m_global_control_state;

static pong_event_t                 m_pong_event;

// "Private" functions
static void reset_ball(void)
{
    m_gamestate.pong_pos_x = LEVEL_SIZE_X / 2;
    m_gamestate.pong_pos_y = LEVEL_SIZE_Y / 2;
    m_gamestate.pong_speed_x = 8;
    m_gamestate.pong_speed_y = 5;    
}

static void reset_game(void)
{
    reset_ball();
    for(int i = 0; i < PONG_NUM_PLAYERS; i++)
    {
        m_gamestate.player[i].paddle_pos_y = LEVEL_SIZE_Y / 2;
        m_gamestate.player[i].score = 0;
    }    
}

static void update_game_loop(void *p)
{
    // Update paddle positions based on game input
    for(int i = 0; i < PONG_NUM_PLAYERS; i++)
    {
        m_gamestate.player[i].paddle_pos_y = m_global_control_state.player[i].paddle_x;
    }

    // Update pong X position
    m_gamestate.pong_pos_x += m_gamestate.pong_speed_x;
    
    // Check if the ball has reached one of the board edges
    if(m_gamestate.pong_pos_x > LEVEL_SIZE_X)
    {
        // Right edge hit. Check if paddle is in position
        if(m_gamestate.pong_pos_y > (m_gamestate.player[1].paddle_pos_y - PADDLE_HALFSIZE_Y) && 
           m_gamestate.pong_pos_y < (m_gamestate.player[1].paddle_pos_y + PADDLE_HALFSIZE_Y))
        {
            m_gamestate.pong_pos_x = LEVEL_SIZE_X;
            m_gamestate.pong_speed_x *= -1;
        }
        else
        {
            reset_ball();
        }
    }
    else if(m_gamestate.pong_pos_x < 0)
    {
        // Left edge hit. Check if the paddle is in position
        if(m_gamestate.pong_pos_y > (m_gamestate.player[0].paddle_pos_y - PADDLE_HALFSIZE_Y) && 
           m_gamestate.pong_pos_y < (m_gamestate.player[0].paddle_pos_y + PADDLE_HALFSIZE_Y))
        {
            m_gamestate.pong_pos_x = 0;
            m_gamestate.pong_speed_x *= -1;
        }
        else
        {
            reset_ball();
        }     
    }

    // Update pong Y position
    m_gamestate.pong_pos_y += m_gamestate.pong_speed_y;
    if(m_gamestate.pong_pos_y > LEVEL_SIZE_Y)
    {
        m_gamestate.pong_pos_y = LEVEL_SIZE_Y;
        m_gamestate.pong_speed_y *= -1;
    }
    else if(m_gamestate.pong_pos_y < 0)
    {
        m_gamestate.pong_pos_y = 0;
        m_gamestate.pong_speed_y *= -1;        
    }

    // Send event back
    m_pong_event.evt_type = PONG_EVENT_GAMESTATE_UPDATE;
    m_pong_event.game_state = &m_gamestate;
    m_event_callback(&m_pong_event);
}

// "Public" functions
uint32_t app_pong_init(pong_config_t *config)
{
    if(config->event_handler == 0) return -1;
    m_event_callback = config->event_handler;
    m_game_initialized = true;
    app_timer_create(&m_game_loop_timer_id, APP_TIMER_MODE_REPEATED, update_game_loop);
    reset_game();
    return 0;
}

uint32_t app_pong_start_game(void)
{
    if(!m_game_initialized) return -1;
    app_timer_start(m_game_loop_timer_id, APP_TIMER_TICKS(GAME_LOOP_UPDATE_MS), 0);
    return 0;
}

void app_pong_set_global_control_state(pong_global_control_state_t *control_state)
{
    m_global_control_state = *control_state;
}

pong_gamestate_t *app_pong_get_gamestate(void)
{
    return &m_gamestate;
}