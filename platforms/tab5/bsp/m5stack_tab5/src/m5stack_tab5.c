/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"

#include "bsp_err_check.h"
#include "bsp/m5stack_tab5.h"
#include "bsp/bsp_pi4ioe.h"
#include "bsp/touch.h"
#include "bsp/display.h"
#include "esp_lcd_st7123.h"
#include "esp_lcd_touch_st7123.h"
#include "esp_codec_dev_defaults.h"
#include "esp_video_init.h"

#include "esp_log.h"

static const char *TAG = "bsp";

#if ( BSP_CONFIG_NO_GRAPHIC_LIB == 0 )
static lv_display_t          *disp;
static lv_indev_t            *disp_indev = NULL;
static bsp_lcd_handles_t      disp_handles;
static esp_lcd_touch_handle_t _lcd_touch_handle; // LCD touch handle
#if BSP_MIPI_DSI_PHY_PWR_LDO_CHAN > 0
static esp_ldo_channel_handle_t disp_phy_pwr_chan = NULL;
#endif
#endif // (BSP_CONFIG_NO_GRAPHIC_LIB == 0)

static sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL; //SD LDO handle
static sdmmc_card_t        *bsp_sdcard      = NULL; // Global uSD card handler

// i2s
static i2s_chan_handle_t            i2s_tx_chan = NULL;
static i2s_chan_handle_t            i2s_rx_chan = NULL;
static const audio_codec_data_if_t *i2s_data_if = NULL; /* Codec data interface */

/* Can be used for i2s_std_gpio_config_t and/or i2s_std_config_t initialization */
#define BSP_I2S_GPIO_CFG              \
    {                                 \
        .mclk         = BSP_I2S_MCLK, \
        .bclk         = BSP_I2S_SCLK, \
        .ws           = BSP_I2S_LCLK, \
        .dout         = BSP_I2S_DOUT, \
        .din          = BSP_I2S_DSIN, \
        .invert_flags = {             \
                         .mclk_inv = false,            \
                         .bclk_inv = false,            \
                         .ws_inv   = false,            \
                         },                            \
}

/* This configuration is used by default in bsp_codec_i2sdata_init() */
#define BSP_I2S_DUPLEX_CFG( _sample_rate )                                                              \
    {                                                                                                   \
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG( _sample_rate ),                                         \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG( I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO ), \
        .gpio_cfg = BSP_I2S_GPIO_CFG,                                                                   \
    }

char *bsp_get_info()
{
#if !defined( BOARD_NAME )
#define BOARD_NAME "UNDEFINED"
#endif
#if !defined( BOARD_VENDOR )
#define BOARD_VENDOR "UNDEFINED"
#endif
#define BOARD_INFO ( BOARD_VENDOR " " BOARD_NAME )
    return BOARD_INFO;
}
#if 0
/* the list to change driver capability */
static const gpio_num_t _driver_gpios[] = {
    // EXT I2C
    GPIO_NUM_0,
    GPIO_NUM_1,
    // esp-hosted esp32c6
    GPIO_NUM_8,
    GPIO_NUM_9,
    GPIO_NUM_10,
    GPIO_NUM_11,
    GPIO_NUM_12,
    GPIO_NUM_13,
    GPIO_NUM_15,
    // Display
    GPIO_NUM_22,
    GPIO_NUM_23,
    // Audio
    GPIO_NUM_26,
    GPIO_NUM_27,
    GPIO_NUM_28,
    GPIO_NUM_29,
    GPIO_NUM_30,
    // SYS I2C
    GPIO_NUM_31,
    GPIO_NUM_32,
    // uSD card
    GPIO_NUM_39,
    GPIO_NUM_40,
    GPIO_NUM_41,
    GPIO_NUM_42,
    GPIO_NUM_43,
    GPIO_NUM_44,
};
#endif
/* initial gpio config */
esp_err_t bsp_gpio_init( void )
{
#if 0
    // gpio_set_drive_capability((gpio_num_t)48, GPIO_DRIVE_CAP_0);
    for (int i = 0; i < sizeof(_driver_gpios) / sizeof(_driver_gpios[0]); i++) {
        gpio_num_t gpio = _driver_gpios[i];
        esp_err_t ret   = gpio_set_drive_capability(gpio, GPIO_DRIVE_CAP_0);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "GPIO %d drive capability set to GPIO_DRIVE_CAP_0", gpio);
        } else {
            ESP_LOGE(TAG, "Failed to set GPIO %d drive capability: %s", gpio, esp_err_to_name(ret));
        }
    }
#endif
    return ESP_OK;
}

/**
 * @brief I2C handle for BSP usage
 *
 * In IDF v5.4 you can call i2c_master_get_bus_handle(BSP_I2C_NUM, i2c_master_bus_handle_t *ret_handle)
 * from #include "esp_private/i2c_platform.h" to get this handle
 *
 * For IDF 5.2 and 5.3 you must call bsp_i2c_get_handle()
 */
// sys i2c
static i2c_master_bus_handle_t s_i2c_bus_handle = NULL;

