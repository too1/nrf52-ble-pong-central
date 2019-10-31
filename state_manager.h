#ifndef __STATE_MANAGER_H
#define __STATE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

struct state_mngr_t;
#define STATE_MNGR_INVALID_STATE 0

typedef enum {STATE_OP_ENTER, STATE_OP_UPDATE, STATE_OP_DRAW, STATE_OP_EXIT} state_ops_t;

typedef uint32_t state_t;

typedef struct
{
    state_t     go_to_state;
} state_ops_return_t;

typedef void (*state_op_function_t)(struct state_mngr_t * context, state_ops_return_t * return_data);

typedef struct
{
    state_t             state;
    state_op_function_t op_func;
} state_ops_map_t;

typedef struct
{
    state_ops_t         current_op;
    state_t             current_state;
    state_t             previous_state;
    state_ops_map_t    *state_ops_map;
    uint32_t            state_ops_map_size;
    uint32_t            lifetime;
} state_mngr_t;


void state_mngr_init(state_mngr_t *state_mngr_instance, state_ops_map_t *ops_map, uint32_t ops_map_length);

void state_mngr_run_state(state_mngr_t *instance, state_t state);

void state_mngr_on_update(state_mngr_t *state_mngr_instance);

void state_mngr_on_draw(state_mngr_t *state_mngr_instance);

#endif
