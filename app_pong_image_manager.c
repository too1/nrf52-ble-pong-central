#include "app_pong_image_manager.h"
#include "nordic_common.h"
#include "nrf_fstorage.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"
#include "nrf_error.h"
#include "app_error.h"

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);


NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = PONG_IMAGE_FLASH_AREA_BEGIN,
    .end_addr   = PONG_IMAGE_FLASH_AREA_END,
};

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        //NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            //NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            //NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.", p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}

void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        
    }
}

static uint32_t find_free_record(void)
{
    uint32_t *flash_ptr;
    for(uint32_t address = PONG_IMAGE_FLASH_AREA_BEGIN; address < PONG_IMAGE_FLASH_AREA_END; address += PONG_IMAGE_RECORD_SIZE)
    {
        flash_ptr = (uint32_t*)address;
        if(*flash_ptr == 0xFFFFFFFF) return address;
    }
    return 0;
}

uint32_t app_pong_image_init(void)
{
    nrf_fstorage_api_t * p_fs_api;
    p_fs_api = &nrf_fstorage_sd;

    ret_code_t rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);
}

uint32_t app_pong_image_find_by_name(pong_image_info_t *info, const char* name)
{

}

uint32_t app_pong_image_find_by_type(pong_image_info_t *info, uint32_t type)
{

}

uint32_t app_pong_image_store(pong_image_info_t *info)
{
    uint32_t magic_number = PONG_IMAGE_RECORD_START_MAGIC_NUMBER;

    uint32_t free_record_start_address = find_free_record();
    if(free_record_start_address != 0)
    {
        
        ret_code_t rc = nrf_fstorage_write(&fstorage, free_record_start_address, &magic_number, sizeof(magic_number), NULL);
        APP_ERROR_CHECK(rc);
        free_record_start_address += 4;
        wait_for_flash_ready(&fstorage);
        rc = nrf_fstorage_write(&fstorage, free_record_start_address, info, sizeof(pong_image_info_t), NULL);   
        APP_ERROR_CHECK(rc);
        free_record_start_address += sizeof(pong_image_info_t);
        rc = nrf_fstorage_write(&fstorage, free_record_start_address, info->data_ptr, info->width * info->height, NULL); 
        APP_ERROR_CHECK(rc);
    }
}