esp_err_t bsp_i2c_init( void )
{
    /* I2C was initialized before */
    if ( s_i2c_bus_handle != NULL ) {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_conf = {
        .i2c_port                     = BSP_I2C_NUM,
        .sda_io_num                   = BSP_I2C_SDA,
        .scl_io_num                   = BSP_I2C_SCL,
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
    };
    BSP_ERROR_CHECK_RETURN_ERR( i2c_new_master_bus( &i2c_bus_conf, &s_i2c_bus_handle ) );

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit( void )
{
    if ( s_i2c_bus_handle == NULL ) {
        return ESP_OK;
    }
    BSP_ERROR_CHECK_RETURN_ERR( i2c_del_master_bus( s_i2c_bus_handle ) );
    s_i2c_bus_handle = NULL;
    return ESP_OK;
}

esp_err_t bsp_i2c_device_probe( uint8_t addr )
{
    return i2c_master_probe( s_i2c_bus_handle, addr, 100 );
}

i2c_master_bus_handle_t bsp_i2c_get_handle( void )
{
    if ( s_i2c_bus_handle == NULL ) {
        bsp_i2c_init();
    }
    return s_i2c_bus_handle;
}

// ext i2c
static bool                    ext_i2c_initialized  = false;
static i2c_master_bus_handle_t s_ext_i2c_bus_handle = NULL;

esp_err_t bsp_ext_i2c_init( void )
{
    if ( ext_i2c_initialized ) {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = BSP_EXT_I2C_NUM,
        .scl_io_num                   = BSP_EXT_I2C_SCL,
        .sda_io_num                   = BSP_EXT_I2C_SDA,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus( &i2c_mst_config, &s_ext_i2c_bus_handle );

    ext_i2c_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_ext_i2c_deinit( void )
{
    ext_i2c_initialized = false;
    return i2c_del_master_bus( s_ext_i2c_bus_handle );
}

esp_err_t bsp_ext_i2c_device_probe( uint8_t addr )
{
    return i2c_master_probe( s_ext_i2c_bus_handle, addr, 100 );
}

i2c_master_bus_handle_t bsp_ext_i2c_get_handle( void )
{
    if ( !ext_i2c_initialized )
        bsp_ext_i2c_init();
    return s_ext_i2c_bus_handle;
}

#if 0
// grove i2c
static bool grove_i2c_initialized                   = false;
static i2c_master_bus_handle_t grove_i2c_bus_handle = NULL;

esp_err_t bsp_grove_i2c_init(void)
{
    if (grove_i2c_initialized) {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = BSP_EXT_I2C_NUM,
        .scl_io_num                   = 54,  // BSP_EXT_I2C_SCL,
        .sda_io_num                   = 53,  // BSP_EXT_I2C_SDA,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&i2c_mst_config, &grove_i2c_bus_handle);

    grove_i2c_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_grove_i2c_deinit(void)
{
    grove_i2c_initialized = false;
    return i2c_del_master_bus(grove_i2c_bus_handle);
}

i2c_master_bus_handle_t bsp_grove_i2c_get_handle(void)
{
    return grove_i2c_bus_handle;
}
#endif

esp_err_t bsp_i2c_scan( i2c_master_bus_handle_t i2c_handle )
{
    esp_err_t ret;
    uint8_t   address;

    if ( i2c_handle == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }

    printf( "scan i2c device\n" );
    printf( "\n     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n" );
    for ( int i = 0; i < 128; i += 16 ) {
        printf( "%02x: ", i );
        for ( int j = 0; j < 16; j++ ) {
            fflush( stdout );
            address = i + j;
            ret     = i2c_master_probe( i2c_handle, address, 50 );
            if ( ret == ESP_OK ) {
                printf( "%02x ", address );
            }
            else if ( ret == ESP_ERR_TIMEOUT ) {
                printf( "UU " );
            }
            else {
                printf( "-- " );
            }
        }
        printf( "\r\n" );
    }
    printf( "\nscan i2c device finished\n" );

    return ESP_OK;
}

void bsp_set_charge_qc_en( bool en )
{
    bsp_io_expander2_set_bit( IO_EXPANDER2_nCHG_QC_EN, !en );
}

void bsp_set_charge_en( bool en )
{
    bsp_io_expander2_set_bit( IO_EXPANDER2_CHG_EN, en );
}

void bsp_set_usb_5v_en( bool en )
{
    bsp_io_expander2_set_bit( IO_EXPANDER2_USB5V_EN, en );

}

void bsp_set_ext_5v_en( bool en )
{

    bsp_io_expander1_set_bit( IO_EXPANDER1_EXT5V_EN, en );

}

void bsp_generate_poweroff_signal()
{
    ESP_LOGW( TAG, "Generate poweroff signal!" );

    // Try to generate poweroff signal 3 times to make sure it works :)
    for ( int i = 0; i < 3; i++ ) {

        bsp_io_expander2_set_bit( IO_EXPANDER2_PWEROFF_PULSE, 1 );
        vTaskDelay( 100 / portTICK_PERIOD_MS );

        bsp_io_expander2_set_bit( IO_EXPANDER2_PWEROFF_PULSE, 0 );
        vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}

bool bsp_headphone_detect( void )
{
    uint8_t hp_detect = bsp_io_expander1_read_bit( IO_EXPANDER1_HP_DETECT );
    
    //ESP_LOGI( TAG, "headphone_detect: %d", hp_detect );

    if ( hp_detect ) {
        return true;
    }

    return false;
}
    
bool bsp_usb_c_detect( void )
{
    uint8_t usb_c_det = bsp_io_expander2_read_bit( IO_EXPANDER2_CHG_EN );

    //ESP_LOGI(TAG, "usb_c_detect: %d", usb_c_det);

    if ( usb_c_det ) {
        return true;
    }

    return false;
}

void bsp_set_ext_antenna_enable( bool en )
{
    bsp_io_expander1_set_bit( IO_EXPANDER1_RF_PTH_L_INT_H_EXT, en );
}

void bsp_set_wifi_power_enable( bool en )
{

    ESP_LOGI( TAG, "set_wifi_power_enable: %d", en );

    bsp_io_expander2_set_bit( IO_EXPANDER2_WLAN_PWR_EN, en );

    uint8_t wlen_power;

    wlen_power = bsp_io_expander2_read_bit( IO_EXPANDER2_WLAN_PWR_EN );

    ESP_LOGI( TAG, "wlen %d", wlen_power );
}

void bsp_reset_tp()
{
    ESP_LOGI( TAG, "reset tp" );

    bsp_io_expander1_set_bit( IO_EXPANDER1_TP_RST, 0 );
    vTaskDelay( pdMS_TO_TICKS( 10 ) );
    bsp_io_expander1_set_bit( IO_EXPANDER1_TP_RST, 1 );
    vTaskDelay( pdMS_TO_TICKS( 100 ) );
}

void bsp_set_camera_enable( bool en )
{
    bsp_io_expander1_set_bit( IO_EXPANDER1_CAM_RST, en );
}

#include "esp_spiffs.h"
//==================================================================================
// spiffs
//==================================================================================
esp_err_t bsp_spiffs_mount( void )
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path              = BSP_SPIFFS_MOUNT_POINT,
        .partition_label        = BSP_SPIFFS_PARTITION_LABEL,
        .max_files              = BSP_SPIFFS_MAX_FILES,
        .format_if_mount_failed = BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL,
    };

    esp_err_t ret_val = esp_vfs_spiffs_register( &conf );

    BSP_ERROR_CHECK_RETURN_ERR( ret_val );

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info( conf.partition_label, &total, &used );
    if ( ret_val != ESP_OK ) {
        ESP_LOGE( TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name( ret_val ) );
    }
    else {
        ESP_LOGI( TAG, "Partition size: total: %d, used: %d", total, used );
    }

    return ret_val;
}

esp_err_t bsp_spiffs_unmount( void )
{
    return esp_vfs_spiffs_unregister( BSP_SPIFFS_PARTITION_LABEL );
}

//==================================================================================
// sd card
//==================================================================================
#define BSP_LDO_PROBE_SD_CHAN ( 4 ) // LDO channel for uSD card power
sdmmc_card_t *bsp_sdcard_get_handle( void )
{
    return bsp_sdcard;
}

void bsp_sdcard_get_sdmmc_host( const int slot, sdmmc_host_t *config )
{
    assert( config );
    sdmmc_host_t host_config = SDMMC_HOST_DEFAULT();
    host_config.slot         = slot;
    host_config.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    memcpy( config, &host_config, sizeof( sdmmc_host_t ) );
}

void bsp_sdcard_sdmmc_get_slot( const int slot, sdmmc_slot_config_t *config )
{
    assert( config );
    memset( config, 0, sizeof( sdmmc_slot_config_t ) );
    /* SD card is connected to Slot 0 pins. Slot 0 uses IO MUX, so not specifying the pins here */
    config->width = BSP_SD_BUS_WIDTH;
    config->cd    = BSP_SD_DET; //SDMMC_SLOT_NO_CD;
    config->wp    = SDMMC_SLOT_NO_WP;
    config->cmd   = BSP_SD_CMD;
    config->clk   = BSP_SD_CLK;
    config->d0    = BSP_SD_D0;
    config->d1    = BSP_SD_D1;
    config->d2    = BSP_SD_D2;
    config->d3    = BSP_SD_D3;
    config->flags = 0;
    // config->flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
}

esp_err_t bsp_sdcard_init( const char *mount_point, size_t max_files )
{
    esp_err_t ret = ESP_OK;

    if ( NULL != bsp_sdcard ) {
        return ESP_ERR_INVALID_STATE;
    }

    sdmmc_host_t host;
    sdmmc_slot_config_t slot_config;
    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = BSP_SD_FORMAT_ON_MOUNT_FAIL,
        .max_files              = max_files,
        .allocation_unit_size   = 16 * 1024
    };

    bsp_sdcard_get_sdmmc_host( SDMMC_HOST_SLOT_0, &host );

    bsp_sdcard_sdmmc_get_slot( SDMMC_HOST_SLOT_0, &slot_config );

#if ( BSP_LDO_PROBE_SD_CHAN > 0 )
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = BSP_LDO_PROBE_SD_CHAN, // `LDO_VO4` is used as the SDMMC IO power
    };

    if ( pwr_ctrl_handle == NULL ) {
        ret = sd_pwr_ctrl_new_on_chip_ldo( &ldo_config, &pwr_ctrl_handle );
        if ( ret != ESP_OK ) {
            ESP_LOGE( TAG, "Failed to create a new on-chip LDO power control driver" );
            return ret;
        }
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif //#if ( BSP_LDO_PROBE_SD_CHAN > 0 )

#if 0 //no additonal power control //( BSP_SD_PWR_CTRL_IO > 0 )
    gpio_config_t io_conf = {
        .pin_bit_mask = ( 1ULL << BSP_SD_PWR_CTRL_IO ),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };

    gpio_config( &io_conf );
    gpio_set_level( BSP_SD_PWR_CTRL_IO, 0 ); // P-FET ON
    vTaskDelay( pdMS_TO_TICKS( 10 ) ); // 안정화 대기
#endif


#if !CONFIG_FATFS_LONG_FILENAMES
    ESP_LOGW( TAG, "Warning: Long filenames on SD card are disabled in menuconfig!" );
#endif

    ret = esp_vfs_fat_sdmmc_mount( mount_point, &host, &slot_config, &mount_config, &bsp_sdcard );

    /* Check for SDMMC mount result. */
    if ( ret != ESP_OK ) {
        if ( ret == ESP_FAIL ) {
            ESP_LOGE( TAG,
                      "Failed to mount filesystem. "
                      "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option." );
        }
        else {
            ESP_LOGE( TAG,
                      "Failed to initialize the card (%s). "
                      "Make sure SD card lines have pull-up resistors in place.",
                      esp_err_to_name( ret ) );
        }
        return ret;
    }

    /* Card has been initialized, print its properties. */
    sdmmc_card_print_info( stdout, bsp_sdcard );

    return ret;
}

esp_err_t bsp_sdcard_deinit( const char *mount_point )
{
    esp_err_t ret = ESP_OK;

    if ( bsp_sdcard == NULL ) {
        return ESP_ERR_INVALID_STATE;
    }
#if 0 //no need //( BSP_LDO_PROBE_SD_CHAN > 0 ) 
   if (pwr_ctrl_handle) {
        ret |= sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
        pwr_ctrl_handle = NULL;
    }

#endif //#if ( BSP_LDO_PROBE_SD_CHAN > 0 )

    /* Unmount an SD card from the FAT filesystem and release resources acquired */

    ret |= esp_vfs_fat_sdcard_unmount( mount_point, bsp_sdcard );

#if 0 //no additonal power control //( BSP_SD_PWR_CTRL_IO > 0 )
    
    gpio_set_level( BSP_SD_PWR_CTRL_IO, 1 ); // P-FET OFF
    vTaskDelay( pdMS_TO_TICKS( 10 ) ); // 안정화 대기

#endif //#if ( BSP_SD_PWR_CTRL_IO > 0 )

    /* Make SD/MMC card information structure pointer NULL */
    bsp_sdcard = NULL;
    //if (spi_sd_initialized) {
    //    ret |= spi_bus_free(BSP_SDSPI_HOST);
    //    spi_sd_initialized = false;
    //}
    return ret;
}

esp_err_t bsp_sdcard_mount( void )
{
    return bsp_sdcard_init( BSP_SD_MOUNT_POINT, BSP_SD_MAX_OPENED_FILES );
}

esp_err_t bsp_sdcard_unmount( void )
{
    return bsp_sdcard_deinit( BSP_SD_MOUNT_POINT );
}

//==================================================================================
// audio es7210 + es8388
//==================================================================================
static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;
static bsp_codec_config_t     g_codec_handle;
static int                    volume;

/**************************************************************************************************
 *
 * I2S Audio Function
 *
 **************************************************************************************************/
esp_err_t bsp_codec_i2sdata_init( const i2s_std_config_t *i2s_config )
{
    esp_err_t ret = ESP_FAIL;
    if ( i2s_tx_chan && i2s_rx_chan ) {
        /* Audio was initialized before */
        return ESP_OK;
    }

    /* Setup I2S peripheral */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG( BSP_I2S_NUM, I2S_ROLE_MASTER );
    chan_cfg.auto_clear        = true; // Auto clear the legacy data in the DMA buffer
    BSP_ERROR_CHECK_RETURN_ERR( i2s_new_channel( &chan_cfg, &i2s_tx_chan, &i2s_rx_chan ) );

    /* Setup I2S channels */
    const i2s_std_config_t  std_cfg_default = BSP_I2S_DUPLEX_CFG( BSP_I2S_SAMPLE_RATE );
    const i2s_std_config_t *p_i2s_cfg       = &std_cfg_default;
    if ( i2s_config != NULL ) {
        p_i2s_cfg = i2s_config;
    }

    if ( i2s_tx_chan != NULL ) {
        ESP_GOTO_ON_ERROR( i2s_channel_init_std_mode( i2s_tx_chan, p_i2s_cfg ), err, TAG, "I2S channel initialization failed" );
        ESP_GOTO_ON_ERROR( i2s_channel_enable( i2s_tx_chan ), err, TAG, "I2S enabling failed" );
    }

    // if (i2s_rx_chan != NULL) {
    //     ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_rx_chan, p_i2s_cfg));
    //     ESP_ERROR_CHECK(i2s_channel_enable(i2s_rx_chan));
    // }

    i2s_tdm_config_t tdm_cfg = {
        .clk_cfg = {
                    .sample_rate_hz  = (uint32_t)BSP_I2S_SAMPLE_RATE,
                    .clk_src         = I2S_CLK_SRC_DEFAULT,
                    .ext_clk_freq_hz = 0,
                    .mclk_multiple   = I2S_MCLK_MULTIPLE_256,
                    .bclk_div        = 8,
                    },
        .slot_cfg = { .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT, 
		              .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, 
					  .slot_mode = I2S_SLOT_MODE_STEREO, 
					  .slot_mask = ( I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3 ), 
					  .ws_width = I2S_TDM_AUTO_WS_WIDTH, 
					  .ws_pol = false, 
					  .bit_shift = true, 
					  .left_align = false, 
					  .big_endian = false, 
					  .bit_order_lsb = false, 
					  .skip_mask = false, 
					  .total_slot = I2S_TDM_AUTO_SLOT_NUM },
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };

    if ( i2s_rx_chan != NULL ) {
        ESP_GOTO_ON_ERROR( i2s_channel_init_tdm_mode( i2s_rx_chan, &tdm_cfg ), err, TAG, "I2S channel initialization failed" );
        ESP_GOTO_ON_ERROR( i2s_channel_enable( i2s_rx_chan ), err, TAG, "I2S enabling failed" );
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port      = BSP_I2S_NUM,
        .tx_handle = i2s_tx_chan,
        .rx_handle = i2s_rx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data( &i2s_cfg );
    BSP_NULL_CHECK_GOTO( i2s_data_if, err );

    return ESP_OK;

err:
    if ( i2s_tx_chan ) {
        i2s_del_channel( i2s_tx_chan );
    }
    if ( i2s_rx_chan ) {
        i2s_del_channel( i2s_rx_chan );
    }

    return ret;
}

const audio_codec_data_if_t *bsp_i2s_codec_get_itf( void )
{
    return i2s_data_if;
}

esp_codec_dev_handle_t bsp_audio_codec_speaker_init( void )
{
    static esp_codec_dev_handle_t codec = NULL;
    if ( codec ) {
        return codec;
    }

    const audio_codec_data_if_t *i2s_data_if = bsp_i2s_codec_get_itf();
    if ( i2s_data_if == NULL ) {
        /* Initilize I2C */
        BSP_ERROR_CHECK_RETURN_NULL( bsp_i2c_init() );
        /* Configure I2S peripheral and Power Amplifier */
        BSP_ERROR_CHECK_RETURN_NULL( bsp_codec_i2sdata_init( NULL ) );
        i2s_data_if = bsp_i2s_codec_get_itf();
    }
    assert( i2s_data_if );
    /* Enable Feature */
    //BSP_ERROR_CHECK_RETURN_NULL(bsp_feature_enable(BSP_FEATURE_SPEAKER, true));
	
    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port       = BSP_I2C_NUM,
        .addr       = ES8388_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl( &i2c_cfg );
    BSP_NULL_CHECK( i2c_ctrl_if, NULL );

    es8388_codec_cfg_t es8388_cfg = {
        .ctrl_if     = i2c_ctrl_if,
        .gpio_if     = NULL,
        .codec_mode  = ESP_CODEC_DEV_WORK_MODE_DAC,
        .master_mode = false,
        .pa_pin      = BSP_POWER_AMP_IO, // use PI4IOE1 P1
        .hw_gain     = {
            .pa_voltage        = 5.0, /*default 5.0V */
            .codec_dac_voltage = 3.3, /*default 3.3V */
            .pa_gain           = 0.0, /*default 0.0dB */
        }
    };
    const audio_codec_if_t *codec_dev = es8388_codec_new( &es8388_cfg );
    BSP_NULL_CHECK( codec_dev, NULL );

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_dev,
        .data_if  = i2s_data_if,
    };
    codec = esp_codec_dev_new( &codec_dev_cfg );
    BSP_NULL_CHECK( codec, NULL );

    //Brandon: volume curve setup??

    return codec;
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init( void )
{
    const audio_codec_data_if_t *i2s_data_if = bsp_i2s_codec_get_itf();
    if ( i2s_data_if == NULL ) {
        BSP_ERROR_CHECK_RETURN_NULL( bsp_i2c_init() );
        /* Configure I2S peripheral and Power Amplifier */
        BSP_ERROR_CHECK_RETURN_NULL( bsp_codec_i2sdata_init( NULL ) );
        i2s_data_if = bsp_i2s_codec_get_itf();
    }
    BSP_NULL_CHECK( i2s_data_if, NULL );

    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port       = BSP_I2C_NUM,
        .addr       = ES7210_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl( &i2c_cfg );
    BSP_NULL_CHECK( i2c_ctrl_if, NULL );

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = i2c_ctrl_if, // Codec Control interface
    };
    es7210_cfg.mic_selected           = ES7210_SEL_MIC1 | ES7210_SEL_MIC2 | ES7210_SEL_MIC3 | ES7210_SEL_MIC4;
    const audio_codec_if_t *codec_dev = es7210_codec_new( &es7210_cfg );
    BSP_NULL_CHECK( codec_dev, NULL );

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN, // Codec device type: Codec input device like ADC (capture data from microphone)
        .codec_if = codec_dev, // Codec interface
        .data_if  = i2s_data_if, // Codec data interface
    };

    return esp_codec_dev_new( &codec_dev_cfg );
}

static esp_err_t bsp_i2s_read( void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms )
{
    esp_err_t ret = ESP_OK;
    ret           = esp_codec_dev_read( record_dev_handle, audio_buffer, len );
    *bytes_read   = len;
    return ret;
}

static esp_err_t bsp_i2s_write( void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms )
{
    esp_err_t ret  = ESP_OK;
    ret            = esp_codec_dev_write( play_dev_handle, audio_buffer, len );
    *bytes_written = len;
    return ret;
}

static esp_err_t bsp_codec_set_in_gain( float gain )
{
    return esp_codec_dev_set_in_gain( record_dev_handle, gain );
}

static esp_err_t bsp_codec_set_mute( bool enable )
{
    esp_err_t ret = ESP_OK;
    ret           = esp_codec_dev_set_out_mute( play_dev_handle, enable );
    return ret;
}

static esp_err_t bsp_codec_set_volume( int v )
{
    esp_err_t ret = ESP_OK;

    if ( v <= 0 ) {
        volume = 0;
        ret    = esp_codec_dev_set_out_mute( play_dev_handle, true );
    }
    else {
        volume = v;
        ret    = esp_codec_dev_set_out_mute( play_dev_handle, false );
        ret |= esp_codec_dev_set_out_vol( play_dev_handle, volume );
    }

    return ret;
}

static int bsp_codec_get_volume( void )
{
    return volume;
}

bsp_codec_config_t *bsp_get_codec_handle( void )
{
    return &g_codec_handle;
}

static esp_err_t bsp_codec_es8388_set( uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch )
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate     = rate,
        .channel         = ch,
        .bits_per_sample = bits_cfg,
    };

    if ( play_dev_handle ) {
        ret = esp_codec_dev_close( play_dev_handle );
    }
    ret = esp_codec_dev_open( play_dev_handle, &fs );

    return ret;
}

static esp_err_t bsp_codec_es7210_set( uint32_t rate, uint32_t bps, i2s_slot_mode_t ch )
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate     = rate,
        .channel         = ch,
        .bits_per_sample = bps,
    };

    if ( record_dev_handle ) {
        ret = esp_codec_dev_close( record_dev_handle );
    }
    ret = esp_codec_dev_open( record_dev_handle, &fs );

    // esp_codec_dev_set_in_gain(record_dev_handle, 80.0); // Set codec input gain

    return ret;
}

