#include "app_pong.h"
#include "app_display.h"

APP_TIMER_DEF(m_game_loop_timer_id);

static bool                         m_game_initialized = false;
static pong_callback_t              m_event_callback;

static pong_gamestate_t             m_gamestate;
static pong_global_control_state_t  m_global_control_state;

static pong_main_state_t            m_main_state = STATE_WAITING_FOR_PLAYERS; 
static uint32_t state_lifetime = 0;

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

static void update_game_running(void)
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
}

static void update_game_loop(void *p)
{
    switch(m_main_state)
    {
        case STATE_WAITING_FOR_PLAYERS:
            break;
            
        case STATE_GAME_RUNNING:
            update_game_running();
            break;
    }

    // Send event back
    m_pong_event.evt_type = PONG_EVENT_GAMESTATE_UPDATE;
    m_pong_event.game_state = &m_gamestate;
    m_event_callback(&m_pong_event);
}

static void set_main_state(uint32_t new_state)
{
    m_main_state = new_state;
    state_lifetime = 0;
}

// "Public" functions
uint32_t app_pong_init(pong_config_t *config)
{
    if(config->event_handler == 0) return -1;
    m_event_callback = config->event_handler;
    m_game_initialized = true;
    app_timer_create(&m_game_loop_timer_id, APP_TIMER_MODE_REPEATED, update_game_loop);
    reset_game();
    m_gamestate.player[0].connected_state = CONSTATE_DISCONNECTED;
    m_gamestate.player[1].connected_state = CONSTATE_DISCONNECTED;
    m_gamestate.player[0].color = CL_TEAL;
    m_gamestate.player[1].color = CL_YELLOW;
    m_main_state = STATE_WAITING_FOR_PLAYERS;
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

void app_pong_controller_status_change(uint16_t conn_handle, controller_state_t state)
{
    switch(state)
    {
        case CONSTATE_DISCONNECTED:
            // Look for an existing player and assign the state to it
            for(int i = 0; i < PONG_NUM_PLAYERS; i++)
            {
               if(m_gamestate.player[i].con_handle == conn_handle)
               {
                   m_gamestate.player[i].connected_state = CONSTATE_DISCONNECTED;
                   m_gamestate.player[i].con_handle = 0xFFFF;
                   break;
               }                
            }
           
            // Stop the game if running
            if(m_main_state == STATE_GAME_RUNNING)
            {
                set_main_state(STATE_WAITING_FOR_PLAYERS);
            }
            break;
            
        case CONSTATE_CONNECTED:
            // Look for a free player to assign the newly connected controller to
            for(int i = 0; i < PONG_NUM_PLAYERS; i++)
            {
               if(m_gamestate.player[i].connected_state == CONSTATE_DISCONNECTED)
               {
                   m_gamestate.player[i].connected_state = CONSTATE_CONNECTED;
                   m_gamestate.player[i].con_handle = conn_handle;
                   
                   break;
               }
            }
            break;
            
        case CONSTATE_ACTIVE:
            // Look for an existing player and assign the state to
            for(int i = 0; i < PONG_NUM_PLAYERS; i++)
            {
               if(m_gamestate.player[i].connected_state == CONSTATE_CONNECTED &&
                  m_gamestate.player[i].con_handle == conn_handle)
               {
                   m_gamestate.player[i].connected_state = CONSTATE_ACTIVE;
                   
                   // Send a callback to update the color on the connected controller to the player color
                   m_pong_event.evt_type = PONG_EVENT_CON_SET_COLOR;
                   m_pong_event.game_state = &m_gamestate;
                   m_pong_event.params.con_set_color.color = m_gamestate.player[i].color;
                   m_pong_event.params.con_set_color.conn_handle = conn_handle;
                   m_event_callback(&m_pong_event);
                   break;
               }                
            }
            // Check if both players are active, and start the game if true
            if(m_gamestate.player[0].connected_state == CONSTATE_ACTIVE && 
               m_gamestate.player[1].connected_state == CONSTATE_ACTIVE)
            {
                set_main_state(STATE_GAME_RUNNING);
            }
            break;
    }
}

pong_gamestate_t *app_pong_get_gamestate(void)
{
    return &m_gamestate;
}

void app_pong_draw_display(void)
{
    uint32_t blink_fast = (state_lifetime >> 3) % 2;
    uint32_t paddle1_color, paddle2_color;
    
    switch(m_main_state)
    {
        case STATE_WAITING_FOR_PLAYERS:
            paddle1_color = (m_gamestate.player[0].connected_state == CONSTATE_DISCONNECTED) ? CL_RED : m_gamestate.player[0].color;
            if(m_gamestate.player[0].connected_state != CONSTATE_ACTIVE && blink_fast) paddle1_color = CL_BLACK;
            paddle2_color = (m_gamestate.player[1].connected_state == CONSTATE_DISCONNECTED) ? CL_RED : m_gamestate.player[1].color;
            if(m_gamestate.player[1].connected_state != CONSTATE_ACTIVE && blink_fast) paddle2_color = CL_BLACK;
            app_display_draw_paddles(16, 16, paddle1_color, paddle2_color);
            break;
            
        case STATE_GAME_RUNNING:
            app_display_draw_paddles(m_gamestate.player[0].paddle_pos_y * 32 / LEVEL_SIZE_Y, m_gamestate.player[1].paddle_pos_y * 32 / LEVEL_SIZE_Y,
                                     CL_BLUE, CL_GREEN);
            app_display_draw_ball(m_gamestate.pong_pos_x * 59 / LEVEL_SIZE_X, m_gamestate.pong_pos_y * 30 / LEVEL_SIZE_Y);
            break;
    }
    state_lifetime++;
}