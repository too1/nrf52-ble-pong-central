#include "app_pong.h"
#include "app_display.h"
#include "math.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

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
static void reset_ball(uint32_t player_score_index);

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

        case STATE_GAME_TOURNAMENT_ROUND_STARTING:
            reset_game();
            app_display_draw_text("Get ready...", 28, 1, CL_BLUE, ALIGNMENT_CENTRE);
            app_display_draw_text(m_gamestate.player[0].name, 1, 11, m_gamestate.player[0].color, ALIGNMENT_LEFT);
            app_display_draw_text("vs", 63, 12, CL_WHITE, ALIGNMENT_RIGHT);
            app_display_draw_text(m_gamestate.player[1].name, 52,21, m_gamestate.player[1].color, ALIGNMENT_RIGHT);
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

static uint8_t get_random_number(void)
{
    uint8_t ret_val;
    sd_rand_application_vector_get(&ret_val, 1);
    return ret_val;
}

static void on_set_thingy_color(uint32_t thingy_index, uint32_t color)
{
    if(m_gamestate.player[thingy_index].connected_state == CONSTATE_ACTIVE)
    {
        m_pong_event.evt_type = PONG_EVENT_CON_SET_COLOR;
        m_pong_event.game_state = &m_gamestate;
        m_pong_event.params.con_set_color.color = color;
        m_pong_event.params.con_set_color.conn_handle = m_gamestate.player[thingy_index].con_handle;
        m_event_callback(&m_pong_event);
    }
}

static void play_sound(uint32_t con_index, pong_sound_sample_t sample_id)
{
    // Send event back to the application to play the sound
    m_pong_event.evt_type = PONG_EVENT_PLAY_SOUND;
    m_pong_event.game_state = &m_gamestate;
    m_pong_event.params.play_sound.controller_index = con_index;
    m_pong_event.params.play_sound.sample_id = sample_id;
    m_event_callback(&m_pong_event);
}

static void on_game_started()
{
    m_pong_event.evt_type = PONG_EVENT_GAME_STARTED;
    m_event_callback(&m_pong_event);
}

static void on_point_scored(uint8_t player_id)
{
    m_pong_event.evt_type = PONG_EVENT_POINT_SCORED;
    m_pong_event.params.point_scored.player_index = player_id;
    m_event_callback(&m_pong_event);
}

static void on_game_over(uint8_t player_id)
{
    m_pong_event.evt_type = PONG_EVENT_GAME_OVER;
    m_pong_event.params.game_over.winner_index = player_id;
    m_event_callback(&m_pong_event);
}