void bsp_codec_init( void )
{
    play_dev_handle = bsp_audio_codec_speaker_init();
    assert( ( play_dev_handle ) && "play_dev_handle not initialized" );

    record_dev_handle = bsp_audio_codec_microphone_init();
    assert( ( record_dev_handle ) && "record_dev_handle not initialized" );

    // bsp_codec_es7210_set(16000, 16, 2);
    // bsp_codec_es8388_set(16000, 16, 2);
    // bsp_codec_es7210_set(48000, 16, 2);
    bsp_codec_es7210_set( BSP_I2S_SAMPLE_RATE, 16, 4 );
    bsp_codec_es8388_set( BSP_I2S_SAMPLE_RATE, 16, 2 );

	/* codec handle */
    bsp_codec_config_t *codec_cfg  = &g_codec_handle; 
    codec_cfg->i2s_read            = bsp_i2s_read; 
    codec_cfg->i2s_write           = bsp_i2s_write; 
    codec_cfg->set_mute            = bsp_codec_set_mute; 
    codec_cfg->set_volume          = bsp_codec_set_volume; 
    codec_cfg->get_volume          = bsp_codec_get_volume;
    codec_cfg->set_in_gain         = bsp_codec_set_in_gain; 
    codec_cfg->codec_reconfig_fn   = bsp_codec_es7210_set;
    codec_cfg->i2s_reconfig_clk_fn = bsp_codec_es8388_set;

    codec_cfg->set_volume( 80 );

    //return ESP_OK;
}

