

#include <string.h>
#include "nrf_error.h"
#include "drv_led_matrix.h"
#include "nrfx_spim.h"
#include "sdk_macros.h"
#include "nrf_delay.h"

static const nrfx_spim_t spi_red_instance      = NRFX_SPIM_INSTANCE(0);  /**< SPI instance. */
static const nrfx_spim_t spi_green_instance    = NRFX_SPIM_INSTANCE(1);  /**< SPI instance. */
static const nrfx_spim_t spi_blue_instance     = NRFX_SPIM_INSTANCE(2);  /**< SPI instance. */

NRF_SPIM_Type * spim_red = NRF_SPIM0;
NRF_SPIM_Type * spim_green = NRF_SPIM1;
NRF_SPIM_Type * spim_blue = NRF_SPIM2;

            
typedef enum 
{
    ACTIVE_SCREEN_HALF_TOP,
    ACTIVE_SCREEN_HALF_BOTTOM
}active_screen_half_state_t;

static led_matrix_buffer_t mbuff[MATRIX_MULTI_DRAW];

static void drv_led_start_spi_transfer()
{   
    spim_blue->TASKS_START = 1;
    spim_red->TASKS_START = 1;
    spim_green->TASKS_START = 1;
}


static void drv_led_update_latch_pins(void)
{
    static uint8_t latch_pin_mask = 15;
    uint32_t gpio_port = NRF_GPIO->OUT;
    
    gpio_port = gpio_port & ~(0xF << 13); // Clear bits 
    gpio_port = gpio_port | (latch_pin_mask << 13); // Set bits
    NRF_GPIO->OUT = gpio_port;
    
    latch_pin_mask = (latch_pin_mask + 1) & 0x0F;
}


static void drv_led_swap_spi_pins(active_screen_half_state_t active_screen_half)
{
    spim_red->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
    spim_green->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
    spim_blue->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
    
    if(active_screen_half == ACTIVE_SCREEN_HALF_BOTTOM)
    {
        spim_red->PSEL.MOSI = PIN_R_1;
        spim_green->PSEL.MOSI = PIN_G_1;
        spim_blue->PSEL.MOSI = PIN_B_1;
    }
    else
    {
        spim_red->PSEL.MOSI = PIN_R_2;
        spim_green->PSEL.MOSI = PIN_G_2;
        spim_blue->PSEL.MOSI = PIN_B_2;
    }
    
    spim_red->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
    spim_green->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
    spim_blue->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
}


/**
 * @brief SPI user event handler.
 * @param event
 */
static void drv_led_spi_event_handler_red(nrfx_spim_evt_t const * p_event, void * p_context){}
static void drv_led_spi_event_handler_green(nrfx_spim_evt_t const * p_event, void * p_context){}
static void drv_led_spi_event_handler_blue(nrfx_spim_evt_t const * p_event, void * p_context){}   

/*
 * This timer is used to start an SPI transfer after every latch address update
 */
static uint32_t drv_led_reset_spi_transfer_config()
{   
    static uint32_t multi_draw_index = 0;
    ret_code_t err_code;
    // SPI EasyDMA transfer flags
    uint8_t xfer_flags = NRFX_SPIM_FLAG_REPEATED_XFER | NRFX_SPIM_FLAG_HOLD_XFER | NRFX_SPIM_FLAG_TX_POSTINC;
    
    // Initiate Red buffer
    nrfx_spim_xfer_desc_t xfer_red = NRFX_SPIM_XFER_TRX(&mbuff[multi_draw_index].r[0], MATRIX_BUFFER_WIDTH, NULL, 0);
    err_code = nrfx_spim_xfer(&spi_red_instance, &xfer_red, xfer_flags);
    VERIFY_SUCCESS(err_code);
   
    // Initiate Green buffer
    nrfx_spim_xfer_desc_t xfer_green = NRFX_SPIM_XFER_TRX(&mbuff[multi_draw_index].g[0], MATRIX_BUFFER_WIDTH, NULL, 0);
    err_code = nrfx_spim_xfer(&spi_green_instance, &xfer_green, xfer_flags);
    VERIFY_SUCCESS(err_code);

    // Initiate Blue buffer
    nrfx_spim_xfer_desc_t xfer_blue = NRFX_SPIM_XFER_TRX(&mbuff[multi_draw_index].b[0], MATRIX_BUFFER_WIDTH, NULL, 0);
    err_code = nrfx_spim_xfer(&spi_blue_instance, &xfer_blue, xfer_flags);
    VERIFY_SUCCESS(err_code);

    multi_draw_index = (multi_draw_index + 1) % MATRIX_MULTI_DRAW;
    return NRF_SUCCESS;
}


/**
 * Initiate 3 SPI modules with MOSI pins corresponding to R, G, and B pins
 */
