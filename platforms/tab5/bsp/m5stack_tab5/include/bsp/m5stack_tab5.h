/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP BSP: m5stack_tab5
 */

#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/sdmmc_host.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"

#include "bsp/config.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "esp_codec_dev.h"

//#define USE_LVGL_ADAPTER   1 //ready for change

#if ( BSP_CONFIG_NO_GRAPHIC_LIB == 0 )
#include "lvgl.h"
#ifdef USE_LVGL_ADAPTER
#include "esp_lv_adapter.h"
#else
#include "esp_lvgl_port.h"
#endif
#endif // BSP_CONFIG_NO_GRAPHIC_LIB == 0

/*Definations of Board*/
#define BOARD_NAME   "Tab5"
#define BOARD_VENDOR "M5"
#define BOARD_URL    "null"
/**************************************************************************************************
 *  BSP Board Name
 **************************************************************************************************/
/** @defgroup boardname Board Name
 *  @brief BSP Board Name
 *  @{
 */
#define BSP_BOARD_M5STACK_TAB5
/** @} */ // end of boardname

/**************************************************************************************************
 *  BSP Capabilities
 **************************************************************************************************/

/** @defgroup capabilities Capabilities
 *  @brief BSP Capabilities
 *  @{
 */
#define BSP_CAPS_DISPLAY       1
#define BSP_CAPS_TOUCH         1
#define BSP_CAPS_BUTTONS       0
#define BSP_CAPS_KNOB          0
#define BSP_CAPS_AUDIO         1
#define BSP_CAPS_AUDIO_SPEAKER 1
#define BSP_CAPS_AUDIO_MIC     1
#define BSP_CAPS_SDCARD        1
#define BSP_CAPS_LED           0
#define BSP_CAPS_CAMERA        1
#define BSP_CAPS_BAT           0
#define BSP_CAPS_IMU           0
#define BSP_CAPS_USB_HOST      1
/** @} */ // end of capabilities

/**************************************************************************************************
 *  Board pinout
 **************************************************************************************************/
/** @defgroup g01_i2c I2C
 *  @brief I2C BSP API
 *  @{
 */
/* SYS I2C */
#define BSP_I2C_NUM          0
#define BSP_I2C_CLK_SPEED_HZ 400000
#define BSP_I2C_SCL          ( GPIO_NUM_32 )
#define BSP_I2C_SDA          ( GPIO_NUM_31 )

/* EXT I2C */
#define BSP_EXT_I2C_NUM 1
#define BSP_EXT_I2C_SCL ( GPIO_NUM_54 )
#define BSP_EXT_I2C_SDA ( GPIO_NUM_53 )

/* io expander pin mask 
 Using interrupts with the esp_io_expander is not appropriate. Control it directly via I2C. */
#include "esp_bit_defs.h"
#define IO_EXPANDER_PIN_NUM_0           BIT0
#define IO_EXPANDER_PIN_NUM_1           BIT1
#define IO_EXPANDER_PIN_NUM_2           BIT2
#define IO_EXPANDER_PIN_NUM_3           BIT3
#define IO_EXPANDER_PIN_NUM_4           BIT4
#define IO_EXPANDER_PIN_NUM_5           BIT5
#define IO_EXPANDER_PIN_NUM_6           BIT6
#define IO_EXPANDER_PIN_NUM_7           BIT7

#define IO_EXPANDER1_RF_PTH_L_INT_H_EXT ( IO_EXPANDER_PIN_NUM_0 ) /* output */  
#define IO_EXPANDER1_SPK_EN             ( IO_EXPANDER_PIN_NUM_1 ) /* output */
#define IO_EXPANDER1_EXT5V_EN           ( IO_EXPANDER_PIN_NUM_2 ) /* output */

#define IO_EXPANDER1_LCD_RST            ( IO_EXPANDER_PIN_NUM_4 ) /* output */
#define IO_EXPANDER1_TP_RST             ( IO_EXPANDER_PIN_NUM_5 ) /* output */
#define IO_EXPANDER1_CAM_RST            ( IO_EXPANDER_PIN_NUM_6 ) /* output */
#define IO_EXPANDER1_HP_DETECT          ( IO_EXPANDER_PIN_NUM_7 ) /* input */

#define IO_EXPANDER2_WLAN_PWR_EN        ( IO_EXPANDER_PIN_NUM_0 ) /* output */