uint8_t bsp_codec_feed_channel( void )
{
    return 3; // 2*mic_num + ref_num
}

//==================================================================================
// lcd st7703 1280x720  gt911
//==================================================================================
#if ( BSP_CONFIG_NO_GRAPHIC_LIB == 0 )

#ifndef CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH
#define CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH LEDC_CHANNEL_1
#endif
#define LCD_LEDC_CH CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH

esp_err_t bsp_display_brightness_init( void )
{
    // gpio_config_t io_conf = {};

    // io_conf.intr_type = GPIO_INTR_DISABLE;   //disable interrupt
    // io_conf.mode = GPIO_MODE_OUTPUT;         //set as output mode
    // io_conf.pin_bit_mask = 1 << BSP_LCD_BACKLIGHT; //select pin
    // io_conf.pull_down_en = 0;                //disable pull-down mode
    // io_conf.pull_up_en = 0;                  //disable pull-up mode
    // gpio_config(&io_conf);                   //configure GPIO with the given settings

    // gpio_set_level(BSP_LCD_BACKLIGHT, 1);

    // Setup LEDC peripheral for PWM backlight control

    const ledc_channel_config_t lcd_backlight_channel = {
        .gpio_num   = BSP_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LCD_LEDC_CH,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0
    };
    const ledc_timer_config_t lcd_backlight_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK
    };

    BSP_ERROR_CHECK_RETURN_ERR( ledc_timer_config( &lcd_backlight_timer ) );
    BSP_ERROR_CHECK_RETURN_ERR( ledc_channel_config( &lcd_backlight_channel ) );

    return ESP_OK;
}

