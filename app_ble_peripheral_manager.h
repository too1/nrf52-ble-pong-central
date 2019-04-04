#ifndef __APP_BLE_PERIPHERAL_MANAGER_H
#define __APP_BLE_PERIPHERAL_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"

typedef enum {BLE_PER_MNG_EVT_CONNECTED, 
              BLE_PER_MNG_EVT_DISCONNECTED, 
              BLE_PER_MNG_EVT_DATA_RECEIVED} ble_per_manager_event_type_t;

typedef enum {BLE_PER_MNG_TX_CMD_POINT_SCORED = 1, 
              BLE_PER_MNG_TX_CMD_GAME_OVER, 
              BLE_PER_MNG_TX_CMD_GAME_STARTED,
              } ble_per_manager_tx_command_t;

typedef struct
{
    ble_per_manager_event_type_t evt_type;
    uint8_t *data_ptr;
    uint32_t data_length;
}ble_per_manager_event_t;

typedef void (*ble_per_manager_callback_t)(ble_per_manager_event_t *event);

typedef struct
{
    ble_per_manager_callback_t callback;
}ble_per_manager_config_t;

void ble_per_manager_init(ble_per_manager_config_t *config);

void ble_per_manager_start_advertising(void);

void ble_per_manager_on_ble_evt(ble_evt_t const * p_ble_evt);

void ble_per_manager_on_game_started(uint8_t game_handle);

void ble_per_manager_on_point_scored(uint8_t player_index);

void ble_per_manager_on_game_over(uint8_t winner_index);

#endif