#define IO_EXPANDER2_USB5V_EN           ( IO_EXPANDER_PIN_NUM_3 ) /* output */
#define IO_EXPANDER2_PWEROFF_PULSE      ( IO_EXPANDER_PIN_NUM_4 ) /* output */
#define IO_EXPANDER2_nCHG_QC_EN         ( IO_EXPANDER_PIN_NUM_5 ) /* output */
#define IO_EXPANDER2_CHG_STAT           ( IO_EXPANDER_PIN_NUM_6 ) /* input */
#define IO_EXPANDER2_CHG_EN             ( IO_EXPANDER_PIN_NUM_7 ) /* output */

/** @} */ // end of i2c

/** @defgroup g03_audio Audio
 *  @brief Audio BSP API
 *  @{
 */
#define BSP_I2S_SCLK     ( GPIO_NUM_27 )
#define BSP_I2S_MCLK     ( GPIO_NUM_30 )
#define BSP_I2S_LCLK     ( GPIO_NUM_29 )
#define BSP_I2S_DOUT     ( GPIO_NUM_26 )
#define BSP_I2S_DSIN     ( GPIO_NUM_28 )
#define BSP_POWER_AMP_IO ( GPIO_NUM_NC )
#define BSP_SPEAKER_EN   ( IO_EXPANDER1_SPK_EN )
#define BSP_I2S_SAMPLE_RATE   ( 48000 )

/** @} */ // end of audio

/** @defgroup g04_display Display and Touch
 *  @brief Display BSP API
 *  @{
 */
#define BSP_LCD_BACKLIGHT  ( GPIO_NUM_22 )
#define BSP_LCD_RST        ( GPIO_NUM_NC )        // (IO_EXPANDER_PIN_NUM_4)
#define BSP_LCD_RST_EXT_IO IO_EXPANDER1_LCD_RST   // (IO_EXPANDER_PIN_NUM_4 )
#define BSP_LCD_TOUCH_RST  ( GPIO_NUM_NC ) // IO Exanpder
#define BSP_LCD_TOUCH_INT  ( GPIO_NUM_23 ) // 23
#define BSP_TOUCH_EN       ( GPIO_NUM_NC ) //(IO_EXPANDER_PIN_NUM_5)
/** @} */ // end of display

/** @defgroup g07_usb USB
 *  @brief USB BSP API
 *  @{
 */
#define BSP_USB_EN            IO_EXPANDER2_USB5V_EN   // ( IO_EXPANDER_PIN_NUM_3 )
#define BSP_USB_POS ( GPIO_NUM_20 )
#define BSP_USB_NEG ( GPIO_NUM_19 )
/** @} */ // end of usb

/** @defgroup g12_camera Camera
 *  @brief Camera BSP API
 *  @{
 */
#define BSP_CAM_MCLK_GPIO    ( GPIO_NUM_36 )
#define BSP_CAMERA_GPIO_XCLK ( GPIO_NUM_NC )
#define BSP_CAMERA_RST       ( GPIO_NUM_NC )
#define BSP_CAMERA_EN        IO_EXPANDER1_CAM_RST  //( IO_EXPANDER_PIN_NUM_6 )
/** @} */ // end of camera

/** @defgroup g02_storage SD Card 
 *  @brief SD card BSP API
 *  @{
 */
#define BSP_SD_BUS_WIDTH ( 4 )
#define BSP_SD_D0        ( GPIO_NUM_39 )
#define BSP_SD_D1        ( GPIO_NUM_40 )
#define BSP_SD_D2        ( GPIO_NUM_41 )
#define BSP_SD_D3        ( GPIO_NUM_42 )
#define BSP_SD_CLK       ( GPIO_NUM_43 )
#define BSP_SD_CMD       ( GPIO_NUM_44 )
#define BSP_SD_DET       ( GPIO_NUM_NC )

/** @} */ // end of storage
#define BSP_WIFI_EN      IO_EXPANDER2_WLAN_PWR_EN  //  (IO_EXPANDER_PIN_NUM_0 )

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
 *
 **************************************************************************************************/
extern char *bsp_get_info( void );
/** \addtogroup g01_i2c
 *  @{
 */

extern esp_err_t bsp_gpio_init( void );
/**************************************************************************************************
 *
 * I2C interface
 *
 * There are multiple devices connected to I2C peripheral:
 *  - Codec ES8311 (configuration only)
 *  - LCD Touch controller
 **************************************************************************************************/

