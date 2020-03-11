#ifndef __PONG_IMAGE_MANAGER_H
#define __PONG_IMAGE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define PONG_IMAGE_RECORD_START_MAGIC_NUMBER 0x23874598
#define PONG_IMAGE_FLASH_AREA_BEGIN          0x70000
#define PONG_IMAGE_FLASH_AREA_END            0x7FFFF
#define PONG_IMAGE_RECORD_SIZE               4096
#define PONG_IMAGE_FLASH_PAGE_SIZE           4096

typedef struct
{
    char img_name[32];
    uint32_t img_type;
    uint16_t width;
    uint16_t height;
    uint8_t *data_ptr;
} pong_image_info_t;

uint32_t app_pong_image_init(void);

uint32_t app_pong_image_find_by_name(pong_image_info_t *info, const char* name);

uint32_t app_pong_image_find_by_type(pong_image_info_t *info, uint32_t type);

uint32_t app_pong_image_store(pong_image_info_t *info);

#endif
