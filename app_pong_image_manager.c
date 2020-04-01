#include <string.h>
#include <stddef.h>
#include "app_pong_image_manager.h"
#include "nordic_common.h"
#include "nrf_fstorage.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"
#include "nrf_error.h"
#include "app_error.h"

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

static uint32_t             m_current_flash_operation_state = 0;
static uint32_t             m_current_flash_operation_address = 0;
static pong_image_info_t    m_current_image_for_write;
static uint8_t*             m_current_image_for_write_dataptr;

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

static void image_store_operation_splitter()
{
    ret_code_t rc;
    static uint32_t magic_number = PONG_IMAGE_RECORD_START_MAGIC_NUMBER;
    
    switch(m_current_flash_operation_state)
    {
        case 0:
            break;
            
        case 1:
            rc = nrf_fstorage_write(&fstorage, m_current_flash_operation_address, 
                                    &magic_number, sizeof(magic_number), NULL);
            APP_ERROR_CHECK(rc);
            m_current_flash_operation_state++;
            m_current_flash_operation_address += sizeof(magic_number);
            break;
            
        case 2:
            rc = nrf_fstorage_write(&fstorage, m_current_flash_operation_address, 
                                    &m_current_image_for_write, sizeof(pong_image_info_t), NULL);   
            APP_ERROR_CHECK(rc);
            m_current_flash_operation_state++;
            m_current_flash_operation_address += sizeof(pong_image_info_t);
            break;
            
        case 3:
            rc = nrf_fstorage_write(&fstorage, m_current_flash_operation_address, 
                                    m_current_image_for_write_dataptr, 
                                    m_current_image_for_write.width * m_current_image_for_write.height, NULL); 
            APP_ERROR_CHECK(rc);
            m_current_flash_operation_state = 0;
            break;
            
        default:
            break;
   }
}

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
            image_store_operation_splitter();
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

uint32_t app_pong_image_find_by_name(pong_image_info_t **info, const char* name)
{

}

uint32_t app_pong_image_find_by_type(pong_image_info_t **info, uint32_t type)
{
    uint32_t *flash_ptr;
    static pong_image_info_t found_image;
    for(int address = PONG_IMAGE_FLASH_AREA_BEGIN; address < PONG_IMAGE_FLASH_AREA_END; address += PONG_IMAGE_RECORD_SIZE)
    {
        flash_ptr = (uint32_t*)address;
        if(*flash_ptr == PONG_IMAGE_RECORD_START_MAGIC_NUMBER)
        {
            if(((pong_image_info_t*)(address + 4))->img_type == type)
            {
                *info = (pong_image_info_t*)(address + 4);
                return 0;
            }
        }
    } 
    *info = NULL;
    return -1;
}

uint32_t app_pong_image_find_by_index(pong_image_info_t **info, uint32_t index)
{
    uint32_t *flash_ptr;
    if(index < ((PONG_IMAGE_FLASH_AREA_END - PONG_IMAGE_FLASH_AREA_BEGIN) / PONG_IMAGE_RECORD_SIZE))
    {
        flash_ptr = (uint32_t*)(PONG_IMAGE_FLASH_AREA_BEGIN + index * PONG_IMAGE_RECORD_SIZE);
        if(*flash_ptr == PONG_IMAGE_RECORD_START_MAGIC_NUMBER)
        {     
            *info = (pong_image_info_t*)(flash_ptr + 1);
            return 0;
        }  
    }
    *info = NULL;
    return -1;
}

uint32_t app_pong_image_store(pong_image_info_t *info)
{
    uint32_t free_record_start_address = find_free_record();
    if(free_record_start_address != 0 && m_current_flash_operation_state == 0)
    {
        memcpy((void*)&m_current_image_for_write, info, sizeof(pong_image_info_t));
        m_current_image_for_write_dataptr = info->data_ptr;
        
        m_current_flash_operation_state = 1;
        m_current_flash_operation_address = free_record_start_address;
        m_current_image_for_write.data_ptr = (uint8_t*)(free_record_start_address + 4 + offsetof(pong_image_info_t, data_ptr) + 4);
        image_store_operation_splitter();
        return 0;
    }
    return -1;    
}