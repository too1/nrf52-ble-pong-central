#include "app_pong.h"
#include "app_display.h"

APP_TIMER_DEF(m_game_loop_timer_id);

#define STATE_MASK_CLEAR_SCREEN (0xFFFFFFFF & ~(1 << STATE_GAME_START_PREDELAY))

static bool                         m_game_initialized = false;
static pong_callback_t              m_event_callback;

static pong_gamestate_t             m_gamestate;
static pong_global_control_state_t  m_global_control_state;

static pong_main_state_t            m_main_state = STATE_WAITING_FOR_PLAYERS; 
static uint32_t state_lifetime = 0;

static pong_event_t                 m_pong_event;
static bool                         m_graphics_invalidate = true;

static void reset_game(void);
static void reset_ball(void);

// "Private" functions
static void reset_player_button_pressed_state(void)
{
    for(int i = 0; i < PONG_NUM_PLAYERS; i++)
    {
        m_global_control_state.player[i].button_pressed = false;
    }
}

static void set_main_state(uint32_t new_state)
{
    reset_player_button_pressed_state();
    if((1 << new_state) & STATE_MASK_CLEAR_SCREEN)
    {
        app_display_clear_screen();
    }
    m_graphics_invalidate = true;
    switch(new_state)
    {
        case STATE_GAME_START_NEW_GAME:
            reset_game();
            break;
            
        case STATE_GAME_START_PREDELAY:
            app_display_draw_square(4, 0, 56, 32, CL_BLACK);
            app_display_score_state(&m_gamestate);
            break;
            
        case STATE_GAME_RUNNING:
            break;
            
        case STATE_GAME_SCORE_LIMIT_REACHED:
            app_display_draw_text("GAME OVER", 32, 2, CL_YELLOW, ALIGNMENT_CENTRE);
            app_display_score_state(&m_gamestate);
            break;
    }
    m_main_state = new_state;
    state_lifetime = 0;
}

static void reset_ball(void)
{
    uint32_t winning_player = 0xFFFF;
    m_gamestate.pong_pos_x = LEVEL_SIZE_X / 2;
    m_gamestate.pong_pos_y = LEVEL_SIZE_Y / 2;
    m_gamestate.pong_speed_x = 8;
    m_gamestate.pong_speed_y = 5;  
    m_gamestate.time_since_last_speed_increment = 0;
    m_gamestate.speed_multiplier_factor = 0;
    
    for(int i = 0; i < PONG_NUM_PLAYERS; i++)
    {
       if(m_gamestate.player[i].score >= PONG_SCORE_LIMIT)
       {
           winning_player = i;
       }
    }
    if(winning_player == 0xFFFF)
    {
       set_main_state(STATE_GAME_START_PREDELAY);
    }
    else 
    {
       set_main_state(STATE_GAME_SCORE_LIMIT_REACHED);
    }
}

static void reset_game(void)
{
    for(int i = 0; i < PONG_NUM_PLAYERS; i++)
    {
        m_gamestate.player[i].paddle_pos_y = LEVEL_SIZE_Y / 2;
        m_gamestate.player[i].score = 0;
    }    
    reset_ball();
}

static bool check_paddle_y_intersect(uint32_t y, uint32_t paddle_y)
{
    if(paddle_y < PADDLE_HALFSIZE_Y)
    {
        return (y < (paddle_y + PADDLE_HALFSIZE_Y));
    }
    else if(paddle_y > (LEVEL_SIZE_Y - PADDLE_HALFSIZE_Y))
    {
        return (y > (paddle_y - PADDLE_HALFSIZE_Y));
    }
    else
    {
        return (y > (paddle_y - PADDLE_HALFSIZE_Y) && y < (paddle_y + PADDLE_HALFSIZE_Y));
    }
}

static int32_t scale_speed(int32_t speed)
{
    return speed * (4 + (int32_t)m_gamestate.speed_multiplier_factor) / 4;
}

static void update_game_running(void)
{
    // Update pong X position
    m_gamestate.pong_pos_x += scale_speed(m_gamestate.pong_speed_x);
    
    // Check if the ball has reached one of the board edges
    if(m_gamestate.pong_pos_x > LEVEL_SIZE_X)
    {
        // Right edge hit. Check if paddle is in position
        if(check_paddle_y_intersect(m_gamestate.pong_pos_y, m_gamestate.player[1].paddle_pos_y))
        {
            m_gamestate.pong_pos_x = LEVEL_SIZE_X;
            m_gamestate.pong_speed_x *= -1;
        }
        else
        {
            // The ball missed. Increment score and reset ball
            m_gamestate.player[0].score++;
            reset_ball();
        }
    }
    else if(m_gamestate.pong_pos_x < 0)
    {
        // Left edge hit. Check if the paddle is in position
        if(check_paddle_y_intersect(m_gamestate.pong_pos_y, m_gamestate.player[0].paddle_pos_y))
        {
            m_gamestate.pong_pos_x = 0;
            m_gamestate.pong_speed_x *= -1;
        }
        else
        {
            // The ball missed. Increment score and reset ball
            m_gamestate.player[1].score++;
            reset_ball();
        }     
    }

    // Update pong Y position
    m_gamestate.pong_pos_y += scale_speed(m_gamestate.pong_speed_y);
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
  
    // Update speed multiplier
    m_gamestate.time_since_last_speed_increment++;
    if(m_gamestate.time_since_last_speed_increment > (PONG_SPEED_INC_INTERVAL * GAME_LOOP_UPDATE_FREQ))
    {
        m_gamestate.time_since_last_speed_increment = 0;
        m_gamestate.speed_multiplier_factor++;
    }
}