/**
 * @brief Init I2C driver
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   I2C parameter error
 *      - ESP_FAIL              I2C driver installation error
 *
 */
esp_err_t bsp_i2c_init( void );

/**
 * @brief Deinit I2C driver and free its resources
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   I2C parameter error
 *
 */
esp_err_t bsp_i2c_deinit( void );

/**
 * @brief Get I2C driver handle
 *
 * @return
 *      - I2C handle
 */
i2c_master_bus_handle_t bsp_i2c_get_handle( void );

esp_err_t               bsp_ext_i2c_init( void );
esp_err_t               bsp_ext_i2c_deinit( void );
i2c_master_bus_handle_t bsp_ext_i2c_get_handle( void );
#if 0
esp_err_t bsp_grove_i2c_init(void);
esp_err_t bsp_grove_i2c_deinit(void);
i2c_master_bus_handle_t bsp_grove_i2c_get_handle(void);
#endif
esp_err_t bsp_i2c_scan( i2c_master_bus_handle_t i2c_handle );

/** @} */ // end of i2c

/** \addtogroup g03_audio
 *  @{
 */

/**************************************************************************************************
 *
 * I2S audio interface
 *
 * There are two devices connected to the I2S peripheral:
 *  - Codec ES8311 for output(playback) and input(recording) path
 *
 * For speaker initialization use bsp_audio_codec_speaker_init() which is inside initialize I2S with bsp_codec_i2sdata_init().
 * For microphone initialization use bsp_audio_codec_microphone_init() which is inside initialize I2S with bsp_codec_i2sdata_init(). 
 * After speaker or microphone initialization, use functions from esp_codec_dev for play/record audio.
 * Example audio play:
 * \code{.c}
 * esp_codec_dev_set_out_vol(spk_codec_dev, DEFAULT_VOLUME);
 * esp_codec_dev_open(spk_codec_dev, &fs);
 * esp_codec_dev_write(spk_codec_dev, wav_bytes, bytes_read_from_spiffs);
 * esp_codec_dev_close(spk_codec_dev);
 * \endcode
 **************************************************************************************************/
#define BSP_I2S_NUM 1
/**
 * @brief Init audio
 *
 * @note There is no deinit audio function. Users can free audio resources by calling i2s_del_channel()
 * @warning The type of i2s_config param is depending on IDF version.
 * @param[in]  i2s_config I2S configuration. Pass NULL to use default values (Mono, duplex, 16bit, 22050 Hz)
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_NOT_SUPPORTED The communication mode is not supported on the current chip
 *      - ESP_ERR_INVALID_ARG   NULL pointer or invalid configuration
 *      - ESP_ERR_NOT_FOUND     No available I2S channel found
 *      - ESP_ERR_NO_MEM        No memory for storing the channel information
 *      - ESP_ERR_INVALID_STATE This channel has not initialized or already started
 */
esp_err_t bsp_codec_i2sdata_init( const i2s_std_config_t *i2s_config );

/**
 * @brief Get codec I2S interface (initialized in bsp_codec_i2sdata_init)
 *
 * @return
 *      - Pointer to codec I2S interface handle or NULL when error occurred
 */
const audio_codec_data_if_t *bsp_audio_get_codec_itf( void );

/**
 * @brief Initialize speaker codec device
 *
 * @return Pointer to codec device handle or NULL when error occurred
 */
esp_codec_dev_handle_t bsp_audio_codec_speaker_init( void );

/**
 * @brief Initialize microphone codec device
 *
 * @return Pointer to codec device handle or NULL when error occurred
 */
esp_codec_dev_handle_t bsp_audio_codec_microphone_init( void );

typedef esp_err_t ( *bsp_i2s_read_fn )( void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms );
typedef esp_err_t ( *bsp_i2s_write_fn )( void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms );
typedef esp_err_t ( *bsp_codec_set_in_gain_fn )( float gain );
typedef esp_err_t ( *bsp_codec_mute_fn )( bool enable );
typedef int ( *bsp_codec_volume_fn )( int volume );
typedef esp_err_t ( *bsp_codec_get_volume_fn )( void );
typedef esp_err_t ( *bsp_codec_reconfig_fn )( uint32_t rate, uint32_t bps, i2s_slot_mode_t ch );
typedef esp_err_t ( *bsp_i2s_reconfig_clk_fn )( uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch );

