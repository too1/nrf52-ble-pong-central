#ifndef __APP_BLE_PERIPHERAL_MANAGER_H
#define __APP_BLE_PERIPHERAL_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"

typedef struct
{


}ble_per_manager_event_t;

typedef void (*ble_per_manager_callback_t)(ble_per_manager_event_t *event);

typedef struct
{
    ble_per_manager_callback_t callback;
}ble_per_manager_config_t;

void ble_per_manager_init(ble_per_manager_config_t *config);

void ble_per_manager_start_advertising(void);

void ble_per_manager_on_ble_evt(ble_evt_t const * p_ble_evt);

#endif