esp_err_t bsp_display_brightness_deinit( void )
{
    const ledc_timer_config_t lcd_backlight_timer = {
        .speed_mode  = LEDC_LOW_SPEED_MODE,
        .timer_num   = LEDC_TIMER_0,
        .deconfigure = 1
    };
    BSP_ERROR_CHECK_RETURN_ERR( ledc_timer_pause( LEDC_LOW_SPEED_MODE, 1 ) );
    BSP_ERROR_CHECK_RETURN_ERR( ledc_timer_config( &lcd_backlight_timer ) );
    return ESP_OK;
}

esp_err_t bsp_display_brightness_set( int brightness_percent )
{
    if ( brightness_percent > 100 ) {
        brightness_percent = 100;
    }
    if ( brightness_percent < 0 ) {
        brightness_percent = 0;
    }

    ESP_LOGI( TAG, "Setting LCD backlight: %d%%", brightness_percent );
    // uint32_t duty_cycle = (1023 * brightness_percent) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
    uint32_t duty_cycle = ( 4095 * brightness_percent ) / 100; // LEDC resolution set to 12bits, thus: 100% = 4095
    BSP_ERROR_CHECK_RETURN_ERR( ledc_set_duty( LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle ) );
    BSP_ERROR_CHECK_RETURN_ERR( ledc_update_duty( LEDC_LOW_SPEED_MODE, LCD_LEDC_CH ) );

    return ESP_OK;
}

esp_err_t bsp_display_backlight_off( void )
{
    return bsp_display_brightness_set( 0 );
}

esp_err_t bsp_display_backlight_on( void )
{
    return bsp_display_brightness_set( 100 );
}

static esp_err_t bsp_enable_dsi_phy_power( void )
{
#if BSP_MIPI_DSI_PHY_PWR_LDO_CHAN > 0
    // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id    = BSP_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = BSP_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    ESP_RETURN_ON_ERROR( esp_ldo_acquire_channel( &ldo_cfg, &disp_phy_pwr_chan ), TAG, "Acquire LDO channel for DPHY failed" );
    ESP_LOGI( TAG, "MIPI DSI PHY Powered on" );
#endif // BSP_MIPI_DSI_PHY_PWR_LDO_CHAN > 0

    return ESP_OK;
}

static esp_err_t bsp_disable_dsi_phy_power( void )
{
#if BSP_MIPI_DSI_PHY_PWR_LDO_CHAN > 0
    // Turn off the power for MIPI DSI PHY
    if ( disp_phy_pwr_chan ) {
        ESP_LOGI( TAG, "MIPI DSI PHY Powered off" );
        ESP_RETURN_ON_ERROR( esp_ldo_release_channel( disp_phy_pwr_chan ), TAG, "Release LDO channel for DPHY failed" );
        disp_phy_pwr_chan = NULL;
    }
#endif // BSP_MIPI_DSI_PHY_PWR_LDO_CHAN > 0

    return ESP_OK;
}

#include "esp_lcd_panel_interface.h"

static esp_err_t bsp_ext_io_lcd_reset( esp_lcd_panel_t *panel )
{
    (void)panel;
    ESP_LOGI( TAG, "Reset LCD panel via IO Expander" );
    bsp_io_expander1_set_bit( IO_EXPANDER1_LCD_RST, 0 );
    vTaskDelay( pdMS_TO_TICKS( 50 ) );
    bsp_io_expander1_set_bit( IO_EXPANDER1_LCD_RST, 1 );
    vTaskDelay( pdMS_TO_TICKS( 50 ) );
    return ESP_OK;
}

esp_err_t bsp_display_new( const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io )
{
    esp_err_t         ret = ESP_OK;
    bsp_lcd_handles_t handles;
    ret = bsp_display_new_with_handles( config, &handles );

    *ret_panel = handles.panel;
    *ret_io    = handles.io;

    return ret;
}

#define LCD_MIPI_DSI_USE_ST7123
#ifdef LCD_MIPI_DSI_USE_ST7123
#include "esp_lcd_st7123.h"

// ST7123 vendor specific initialization commands
static const st7123_lcd_init_cmd_t st7123_vendor_specific_init_default[] = {
    {0x60, (uint8_t[]){0x71, 0x23, 0xa2}, 3, 0},
    {0x60, (uint8_t[]){0x71, 0x23, 0xa3}, 3, 0},
    {0x60, (uint8_t[]){0x71, 0x23, 0xa4}, 3, 0},
    {0xA4, (uint8_t[]){0x31}, 1, 0},
    {0xD7, (uint8_t[]){0x10, 0x0A, 0x10, 0x2A, 0x80, 0x80}, 6, 0},
    {0x90, (uint8_t[]){0x71, 0x23, 0x5A, 0x20, 0x24, 0x09, 0x09}, 7, 0},
    {0xA3, (uint8_t[]){0x80, 0x01, 0x88, 0x30, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00,
                       0x1E, 0x5C, 0x1E, 0x80, 0x00, 0x4F, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46,
                       0x00, 0x00, 0x1E, 0x5C, 0x1E, 0x80, 0x00, 0x6F, 0x58, 0x00, 0x00, 0x00, 0xFF},
     40, 0},
    {0xA6, (uint8_t[]){0x03, 0x00, 0x24, 0x55, 0x36, 0x00, 0x39, 0x00, 0x6E, 0x6E, 0x91, 0xFF, 0x00, 0x24,
                       0x55, 0x38, 0x00, 0x37, 0x00, 0x6E, 0x6E, 0x91, 0xFF, 0x00, 0x24, 0x11, 0x00, 0x00,
                       0x00, 0x00, 0x6E, 0x6E, 0x91, 0xFF, 0x00, 0xEC, 0x11, 0x00, 0x03, 0x00, 0x03, 0x6E,
                       0x6E, 0xFF, 0xFF, 0x00, 0x08, 0x80, 0x08, 0x80, 0x06, 0x00, 0x00, 0x00, 0x00},
     55, 0},
    {0xA7, (uint8_t[]){0x19, 0x19, 0x80, 0x64, 0x40, 0x07, 0x16, 0x40, 0x00, 0x44, 0x03, 0x6E, 0x6E, 0x91, 0xFF,
                       0x08, 0x80, 0x64, 0x40, 0x25, 0x34, 0x40, 0x00, 0x02, 0x01, 0x6E, 0x6E, 0x91, 0xFF, 0x08,
                       0x80, 0x64, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x6E, 0x6E, 0x91, 0xFF, 0x08, 0x80,
                       0x64, 0x40, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x6E, 0x6E, 0x84, 0xFF, 0x08, 0x80, 0x44},
     60, 0},
    {0xAC, (uint8_t[]){0x03, 0x19, 0x19, 0x18, 0x18, 0x06, 0x13, 0x13, 0x11, 0x11, 0x08, 0x08, 0x0A, 0x0A, 0x1C,
                       0x1C, 0x07, 0x07, 0x00, 0x00, 0x02, 0x02, 0x01, 0x19, 0x19, 0x18, 0x18, 0x06, 0x12, 0x12,
                       0x10, 0x10, 0x09, 0x09, 0x0B, 0x0B, 0x1C, 0x1C, 0x07, 0x07, 0x03, 0x03, 0x01, 0x01},
     44, 0},
    {0xAD, (uint8_t[]){0xF0, 0x00, 0x46, 0x00, 0x03, 0x50, 0x50, 0xFF, 0xFF, 0xF0, 0x40, 0x06, 0x01,
                       0x07, 0x42, 0x42, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF},
     25, 0},
    {0xAE, (uint8_t[]){0xFE, 0x3F, 0x3F, 0xFE, 0x3F, 0x3F, 0x00}, 7, 0},
    {0xB2,
     (uint8_t[]){0x15, 0x19, 0x05, 0x23, 0x49, 0xAF, 0x03, 0x2E, 0x5C, 0xD2, 0xFF, 0x10, 0x20, 0xFD, 0x20, 0xC0, 0x00},
     17, 0},
    {0xE8, (uint8_t[]){0x20, 0x6F, 0x04, 0x97, 0x97, 0x3E, 0x04, 0xDC, 0xDC, 0x3E, 0x06, 0xFA, 0x26, 0x3E}, 15, 0},
    {0x75, (uint8_t[]){0x03, 0x04}, 2, 0},
    {0xE7, (uint8_t[]){0x3B, 0x00, 0x00, 0x7C, 0xA1, 0x8C, 0x20, 0x1A, 0xF0, 0xB1, 0x50, 0x00,
                       0x50, 0xB1, 0x50, 0xB1, 0x50, 0xD8, 0x00, 0x55, 0x00, 0xB1, 0x00, 0x45,
                       0xC9, 0x6A, 0xFF, 0x5A, 0xD8, 0x18, 0x88, 0x15, 0xB1, 0x01, 0x01, 0x77},
     36, 0},
    {0xEA, (uint8_t[]){0x13, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x2C}, 8, 0},
    {0xB0, (uint8_t[]){0x22, 0x43, 0x11, 0x61, 0x25, 0x43, 0x43}, 7, 0},
    {0xb7, (uint8_t[]){0x00, 0x00, 0x73, 0x73}, 0x04, 0},
    {0xBF, (uint8_t[]){0xA6, 0XAA}, 2, 0},
    {0xA9, (uint8_t[]){0x00, 0x00, 0x73, 0xFF, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03}, 10, 0},
    {0xC8, (uint8_t[]){0x00, 0x00, 0x10, 0x1F, 0x36, 0x00, 0x5D, 0x04, 0x9D, 0x05, 0x10, 0xF2, 0x06,
                       0x60, 0x03, 0x11, 0xAD, 0x00, 0xEF, 0x01, 0x22, 0x2E, 0x0E, 0x74, 0x08, 0x32,
                       0xDC, 0x09, 0x33, 0x0F, 0xF3, 0x77, 0x0D, 0xB0, 0xDC, 0x03, 0xFF},
     37, 0},
    {0xC9, (uint8_t[]){0x00, 0x00, 0x10, 0x1F, 0x36, 0x00, 0x5D, 0x04, 0x9D, 0x05, 0x10, 0xF2, 0x06,
                       0x60, 0x03, 0x11, 0xAD, 0x00, 0xEF, 0x01, 0x22, 0x2E, 0x0E, 0x74, 0x08, 0x32,
                       0xDC, 0x09, 0x33, 0x0F, 0xF3, 0x77, 0x0D, 0xB0, 0xDC, 0x03, 0xFF},
     37, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x11, (uint8_t[]){0x00}, 1, 100},
    {0x29, (uint8_t[]){0x00}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 100},
};