typedef struct {
    bsp_i2s_read_fn          i2s_read;
    bsp_i2s_write_fn         i2s_write;
    bsp_codec_mute_fn        set_mute;
    bsp_codec_volume_fn      set_volume;
    bsp_codec_get_volume_fn  get_volume;
    bsp_codec_set_in_gain_fn set_in_gain;
    bsp_codec_reconfig_fn    codec_reconfig_fn;
    bsp_i2s_reconfig_clk_fn  i2s_reconfig_clk_fn;
} bsp_codec_config_t;



/**
 * @brief Initialize codec play and record handle.
 *
 * @return
 */
void                bsp_codec_init( void );
bsp_codec_config_t *bsp_get_codec_handle( void );
uint8_t             bsp_codec_feed_channel( void );

/** @} */ // end of audio



/** \addtogroup g02_storage
 *  @{
 */

/**************************************************************************************************
 *
 * SPIFFS
 *
 * After mounting the SPIFFS, it can be accessed with stdio functions ie.:
 * \code{.c}
 * FILE* f = fopen(BSP_SPIFFS_MOUNT_POINT"/hello.txt", "w");
 * fprintf(f, "Hello World!\n");
 * fclose(f);
 * \endcode
 **************************************************************************************************/
#define BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL ( true )
#define BSP_SPIFFS_MOUNT_POINT          "/spiffs"
#define BSP_SPIFFS_PARTITION_LABEL      "storage"
#define BSP_SPIFFS_MAX_FILES            5

/**
 * @brief Mount SPIFFS to virtual file system
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if esp_vfs_spiffs_register was already called
 *      - ESP_ERR_NO_MEM if memory can not be allocated
 *      - ESP_FAIL if partition can not be mounted
 *      - other error codes
 */
esp_err_t bsp_spiffs_mount(void);

/**
 * @brief Unmount SPIFFS from virtual file system
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_FOUND if the partition table does not contain SPIFFS partition with given label
 *      - ESP_ERR_INVALID_STATE if esp_vfs_spiffs_unregister was already called
 *      - ESP_ERR_NO_MEM if memory can not be allocated
 *      - ESP_FAIL if partition can not be mounted
 *      - other error codes
 */
esp_err_t bsp_spiffs_unmount(void);

/**************************************************************************************************
 *
 * SD card
 *
 * After mounting the SD card, it can be accessed with stdio functions ie.:
 * \code{.c}
 * FILE* f = fopen(BSP_SD_MOUNT_POINT"/hello.txt", "w");
 * fprintf(f, "Hello %s!\n", bsp_sdcard->cid.name);
 * fclose(f);
 * \endcode
 *
 **************************************************************************************************/
#define BSP_SD_MOUNT_POINT          "/sdcard"
#define BSP_SD_MAX_OPENED_FILES     10
#define BSP_SD_FORMAT_ON_MOUNT_FAIL ( false )
/**
 * @brief Mount microSD card to virtual file system
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if esp_vfs_fat_sdmmc_mount was already called
 *      - ESP_ERR_NO_MEM if memory can not be allocated
 *      - ESP_FAIL if partition can not be mounted
 *      - other error codes from SDMMC or SPI drivers, SDMMC protocol, or FATFS drivers
 */
/**
 * @brief Init SD crad
 *
 * @param mount_point Path where partition should be registered (e.g. "/sdcard")
 * @param max_files Maximum number of files which can be open at the same time
 * @return
 *    - ESP_OK                  Success
 *    - ESP_ERR_INVALID_STATE   If esp_vfs_fat_register was already called
 *    - ESP_ERR_NOT_SUPPORTED   If dev board not has SDMMC/SDSPI
 *    - ESP_ERR_NO_MEM          If not enough memory or too many VFSes already registered
 *    - Others                  Fail
 */
esp_err_t bsp_sdcard_init( const char *mount_point, size_t max_files );

