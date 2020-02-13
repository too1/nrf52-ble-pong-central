#include "state_manager.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static state_ops_return_t state_return_val;

static void run_function_by_state(state_mngr_t *instance, state_ops_return_t *ret_val)
{
    if(instance->current_state != STATE_MNGR_INVALID_STATE && instance->current_state <= instance->state_ops_map_size)
    {
        instance->state_ops_map[instance->current_state - 1].op_func(instance, ret_val);
    }
}

void state_mngr_init(state_mngr_t *state_mngr_instance, state_ops_map_t *ops_map, uint32_t ops_map_length)
{
    state_mngr_instance->state_ops_map = ops_map;
    state_mngr_instance->state_ops_map_size = ops_map_length;
    state_mngr_instance->current_state = STATE_MNGR_INVALID_STATE;
    state_mngr_instance->previous_state = STATE_MNGR_INVALID_STATE;
}

void state_mngr_run_state(state_mngr_t *instance, state_t state)
{
    while(1)
    {
        NRF_LOG_INFO("Leave state: %i", (int)instance->current_state);
        instance->current_op = STATE_OP_EXIT;
        state_return_val.go_to_state = STATE_MNGR_INVALID_STATE;
        run_function_by_state(instance, &state_return_val);

        instance->previous_state = instance->current_state;
        instance->current_state = state;
        instance->lifetime = 0;

        NRF_LOG_INFO("Enter state: %i", (int)instance->current_state);
        instance->current_op = STATE_OP_ENTER;
        state_return_val.go_to_state = STATE_MNGR_INVALID_STATE;
        run_function_by_state(instance, &state_return_val);
        
        if(state_return_val.go_to_state == STATE_MNGR_INVALID_STATE) break;

        NRF_LOG_INFO("GOTO state from enter");
        instance->previous_state = instance->current_state;
        instance->current_state = state_return_val.go_to_state;
    }
}

void state_mngr_on_update(state_mngr_t * instance)
{
    //NRF_LOG_INFO("Update state: %i", (int)instance->current_state);
    instance->current_op = STATE_OP_UPDATE;
    state_return_val.go_to_state = STATE_MNGR_INVALID_STATE;
    run_function_by_state(instance, &state_return_val);

    if(state_return_val.go_to_state != STATE_MNGR_INVALID_STATE)
    {
        NRF_LOG_INFO("GOTO state from update");
        state_mngr_run_state(instance, state_return_val.go_to_state);
    }
}

void state_mngr_on_draw(state_mngr_t * instance)
{
    //NRF_LOG_INFO("Draw state: %i", (int)instance->current_state);
    instance->current_op = STATE_OP_DRAW;
    state_return_val.go_to_state = STATE_MNGR_INVALID_STATE;
    run_function_by_state(instance, &state_return_val);
    instance->lifetime++;
}