esp_err_t bsp_display_new_with_handles( const bsp_display_config_t *config, bsp_lcd_handles_t *ret_handles )
{
    esp_err_t ret = ESP_OK;

    /* Enable Feature */
    //BSP_ERROR_CHECK_RETURN_ERR(bsp_feature_enable(BSP_FEATURE_LCD, true));
    ESP_RETURN_ON_ERROR( bsp_display_brightness_init(), TAG, "Brightness init failed" );
    ESP_RETURN_ON_ERROR( bsp_enable_dsi_phy_power(), TAG, "DSI PHY power failed" );

    /* create MIPI DSI bus first, it will initialize the DSI PHY as well */
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id             = 0,
        .num_data_lanes     = BSP_LCD_MIPI_DSI_LANE_NUM,
        .phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS,
    };

    esp_lcd_dsi_bus_handle_t mipi_dsi_bus;
    ESP_RETURN_ON_ERROR( esp_lcd_new_dsi_bus( &bus_config, &mipi_dsi_bus ), TAG, "New DSI bus init failed" );

    ESP_LOGI( TAG, "Install MIPI DSI LCD control panel" );

    // we use DBI interface to send LCD commands and parameters
    esp_lcd_dbi_io_config_t   dbi_config = ST7123_PANEL_IO_DBI_CONFIG();
    esp_lcd_panel_io_handle_t io;
    ESP_GOTO_ON_ERROR( esp_lcd_new_panel_io_dbi( mipi_dsi_bus, &dbi_config, &io ), err, TAG, "New panel IO failed" );

    esp_lcd_panel_handle_t disp_panel = NULL;
    ESP_LOGI( TAG, "Install LCD driver of ST7123" );
    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel    = 0,
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 70, // ST7123 DPI clock frequency
        .pixel_format       = LCD_COLOR_PIXEL_FORMAT_RGB565,
        .in_color_format    = LCD_COLOR_FMT_RGB565,
        .num_fbs            = 1,
        .video_timing       = {
                               .h_size            = BSP_LCD_H_RES,
                               .v_size            = BSP_LCD_V_RES,
                               .hsync_pulse_width = 2,
                               .hsync_back_porch  = 40,
                               .hsync_front_porch = 40,
                               .vsync_pulse_width = 2,
                               .vsync_back_porch  = 8,
                               .vsync_front_porch = 220,
                               },
#if CONFIG_BSP_LCD_USE_DMA2D && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0))
        .flags = { 
		    .use_dma2d = true,
		},
#endif							   
    };
    dpi_config.num_fbs = CONFIG_BSP_LCD_DPI_BUFFER_NUMS;

    st7123_vendor_config_t vendor_config = {
        .init_cmds      = st7123_vendor_specific_init_default,
        .init_cmds_size = sizeof( st7123_vendor_specific_init_default ) / sizeof( st7123_vendor_specific_init_default[ 0 ] ),
        .mipi_config    = {
                           .dsi_bus    = mipi_dsi_bus,
                           .dpi_config = &dpi_config,
                           .lane_num   = BSP_LCD_MIPI_DSI_LANE_NUM,
                           },
    };

    const esp_lcd_panel_dev_config_t lcd_dev_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .data_endian    = LCD_RGB_DATA_ENDIAN_LITTLE,
        .rgb_ele_order  = BSP_LCD_COLOR_SPACE,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
        .vendor_config  = &vendor_config,
    };
    //sync check//
    ESP_GOTO_ON_ERROR( esp_lcd_new_panel_st7123( io, &lcd_dev_config, &disp_panel ), err, TAG, "New LCD panel failed" );
    disp_panel->reset = bsp_ext_io_lcd_reset; // use IO expander to reset LCD
#if CONFIG_BSP_LCD_USE_DMA2D && (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
    ESP_GOTO_ON_ERROR(esp_lcd_dpi_panel_enable_dma2d( disp_panel ), err, TAG, "LCD panel enable DMA2D failed");
#endif

    ESP_GOTO_ON_ERROR( esp_lcd_panel_reset( disp_panel ), err, TAG, "LCD panel reset failed" );
    ESP_GOTO_ON_ERROR( esp_lcd_panel_init( disp_panel ), err, TAG, "LCD panel init failed" );
    ESP_GOTO_ON_ERROR( esp_lcd_panel_disp_on_off( disp_panel, true ), err, TAG, "LCD panel ON failed" );

    /* Return all handles */
    ret_handles->io           = io;
    ret_handles->mipi_dsi_bus = mipi_dsi_bus;
    ret_handles->panel        = disp_panel;
    ret_handles->control      = NULL;

    ESP_LOGI( TAG, "Display initialized with resolution %dx%d", BSP_LCD_H_RES, BSP_LCD_V_RES );

    return ret;