/**
 * @brief Deinit SD card
 *
 * @param mount_point Path where partition was registered (e.g. "/sdcard")
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_sdcard_deinit( const char *mount_point );

esp_err_t bsp_sdcard_mount( void );

/**
 * @brief Unmount micorSD card from virtual file system
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_FOUND if the partition table does not contain FATFS partition with given label
 *      - ESP_ERR_INVALID_STATE if esp_vfs_fat_spiflash_mount was already called
 *      - ESP_ERR_NO_MEM if memory can not be allocated
 *      - ESP_FAIL if partition can not be mounted
 *      - other error codes from wear levelling library, SPI flash driver, or FATFS drivers
 */
esp_err_t bsp_sdcard_unmount( void );

/**
 * @brief Get SD card handle
 *
 * @return SD card handle
 */
sdmmc_card_t *bsp_sdcard_get_handle( void );

/** @} */ // end of storage

/** \addtogroup g04_display
 *  @{
 */

/**************************************************************************************************
 *
 * LCD interface
 *
 *
 * LVGL is used as graphics library. LVGL is NOT thread safe, therefore the user must take LVGL mutex
 * by calling bsp_display_lock() before calling and LVGL API (lv_...) and then give the mutex with
 * bsp_display_unlock().
 *
 * Display's backlight must be enabled explicitly by calling bsp_display_backlight_on()
 **************************************************************************************************/
#define BSP_LCD_PIXEL_CLOCK_MHZ ( 80 )

#if ( BSP_CONFIG_NO_GRAPHIC_LIB == 0 )

#define BSP_DISPLAY_BRIGHTNESS_LEDC_CH  LEDC_CHANNEL_1

#define BSP_LCD_DRAW_BUF_HEIGHT     50
#define BSP_LCD_DRAW_BUFF_SIZE      ( BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT )
#define BSP_LCD_DRAW_BUFF_DOUBLE    1

//"Select LVGL buffer mode"  depends on CONFIG_BSP_DISPLAY_LVGL_AVOID_TEAR
//#define BSP_DISPLAY_LVGL_FULL_REFRESH   0
//#define BSP_DISPLAY_LVGL_DIRECT_MODE    1

// other task priroity and stack size -> app_config.h
#define  BSP_LCD_LVGL_TASK_AFFINITY     (-1)    /* LVGL port default : -1 */
#define  BSP_LCD_LVGL_TASK_PRIORITY     4       /* LVGL port default : 4 */
#define  BSP_LCD_LVGL_TASK_STACK       (1024*16)  /* LVGL port default : 7168 */

/**
 * @brief BSP display configuration structure
 *
 */
#ifdef USE_LVGL_ADAPTER
typedef struct {
    esp_lv_adapter_config_t          lv_adapter_cfg;
    esp_lv_adapter_rotation_t        rotation;
    esp_lv_adapter_tear_avoid_mode_t tear_avoid_mode;
    struct {
        unsigned int swap_xy;  /*!< Swap X and Y after read coordinates */
        unsigned int mirror_x; /*!< Mirror X after read coordinates */
        unsigned int mirror_y; /*!< Mirror Y after read coordinates */
    } touch_flags;
} bsp_display_cfg_t;
#else 
typedef struct {
    lvgl_port_cfg_t lvgl_port_cfg; /*!< LVGL port configuration */
    uint32_t        buffer_size;   /*!< Size of the buffer for the screen in pixels */
    bool            double_buffer; /*!< True, if should be allocated two buffers */
    struct {
        unsigned int buff_dma : 1;    /*!< Allocated LVGL buffer will be DMA capable */
        unsigned int buff_spiram : 1; /*!< Allocated LVGL buffer will be in PSRAM */
        unsigned int sw_rotate : 1;   /*!< Use software rotation (slower), The feature is unavailable under avoid-tear mode */
    } flags;
} bsp_display_cfg_t;
#endif
/**
 * @brief Initialize display
 *
 * This function initializes SPI, display controller and starts LVGL handling task.
 * LCD backlight must be enabled separately by calling bsp_display_brightness_set()
 *
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_display_t *bsp_display_start( void );

/**
 * @brief Initialize display
 *
 * This function initializes SPI, display controller and starts LVGL handling task.
 * LCD backlight must be enabled separately by calling bsp_display_brightness_set()
 *
 * @param cfg display configuration
 *
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_display_t *bsp_display_start_with_config( const bsp_display_cfg_t *cfg );


lv_display_t *bsp_display_get_handle( void );
/**
 * @brief Get pointer to input device (touch, buttons, ...)
 *
 * @note The LVGL input device is initialized in bsp_display_start() function.
 *
 * @return Pointer to LVGL input device or NULL when not initialized
 */