static uint32_t drv_led_setup_spi()
{
    uint32_t err_code;
    // Default SPI settings
    nrfx_spim_config_t config;
    config.miso_pin     = NRFX_SPIM_PIN_NOT_USED;
    config.ss_pin       = NRFX_SPIM_PIN_NOT_USED;
    config.irq_priority = NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY;
    config.orc          = 0xFF;
    config.frequency    = NRF_SPIM_FREQ_1M;
    config.mode         = NRF_SPIM_MODE_0;
    config.bit_order    = NRF_SPIM_BIT_ORDER_LSB_FIRST;

    // Init SPIM for red channel
    config.mosi_pin     = PIN_R_1;
    config.sck_pin      = NRFX_SPIM_PIN_NOT_USED;
    err_code = nrfx_spim_init(&spi_red_instance, &config, drv_led_spi_event_handler_red, NULL);
    VERIFY_SUCCESS(err_code);
    
    // Init SPIM for green  channel
    config.mosi_pin     = PIN_G_1;
    config.sck_pin      = NRFX_SPIM_PIN_NOT_USED;
    err_code = nrfx_spim_init(&spi_green_instance, &config, drv_led_spi_event_handler_green, NULL);
    VERIFY_SUCCESS(err_code);
    
    // Init SPIM for blue channel
    config.mosi_pin     = PIN_B_1;
    config.sck_pin      = PIN_CLK;
    err_code = nrfx_spim_init(&spi_blue_instance, &config, drv_led_spi_event_handler_blue, NULL);
    VERIFY_SUCCESS(err_code);
    
    return NRF_SUCCESS;
}


void TIMER1_IRQHandler(void)
{   
    static uint8_t pixel_row_number = 0;
        
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;
    
    nrf_gpio_pin_set(PIN_OE);
    nrf_gpio_pin_set(PIN_LAT);
    drv_led_update_latch_pins();
    nrf_gpio_pin_clear(PIN_OE);
    nrf_gpio_pin_clear(PIN_LAT);
    
    if(pixel_row_number == 0)
    {
        drv_led_reset_spi_transfer_config();
    }
    
    if(pixel_row_number == MATRIX_PIXEL_HEIGHT / 2)
    {
        drv_led_swap_spi_pins(ACTIVE_SCREEN_HALF_TOP);
    }
    else if(pixel_row_number == 0)
    {
        drv_led_swap_spi_pins(ACTIVE_SCREEN_HALF_BOTTOM);
    }

    drv_led_start_spi_transfer();
    
    pixel_row_number = (pixel_row_number + 1) & 0x1F; // Increment pixel row number. Reset to 0 on pixel row number overflow
    NRF_TIMER1->TASKS_START = 1;
}


static void drv_led_setup_latch_timer()
{
   // TIMER1 is used to control LC and LD pins. 
    NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
    NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER1->PRESCALER = 0;
    NRF_TIMER1->CC[0] = 4000 / MATRIX_MULTI_DRAW; 
    NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk | TIMER_SHORTS_COMPARE0_STOP_Msk;
    NRF_TIMER1->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
    NVIC_SetPriority(TIMER1_IRQn, 2);
    NVIC_EnableIRQ(TIMER1_IRQn);
}

static void drv_led_setup_gpios(void)
{    
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_R_1);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_B_1);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_G_1);
    
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_R_2);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_B_2);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_G_2);
    
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_LA);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_LB);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_LC);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_LD);
    
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_LAT);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_CLK);
    CONFIG_PIN_AS_OUTPUT_AND_CLEAR(PIN_OE);
}

static void drv_led_matrix_start(void)
{
    NRF_TIMER1->TASKS_START = 1;
}

uint32_t drv_led_matrix_init()
{
    uint32_t err_code;
    memset(&mbuff, 0, sizeof(mbuff));
    
    drv_led_setup_gpios();
    
    err_code = drv_led_setup_spi();
    VERIFY_SUCCESS(err_code);
    
    err_code = drv_led_reset_spi_transfer_config();
    VERIFY_SUCCESS(err_code);
    
    drv_led_setup_latch_timer();
    
    drv_led_matrix_start();
    
    return NRF_SUCCESS;
}


uint32_t drv_led_matrix_draw_pixel_rgb(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color = (r << 16) | (g << 8) | b;
    return drv_led_matrix_draw_pixel(x, y, color);
}

uint32_t drv_led_matrix_draw_pixel(uint8_t x, uint8_t y, uint32_t color)
{
    
    uint8_t color_red = (color & 0x00FF0000) >> 16;
    uint8_t color_green = (color & 0x0000FF00) >> 8;
    uint8_t color_blue = (color & 0x000000FF);
    
    if((x > MATRIX_PIXEL_WIDTH) || (y > MATRIX_PIXEL_HEIGHT))
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    
    for(int i = 0; i < MATRIX_MULTI_DRAW; i++)
    {
        if(color_red > ((255 / (MATRIX_MULTI_DRAW + 1)) * (i + 1)))
        {
            mbuff[i].r[y] |= (1ULL << x);
        }
        else
        {
            mbuff[i].r[y] &= ~(1ULL << x);
        }
    }
    for(int i = 0; i < MATRIX_MULTI_DRAW; i++)
    {
        if(color_green > ((255 / (MATRIX_MULTI_DRAW + 1)) * (i + 1)))
        {
            mbuff[i].g[y] |= (1ULL << x);
        }
        else
        {
            mbuff[i].g[y] &= ~(1ULL << x);
        }
    }
#ifndef SAVE_CURRENT    
    for(int i = 0; i < MATRIX_MULTI_DRAW; i++)
    {
        if(color_blue > ((255 / (MATRIX_MULTI_DRAW + 1)) * (i + 1)))
        {
            mbuff[i].b[y] |= (1ULL << x);
        }
        else
        {
            mbuff[i].b[y] &= ~(1ULL << x);
        }
    } 
#endif

    return NRF_SUCCESS;
}

uint32_t drv_led_matrix_draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    uint32_t err_code;
    for(int h = y; h < (y + height); h++)
    {
        for(int w = x; w < (x + width); w++)
        {
            err_code = drv_led_matrix_draw_pixel(w, h, color);
            VERIFY_SUCCESS(err_code);
        }
    }
    return NRF_SUCCESS;
}


/** @} */