err:
    if ( disp_panel ) {
        esp_lcd_panel_del( disp_panel );
    }
    if ( io ) {
        esp_lcd_panel_io_del( io );
    }
    if ( mipi_dsi_bus ) {
        esp_lcd_del_dsi_bus( mipi_dsi_bus );
    }

    bsp_disable_dsi_phy_power();

    bsp_display_brightness_deinit();

    return ret;
}
#endif // LCD_MIPI_DSI_USE_ST7123

static lv_display_t *bsp_display_lcd_init( const bsp_display_cfg_t *cfg )
{
    assert( cfg != NULL );
    esp_lcd_panel_io_handle_t  io_handle    = NULL;
    const bsp_display_config_t not_usedcfg = { 0 };
    BSP_ERROR_CHECK_RETURN_NULL( bsp_display_new( &not_usedcfg, &disp_handles.panel, &io_handle ) );

    //esp_lcd_panel_disp_on_off(disp_handles.panel, true);

    /* Add LCD screen */
    ESP_LOGD( TAG, "Add LCD screen" );
#ifdef USE_LVGL_ADAPTER
    esp_lv_adapter_display_config_t disp_cfg = {
        .panel    = panel_handle,
        .panel_io = io_handle,
        .profile  = {
                     .interface             = ESP_LV_ADAPTER_PANEL_IF_MIPI_DSI,
                     .rotation              = cfg->rotation,
                     .hor_res               = BSP_LCD_H_RES,
                     .ver_res               = BSP_LCD_V_RES,
                     .buffer_height         = 50,
                     .use_psram             = false,
                     .enable_ppa_accel      = false,
                     .require_double_buffer = false,
                     },
        .tear_avoid_mode = cfg->tear_avoid_mode,
    };

    lv_display_t *disp = esp_lv_adapter_register_display( &disp_cfg );
    if ( !disp ) {
        return NULL;
    }

    return disp;
#else
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle      = io_handle,
        .panel_handle   = disp_handles.panel,
        .control_handle = disp_handles.control,
        .buffer_size    = cfg->buffer_size,
        .double_buffer  = cfg->double_buffer,
        .hres           = BSP_LCD_H_RES,
        .vres           = BSP_LCD_V_RES,
        .monochrome     = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
                     .swap_xy  = false,
                     .mirror_x = false,
                     .mirror_y = false,
                     },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags        = {
                     .buff_dma    = cfg->flags.buff_dma,
                     .buff_spiram = cfg->flags.buff_spiram,
#if LVGL_VERSION_MAJOR >= 9
                     .swap_bytes = ( BSP_LCD_BIGENDIAN ? true : false ),
#endif
#if CONFIG_BSP_DISPLAY_LVGL_AVOID_TEAR
                     .sw_rotate = false, /* Avoid tearing is not supported for SW rotation */
#else
        .sw_rotate = cfg->flags.sw_rotate, /* Only SW rotation is supported for 90° and 270° */
#endif
#if CONFIG_BSP_DISPLAY_LVGL_FULL_REFRESH
        .full_refresh = true,
#elif CONFIG_BSP_DISPLAY_LVGL_DIRECT_MODE
        .direct_mode = true,
#endif
                     }
    };

    const lvgl_port_display_dsi_cfg_t dpi_cfg = {
        .flags = {
#if CONFIG_BSP_DISPLAY_LVGL_AVOID_TEAR
                  .avoid_tearing = true,
#else
        .avoid_tearing = false,
#endif
                  }
    };

    return lvgl_port_add_disp_dsi( &disp_cfg, &dpi_cfg );
#endif
}

esp_err_t bsp_touch_new( const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch )
{
    /* Initilize I2C */
    BSP_ERROR_CHECK_RETURN_ERR( bsp_i2c_init() );
    /* Enable Feature */
    //BSP_ERROR_CHECK_RETURN_ERR(bsp_feature_enable(BSP_FEATURE_TOUCH, true));

    /* Initialize touch */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max        = BSP_LCD_H_RES,
        .y_max        = BSP_LCD_V_RES,
        .rst_gpio_num = BSP_LCD_TOUCH_RST, // Usually shared with LCD reset
        .int_gpio_num = BSP_LCD_TOUCH_INT, //GPIO_NUM_23 cannot be used due to resistor to 3V3
        .levels       = {
                         .reset     = 0,
                         .interrupt = 0,
                         },
        .flags = {
                         .swap_xy  = 0,
                         .mirror_x = 0,
                         .mirror_y = 0,
                         },
    };
    i2c_master_bus_handle_t       i2c_handle   = bsp_i2c_get_handle();
    esp_lcd_panel_io_handle_t     tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_ST7123_CONFIG();
    tp_io_config.dev_addr                      = ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS;
    tp_io_config.scl_speed_hz                  = 400000;
    ESP_RETURN_ON_ERROR( esp_lcd_new_panel_io_i2c( i2c_handle, &tp_io_config, &tp_io_handle ), TAG, "" );

    ESP_RETURN_ON_ERROR( esp_lcd_touch_new_i2c_st7123( tp_io_handle, &tp_cfg, ret_touch ), TAG, "New touch driver initialization failed" );

    return ESP_OK;
}

static lv_indev_t *bsp_display_indev_touch_init( lv_display_t *disp )
{
    BSP_ERROR_CHECK_RETURN_NULL( bsp_touch_new( NULL, &_lcd_touch_handle ) );
    assert( _lcd_touch_handle );

#ifdef USE_LVGL_ADAPTER
    /* Add touch input (for selected screen) */
    const esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG( disp, _lcd_touch_handle );

    return esp_lv_adapter_register_touch( &touch_cfg );

#else
    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp   = disp,
        .handle = _lcd_touch_handle,
    };

    return lvgl_port_add_touch( &touch_cfg );
#endif
}

esp_lcd_touch_handle_t bsp_display_get_touch_handle( void )
{
    return _lcd_touch_handle;
}

static esp_err_t bsp_touch_enter_sleep( void )
{
    assert( _lcd_touch_handle );
    return esp_lcd_touch_enter_sleep( _lcd_touch_handle );
}

static esp_err_t bsp_touch_exit_sleep( void )
{
    assert( _lcd_touch_handle );
    return esp_lcd_touch_exit_sleep( _lcd_touch_handle );
}

lv_display_t *bsp_display_start( void )
{
    bsp_reset_tp();
#ifdef USE_LVGL_ADAPTER
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg  = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation        = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags     = {
                            .swap_xy  = 0,
                            .mirror_x = 0,
                            .mirror_y = 0 }
    };
#else
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size   = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags         = {
                          .buff_dma    = true,
                          .buff_spiram = true,
                          .sw_rotate   = true,
                          }
    };

    cfg.lvgl_port_cfg.task_stack = ( 1024 * 16 ); // Increase stack size for LVGL task to avoid "Stack canary watchpoint triggered" issue
#endif
    return bsp_display_start_with_config( &cfg );
}

lv_display_t *bsp_display_start_with_config( const bsp_display_cfg_t *cfg )
{
    assert( cfg != NULL );

#ifdef USE_LVGL_ADAPTER
    BSP_ERROR_CHECK_RETURN_NULL( esp_lv_adapter_init( &cfg->lv_adapter_cfg ) );
#else
    BSP_ERROR_CHECK_RETURN_NULL( lvgl_port_init( &cfg->lvgl_port_cfg ) );
#endif

    ESP_ERROR_CHECK_WITHOUT_ABORT( bsp_display_brightness_init() );

    disp = bsp_display_lcd_init( cfg );
	if ( disp == NULL ) {
        ESP_LOGE( TAG, "Hardware display init failed -> registering headless dummy display" );
        return NULL; // or headless boot implementaion
    } else {
		disp_indev = bsp_display_indev_touch_init( disp );
		if( disp_indev == NULL ) {
            ESP_LOGE( TAG, "Touch input device initialization failed" );
        }
	}
    
#ifdef USE_LVGL_ADAPTER
    ESP_ERROR_CHECK(esp_lv_adapter_start());
#endif

    return disp;
}