lv_indev_t *bsp_display_get_input_dev( void );

/**
 * @brief Take LVGL mutex
 *
 * @param timeout_ms Timeout in [ms]. 0 will block indefinitely.
 * @return true  Mutex was taken
 * @return false Mutex was NOT taken
 */
bool bsp_display_lock( uint32_t timeout_ms );

/**
 * @brief Give LVGL mutex
 *
 */
void bsp_display_unlock( void );


esp_lcd_touch_handle_t bsp_display_get_touch_handle( void );

/**
 * @brief Set display enter sleep mode
 *
 * All the display (LCD, backlight, touch) will enter sleep mode.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
esp_err_t bsp_display_enter_sleep( void );

/**
 * @brief Set display exit sleep mode
 *
 * All the display (LCD, backlight, touch) will exit sleep mode.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
esp_err_t bsp_display_exit_sleep( void );

/**
 * @brief Rotate screen
 *
 * Display must be already initialized by calling bsp_display_start()
 *
 * @param[in] disp Pointer to LVGL display
 * @param[in] rotation Angle of the display rotation
 */
void bsp_display_rotate( lv_display_t *disp, lv_disp_rotation_t rotation );


#endif // BSP_CONFIG_NO_GRAPHIC_LIB == 0

/** @} */ // end of display

/** @addtogroup g12_camera
 *  @{
 */

/**************************************************************************************************
 *
 * Camera interface
 * Supported camera sensors: SC202CS
 * More information in display_camera_csi example
 *
 **************************************************************************************************/

esp_err_t bsp_cam_osc_init( void );


#define BSP_CAMERA_DEVICE   ( ESP_VIDEO_MIPI_CSI_DEVICE_NAME )
#define BSP_CAMERA_ROTATION ( 270 )

/**
 * @brief BSP camera configuration structure (for future use)
 *
 */
typedef struct {
    uint8_t dummy;
} bsp_camera_cfg_t;

/**
 * @brief Initialize camera
 *
 * Camera sensor initialization.
 */
esp_err_t bsp_camera_start(const bsp_camera_cfg_t *cfg);

/** @} */ // end of camera

/** @defgroup g99_others Others
 *  @brief Other BSP API
 *  @{
 */

/** @} */ // end of others

/** @defgroup g99_others Others
 *  @brief Other BSP API
 *  @{
 */

void bsp_io_expander_pi4ioe_init( i2c_master_bus_handle_t bus_handle );

void bsp_set_charge_qc_en( bool en );

void bsp_set_charge_en( bool en );

void bsp_set_usb_5v_en( bool en );

void bsp_set_ext_5v_en( bool en );

void bsp_generate_poweroff_signal();

bool bsp_headphone_detect();

void bsp_set_ext_antenna_enable( bool en );

void bsp_set_wifi_power_enable( bool en );

void bsp_reset_tp( void );

bool bsp_usb_c_detect( void );

void bsp_set_camera_enable( bool en );

/** \addtogroup g07_usb
 *  @{
 */

/**************************************************************************************************
 *
 * USB
 *
 **************************************************************************************************/

/**
 * @brief Power modes of USB Host connector
 */
typedef enum bsp_usb_host_power_mode_t {
    BSP_USB_HOST_POWER_MODE_USB_DEV, //!< Power from USB DEV port
} bsp_usb_host_power_mode_t;

/**
 * @brief Start USB host
 *
 * This is a one-stop-shop function that will configure the board for USB Host mode
 * and start USB Host library
 *
 * @param[in] mode        USB Host connector power mode (Not used on this board)
 * @param[in] limit_500mA Limit output current to 500mA (Not used on this board)
 * @return
 *     - ESP_OK                 On success
 *     - ESP_ERR_INVALID_ARG    Parameter error
 *     - ESP_ERR_NO_MEM         Memory cannot be allocated
 */
esp_err_t bsp_usb_host_start( bsp_usb_host_power_mode_t mode, bool limit_500mA );

/**
 * @brief Stop USB host
 *
 * USB Host lib will be uninstalled and power from connector removed.
 *
 * @return
 *     - ESP_OK              On success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t bsp_usb_host_stop( void );

/** @} */ // end of usb


esp_err_t bsp_set_speaker_enable(bool enable);

#ifdef __cplusplus
}
#endif