static void update_game_loop(void *p)
{
    // Update paddle positions based on game input, independent of state
    if(m_main_state != STATE_GAME_START_PREDELAY || state_lifetime > (GAME_LOOP_UPDATE_FREQ))
        for(int i = 0; i < PONG_NUM_PLAYERS; i++)
        {
            m_gamestate.player[i].paddle_pos_y = m_global_control_state.player[i].paddle_x;
        }
    
    switch(m_main_state)
    {
        case STATE_WAITING_FOR_PLAYERS:
            break;
            
        case STATE_GAME_START_NEW_GAME:
            set_main_state(STATE_GAME_START_PREDELAY);
            break;
            
        case STATE_GAME_START_PREDELAY:
            if(state_lifetime > (PONG_PREDELAY_TIME_S * GAME_LOOP_UPDATE_FREQ))
            {
                set_main_state(STATE_GAME_RUNNING);
            }
            break;
            
        case STATE_GAME_RUNNING:
            update_game_running();
            break;
            
        case STATE_GAME_SCORE_LIMIT_REACHED:
            // If both players press the button, start a new game
            if(m_global_control_state.player[0].button_pressed && m_global_control_state.player[1].button_pressed)
            {
                set_main_state(STATE_GAME_START_NEW_GAME);
            }
            break;
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
    m_gamestate.player[0].connected_state = CONSTATE_DISCONNECTED;
    m_gamestate.player[1].connected_state = CONSTATE_DISCONNECTED;
    m_gamestate.player[0].color = CL_TEAL;
    m_gamestate.player[1].color = CL_BLUE;
    m_main_state = STATE_WAITING_FOR_PLAYERS;
    return 0;
}

uint32_t app_pong_start_game(void)
{
    if(!m_game_initialized) return -1;
    app_timer_start(m_game_loop_timer_id, APP_TIMER_TICKS(GAME_LOOP_UPDATE_MS), 0);
    return 0;
}

pong_controller_state_t *app_pong_get_controller(uint32_t index)
{
    return &m_global_control_state.player[index];
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
            if(m_main_state != STATE_WAITING_FOR_PLAYERS)
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
                set_main_state(STATE_GAME_START_NEW_GAME);
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
            app_display_draw_paddles(16, 16, paddle1_color, paddle2_color, m_graphics_invalidate);
            app_display_draw_text("Waiting", 32, 2, CL_BLUE, ALIGNMENT_CENTRE);
            app_display_draw_text("for", 32, 11, CL_BLUE, ALIGNMENT_CENTRE);
            app_display_draw_text("players", 32, 20, CL_BLUE, ALIGNMENT_CENTRE);
            break;
            
        case STATE_GAME_START_PREDELAY:
            app_display_draw_text("Get ready", 32, 2, CL_WHITE, ALIGNMENT_CENTRE);
            app_display_draw_square(20, 12, 20, 20, CL_BLACK);
            app_display_draw_int(PONG_PREDELAY_TIME_S - (state_lifetime / GAME_LOOP_UPDATE_FREQ), 32, 14, CL_WHITE, ALIGNMENT_CENTRE);
            app_display_draw_paddles(m_gamestate.player[0].paddle_pos_y * 32 / LEVEL_SIZE_Y, m_gamestate.player[1].paddle_pos_y * 32 / LEVEL_SIZE_Y,
                                     m_gamestate.player[0].color, m_gamestate.player[1].color, m_graphics_invalidate);
            break;
            
        case STATE_GAME_RUNNING:
            app_display_draw_paddles(m_gamestate.player[0].paddle_pos_y * 32 / LEVEL_SIZE_Y, m_gamestate.player[1].paddle_pos_y * 32 / LEVEL_SIZE_Y,
                                     m_gamestate.player[0].color, m_gamestate.player[1].color, m_graphics_invalidate);
            app_display_score_state(&m_gamestate);
            app_display_draw_ball(m_gamestate.pong_pos_x * 59 / LEVEL_SIZE_X, m_gamestate.pong_pos_y * 30 / LEVEL_SIZE_Y);
            break;
            
        case STATE_GAME_SCORE_LIMIT_REACHED:
            break;
    }
    state_lifetime++;
    m_graphics_invalidate = false;
}