lv_indev_t *bsp_display_get_input_dev( void )
{
    return disp_indev;
}

void bsp_display_rotate( lv_display_t *disp, lv_disp_rotation_t rotation )
{
    lv_display_set_rotation( disp, rotation );
}

bool bsp_display_lock( uint32_t timeout_ms )
{
    return lvgl_port_lock( timeout_ms );
}

void bsp_display_unlock( void )
{
    lvgl_port_unlock();
}

static esp_err_t bsp_lcd_enter_sleep( void )
{
    assert( disp_handles.panel );
    return esp_lcd_panel_disp_on_off( disp_handles.panel, false );
}

static esp_err_t bsp_lcd_exit_sleep( void )
{
    assert( disp_handles.panel );
    return esp_lcd_panel_disp_on_off( disp_handles.panel, true );
}

esp_err_t bsp_display_enter_sleep( void )
{
    BSP_ERROR_CHECK_RETURN_ERR( bsp_lcd_enter_sleep() );
    BSP_ERROR_CHECK_RETURN_ERR( bsp_display_backlight_off() );
    BSP_ERROR_CHECK_RETURN_ERR( bsp_touch_enter_sleep() );
    return ESP_OK;
}

esp_err_t bsp_display_exit_sleep( void )
{
    BSP_ERROR_CHECK_RETURN_ERR( bsp_lcd_exit_sleep() );
    BSP_ERROR_CHECK_RETURN_ERR( bsp_display_backlight_on() );
    BSP_ERROR_CHECK_RETURN_ERR( bsp_touch_exit_sleep() );
    return ESP_OK;
}
#endif // (BSP_CONFIG_NO_GRAPHIC_LIB == 0)

//==================================================================================
// camera
//==================================================================================
#define BSP_CAM_FREQ_HZ 24000000 // 24MHz
esp_err_t bsp_cam_osc_init( void )
{
    ledc_timer_config_t timer_conf;
    timer_conf.duty_resolution = LEDC_TIMER_1_BIT;
    timer_conf.freq_hz         = BSP_CAM_FREQ_HZ; // <<<< change this to the frequency you want
    timer_conf.speed_mode      = LEDC_LOW_SPEED_MODE;
    timer_conf.deconfigure     = false;
    timer_conf.clk_cfg         = LEDC_AUTO_CLK;
    timer_conf.timer_num       = LEDC_TIMER_0;
    esp_err_t err              = ledc_timer_config( &timer_conf );
    if ( err != ESP_OK ) {
        ESP_LOGE( TAG, "ledc_timer_config failed for freq %d, rc=%x", BSP_CAM_FREQ_HZ, err );
    }

    ledc_channel_config_t ch_conf;
    ch_conf.gpio_num   = BSP_CAM_MCLK_GPIO; // Camera clock input
    ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_conf.channel    = LEDC_CHANNEL_0;
    ch_conf.intr_type  = LEDC_INTR_DISABLE;
    ch_conf.timer_sel  = LEDC_TIMER_0;
    ch_conf.duty       = 1;
    ch_conf.hpoint     = 0;
    ch_conf.sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE;
    err                = ledc_channel_config( &ch_conf );
    if ( err != ESP_OK ) {
        ESP_LOGE( TAG, "ledc_channel_config failed, rc=%x", err );
    }

    return ESP_OK;
}


esp_err_t bsp_camera_start(const bsp_camera_cfg_t *cfg)
{

   i2c_master_bus_handle_t   i2c_handle   = bsp_i2c_get_handle();
    /* Enable Feature */
    bsp_set_camera_enable(true);//BSP_ERROR_CHECK_RETURN_ERR(bsp_feature_enable(BSP_FEATURE_CAMERA, true));
    vTaskDelay(pdMS_TO_TICKS(100));

    const esp_video_init_csi_config_t base_csi_config = {
        .sccb_config = {
            .init_sccb = false,
            .i2c_handle = i2c_handle,
            .freq = 400000,
        },
        .reset_pin = BSP_CAMERA_RST,
        .pwdn_pin  = -1,
    };

    esp_video_init_config_t cam_config = {
        .csi      = &base_csi_config,
    };

    return esp_video_init(&cam_config);
}


#if 0
/* Feature enable */
esp_err_t bsp_feature_enable(bsp_feature_t feature, bool enable)
{
    esp_err_t ret = ESP_OK;

    switch (feature) {
    case BSP_FEATURE_LCD: {
        bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_LCD_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_LCD_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_LCD_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_TOUCH: {
        bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_TOUCH_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_TOUCH_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_TOUCH_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_SPEAKER: {
        bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_SPEAKER_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_SPEAKER_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_SPEAKER_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_CAMERA: {
        bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_CAMERA_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_CAMERA_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_CAMERA_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_USB: {
        bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander1, BSP_USB_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander1, BSP_USB_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander1, BSP_USB_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_WIFI: {
        bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander1, BSP_WIFI_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander1, BSP_WIFI_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander1, BSP_WIFI_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    }

    return ret;
}
#endif
//==================================================================================
// usb
//==================================================================================
#if BSP_USE_USB_HOST
// USB Host Library task
#include "freertos/task.h"
#include "usb/usb_host.h"

static TaskHandle_t usb_host_task; // USB Host Library task
static void usb_lib_task( void *arg )
{
    while ( 1 ) {
        // Start handling system events
        uint32_t event_flags;
        usb_host_lib_handle_events( portMAX_DELAY, &event_flags );
        if ( event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS ) {
            ESP_ERROR_CHECK( usb_host_device_free_all() );
        }
        if ( event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE ) {
            ESP_LOGI( TAG, "USB: All devices freed" );
            // Continue handling USB events to allow device reconnection
            // The only way this task can be stopped is by calling bsp_usb_host_stop()
        }
    }
}

esp_err_t bsp_usb_host_start( bsp_usb_host_power_mode_t mode, bool limit_500mA )
{
    /* Enable Feature */
    //BSP_ERROR_CHECK_RETURN_ERR(bsp_feature_enable(BSP_FEATURE_USB, true));
    //Install USB Host driver. Should only be called once in entire application
    ESP_LOGI( TAG, "Installing USB Host" );
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags     = ESP_INTR_FLAG_LEVEL1,
    };
    BSP_ERROR_CHECK_RETURN_ERR( usb_host_install( &host_config ) );

    // Create a task that will handle USB library events
    if ( xTaskCreate( usb_lib_task, "usb_lib", 4096, NULL, 10, &usb_host_task ) != pdTRUE ) {
        ESP_LOGE( TAG, "Creating USB host lib task failed" );
        abort();
    }

    return ESP_OK;
}

esp_err_t bsp_usb_host_stop( void )
{
    usb_host_uninstall();
    if ( usb_host_task ) {
        vTaskSuspend( usb_host_task );
        vTaskDelete( usb_host_task );
    }
    return ESP_OK;
}
#endif


esp_err_t bsp_set_speaker_enable(bool enable)
{   
    esp_err_t ret;
    
    ret = bsp_io_expander1_set_bit( IO_EXPANDER1_SPK_EN, enable ? 1 : 0 );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set speaker enable: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Speaker %s (P1=%d)", enable ? "enabled" : "disabled", enable ? 1 : 0);
    return ESP_OK;
}