static void reset_ball(uint32_t player_scored_index)
{
    uint32_t winning_player = 0xFFFF;
    m_gamestate.pong_pos_x = LEVEL_SIZE_X / 2;
    m_gamestate.pong_pos_y = LEVEL_SIZE_Y / 2;
    m_gamestate.time_since_last_speed_increment = 0;
    m_gamestate.ball_pos_x = LEVEL_SIZE_X / 2;
    m_gamestate.ball_pos_y = LEVEL_SIZE_Y / 2;
    m_gamestate.ball_speed = PONG_BALL_START_SPEED;
    m_gamestate.ball_rotation = 0.0f;
    
    uint8_t rand = get_random_number();
    switch(player_scored_index)
    {
        case 0:
            m_gamestate.ball_direction = ((float)rand * PI / 2.0f / 255.0f) - (PI * 0.25f);
            break;
        case 1:
            m_gamestate.ball_direction = ((float)rand * PI / 2.0f / 255.0f) + (PI * 0.75f);
            break;  
        default:
            rand = (rand & ~0x20) | ((rand & 0x40) ? 0x20 : 0x00);
            m_gamestate.ball_direction = (float)rand * 2 * PI / 255.0f;
            break;
    }

    // Reduce the speed increase by one step, but make sure it doesn't go below 1
    m_gamestate.ball_speed_inc_factor /= PONG_BALL_SPEED_INC_AMOUNT;
    if(m_gamestate.ball_speed_inc_factor < 1.0f)
    {
        m_gamestate.ball_speed_inc_factor = 1.0f;
    }

    for(int i = 0; i < PONG_NUM_PLAYERS; i++)
    {
       if(m_gamestate.player[i].score >= PONG_SCORE_LIMIT)
       {
           winning_player = i;
           on_game_over((uint8_t)winning_player);
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
    m_gamestate.ball_speed_inc_factor = 1.0f;
    reset_ball(0xFFFFFFFF);
}

static bool check_paddle_y_intersect(uint32_t y, uint32_t paddle_y)
{
    bool return_val;
    uint32_t paddle_upper = paddle_y + PADDLE_HALFSIZE_Y;
    uint32_t paddle_lower = (paddle_y > PADDLE_HALFSIZE_Y) ? paddle_y - PADDLE_HALFSIZE_Y : 0;
    if(paddle_y < PADDLE_HALFSIZE_Y)
    {
        return_val = (y < paddle_upper);
    }
    else if(paddle_y > (LEVEL_SIZE_Y - PADDLE_HALFSIZE_Y))
    {
        return_val = (y > paddle_lower);
    }
    else
    {
        return_val = (y > paddle_lower && y < paddle_upper);
    }
    NRF_LOG_INFO("%s Ball y: %i, paddle y: %i (%i-%i)", return_val ? "Hit!" : "Miss", y, paddle_y, paddle_lower, paddle_upper);
    return return_val;
}

static void update_game_running(void)
{
    // Update pong position
    m_gamestate.ball_direction += (m_gamestate.ball_rotation * 0.01f / m_gamestate.ball_speed_inc_factor);
    m_gamestate.ball_rotation *= PONG_BALL_ROTATION_DECAY_FACTOR;
    m_gamestate.ball_pos_x += cosf(m_gamestate.ball_direction) * m_gamestate.ball_speed * m_gamestate.ball_speed_inc_factor;
    m_gamestate.ball_pos_y += sinf(m_gamestate.ball_direction) * m_gamestate.ball_speed * m_gamestate.ball_speed_inc_factor;
    m_gamestate.pong_pos_x = (int32_t)m_gamestate.ball_pos_x;
    m_gamestate.pong_pos_y = (int32_t)m_gamestate.ball_pos_y;
    
    // Check if the ball has reached one of the board edges
    if(m_gamestate.pong_pos_x > LEVEL_SIZE_X)
    {
        // Right edge hit. Check if paddle is in position
        if(check_paddle_y_intersect(m_gamestate.pong_pos_y, m_gamestate.player[1].paddle_pos_y))
        {
            m_gamestate.pong_pos_x = LEVEL_SIZE_X;
            m_gamestate.ball_direction = PI - m_gamestate.ball_direction;
            m_gamestate.ball_rotation += (float)m_gamestate.player[1].paddle_pos_y_delta * 0.05f;
            NRF_LOG_INFO("New ball rotation: %i", (int)(m_gamestate.ball_rotation * 1000.0f));
            play_sound(1, SOUND_SAMPLE_HIT);
        }
        else
        {
            // The ball missed. Increment score and reset ball
            play_sound(0, SOUND_SAMPLE_COLLECT_POINT_B);
            play_sound(1, SOUND_SAMPLE_EXPLOSION_B);
            on_point_scored(0);
            m_gamestate.player[0].score++;
            reset_ball(0);
        }
    }
    else if(m_gamestate.pong_pos_x < 0)
    {
        // Left edge hit. Check if the paddle is in position
        if(check_paddle_y_intersect(m_gamestate.pong_pos_y, m_gamestate.player[0].paddle_pos_y))
        {
            m_gamestate.pong_pos_x = 0;
            m_gamestate.ball_direction = PI - m_gamestate.ball_direction;
            m_gamestate.ball_rotation -= (float)m_gamestate.player[0].paddle_pos_y_delta * 0.05f;
            NRF_LOG_INFO("New ball rotation: %i", (int)(m_gamestate.ball_rotation * 1000.0f));
            play_sound(0, SOUND_SAMPLE_HIT);
        }
        else
        {
            // The ball missed. Increment score and reset ball
            play_sound(0, SOUND_SAMPLE_EXPLOSION_B);
            play_sound(1, SOUND_SAMPLE_COLLECT_POINT_B);
            on_point_scored(1);
            m_gamestate.player[1].score++;
            reset_ball(1);
        }     
    }

    if(m_gamestate.pong_pos_y > LEVEL_SIZE_Y)
    {
        m_gamestate.pong_pos_y = LEVEL_SIZE_Y;
        m_gamestate.ball_direction = 2.0f * PI - m_gamestate.ball_direction;

    }
    else if(m_gamestate.pong_pos_y < 0)
    {
        m_gamestate.pong_pos_y = 0;
        m_gamestate.ball_direction = 2.0f * PI - m_gamestate.ball_direction;
    } 
    
    // Update speed multiplier
    m_gamestate.time_since_last_speed_increment++;
    if(m_gamestate.time_since_last_speed_increment > (PONG_SPEED_INC_INTERVAL * GAME_LOOP_UPDATE_FREQ))
    {
        m_gamestate.time_since_last_speed_increment = 0;
        m_gamestate.ball_speed_inc_factor *= PONG_BALL_SPEED_INC_AMOUNT;
    }
}

static void update_game_loop(void *p)
{
    // Update paddle positions based on game input, independent of state
    if(m_main_state != STATE_GAME_START_PREDELAY || state_lifetime > (GAME_LOOP_UPDATE_FREQ))
        for(int i = 0; i < PONG_NUM_PLAYERS; i++)
        {
            m_gamestate.player[i].paddle_pos_y = m_global_control_state.player[i].paddle_x;
            m_gamestate.player[i].paddle_pos_y_delta = m_global_control_state.player[i].paddle_x_delta;
        }
    
    switch(m_main_state)
    {
        case STATE_WAITING_FOR_PLAYERS:
            break;

        case STATE_GAME_TOURNAMENT_ROUND_STARTING:
            if(m_global_control_state.player[0].button_pressed && 
               m_global_control_state.player[1].button_pressed)
            {
                set_main_state(STATE_GAME_START_PREDELAY);
            }
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
    m_gamestate.player[0].color = COLOR_RGB(255, 0, 127);
    m_gamestate.player[1].color = COLOR_RGB(127, 255, 0);
    m_main_state = STATE_WAITING_FOR_PLAYERS;
    return 0;
}

uint32_t app_pong_start_game(void)
{
    if(!m_game_initialized) return -1;
    app_timer_start(m_game_loop_timer_id, APP_TIMER_TICKS(GAME_LOOP_UPDATE_MS), 0);
    return 0;
}

uint32_t app_pong_start_tournament_round(uint8_t *init_buffer, uint32_t init_buffer_length)
{
    static uint8_t player1_name_buf[17], player2_name_buf[17];
    int i;
    if(!m_game_initialized) return -1;
    if(!(m_main_state == STATE_WAITING_FOR_PLAYERS || m_main_state == STATE_GAME_SCORE_LIMIT_REACHED)) return -1;
    m_gamestate.player[0].color = (uint32_t)*init_buffer++ << 16 | 
                                  (uint32_t)*init_buffer++ << 8 | 
                                  (uint32_t)*init_buffer++;
    for(i = 0; i < 15; i++)
    {
        player1_name_buf[i] = *init_buffer;
        if(*init_buffer++ == 0) break;
    }
    player1_name_buf[i + 1] = 0;
    m_gamestate.player[0].name = player1_name_buf;
    on_set_thingy_color(0, m_gamestate.player[0].color);

    m_gamestate.player[1].color = (uint32_t)*init_buffer++ << 16 | 
                                  (uint32_t)*init_buffer++ << 8 | 
                                  (uint32_t)*init_buffer++;
    for(i = 0; i < 15; i++)
    {
        player2_name_buf[i] = *init_buffer;
        if(*init_buffer++ == 0) break;
    }
    player2_name_buf[i + 1] = 0;
    m_gamestate.player[1].name = player2_name_buf;
    on_set_thingy_color(1, m_gamestate.player[1].color);
    on_game_started();
    set_main_state(STATE_GAME_TOURNAMENT_ROUND_STARTING);
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
                   on_set_thingy_color(i, m_gamestate.player[i].color);
                   break;
               }                
            }
            // Check if both players are active, and start the game if true
            /*if(m_gamestate.player[0].connected_state == CONSTATE_ACTIVE && 
               m_gamestate.player[1].connected_state == CONSTATE_ACTIVE)
            {
                set_main_state(STATE_GAME_START_NEW_GAME);
            }*/
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

        case STATE_GAME_TOURNAMENT_ROUND_STARTING:
            app_display_draw_square(0, 12, 56, 24, CL_BLACK);
            if(m_gamestate.player[0].connected_state == CONSTATE_ACTIVE || blink_fast) app_display_draw_text(m_gamestate.player[0].name, 1, 11, m_gamestate.player[0].color, ALIGNMENT_LEFT);
            app_display_draw_text("vs", 63, 12, CL_WHITE, ALIGNMENT_RIGHT);
            if(m_gamestate.player[1].connected_state == CONSTATE_ACTIVE || blink_fast) app_display_draw_text(m_gamestate.player[1].name, 52,21, m_gamestate.player[1].color, ALIGNMENT_RIGHT);
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
            app_display_draw_ball(m_gamestate.pong_pos_x * 58 / LEVEL_SIZE_X, m_gamestate.pong_pos_y * 30 / LEVEL_SIZE_Y);
            break;
            
        case STATE_GAME_SCORE_LIMIT_REACHED:
            break;
    }
    state_lifetime++;
    m_graphics_invalidate = false;
}