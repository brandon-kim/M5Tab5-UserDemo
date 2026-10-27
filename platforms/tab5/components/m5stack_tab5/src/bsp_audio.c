/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_check.h"
#include "bsp_err_check.h"
#include "esp_codec_dev_defaults.h"
#include "bsp/m5stack_tab5.h"

static const char *TAG = "M5Stack Tab5";

/* Can be used for i2s_std_gpio_config_t and/or i2s_std_config_t initialization */
#define BSP_I2S_GPIO_CFG       \
    {                          \
        .mclk = BSP_I2S_MCLK,  \
        .bclk = BSP_I2S_SCLK,  \
        .ws = BSP_I2S_LCLK,    \
        .dout = BSP_I2S_DOUT,  \
        .din = BSP_I2S_DSIN,   \
        .invert_flags = {      \
            .mclk_inv = false, \
            .bclk_inv = false, \
            .ws_inv = false,   \
        },                     \
    }

/* This configuration is used by default in bsp_audio_init() */
#define BSP_I2S_DUPLEX_CFG(_sample_rate)                                                         \
    {                                                                                                 \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                          \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), \
        .gpio_cfg = BSP_I2S_GPIO_CFG,                                                                 \
    }
static i2s_chan_handle_t i2s_tx_chan = NULL;
static i2s_chan_handle_t i2s_rx_chan = NULL;
static const audio_codec_data_if_t *i2s_data_if = NULL;  /* Codec data interface */

esp_err_t bsp_audio_init(const i2s_std_config_t *i2s_config)
{
    esp_err_t ret = ESP_FAIL;
    if (i2s_tx_chan && i2s_rx_chan) {
        /* Audio was initialized before */
        return ESP_OK;
    }

    /* Setup I2S peripheral */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(CONFIG_BSP_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    BSP_ERROR_CHECK_RETURN_ERR(i2s_new_channel(&chan_cfg, &i2s_tx_chan, &i2s_rx_chan));

    /* Setup I2S channels */
    const i2s_std_config_t std_cfg_default = BSP_I2S_DUPLEX_CFG(48000);
    const i2s_std_config_t *p_i2s_cfg = &std_cfg_default;
    if (i2s_config != NULL) {
        p_i2s_cfg = i2s_config;
    }
    if (i2s_tx_chan != NULL) {
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_tx_chan, p_i2s_cfg), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_tx_chan), err, TAG, "I2S enabling failed");
    }
    if (i2s_rx_chan != NULL) {
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_rx_chan, p_i2s_cfg), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_rx_chan), err, TAG, "I2S enabling failed");
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = CONFIG_BSP_I2S_NUM,
        .rx_handle = i2s_rx_chan,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    BSP_NULL_CHECK_GOTO(i2s_data_if, err);

    return ESP_OK;

err:
    if (i2s_tx_chan) {
        i2s_del_channel(i2s_tx_chan);
        i2s_tx_chan = NULL;
    }
    if (i2s_rx_chan) {
        i2s_del_channel(i2s_rx_chan);
        i2s_rx_chan = NULL;
    }

    return ret;
}

const audio_codec_data_if_t *bsp_audio_get_codec_itf(void)
{
    return i2s_data_if;
}

esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    const audio_codec_data_if_t *i2s_data_if = bsp_audio_get_codec_itf();
    if (i2s_data_if == NULL) {

        /* Initilize I2C */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2c_init());
        /* Configure I2S peripheral and Power Amplifier */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_audio_init(NULL));
        i2s_data_if = bsp_audio_get_codec_itf();
    }
    assert(i2s_data_if);
    /* Enable Feature */
    BSP_ERROR_CHECK_RETURN_NULL(bsp_feature_enable(BSP_FEATURE_SPEAKER, true));

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES8388_CODEC_DEFAULT_ADDR,
        .bus_handle = bsp_i2c_get_handle(),
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    BSP_NULL_CHECK(i2c_ctrl_if, NULL);

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };

    es8388_codec_cfg_t codec_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = BSP_POWER_AMP_IO,
        .pa_reverted = false,
        .master_mode = false,
        .hw_gain = gain,
    };
    const audio_codec_if_t *dev = es8388_codec_new(&codec_cfg);
    BSP_NULL_CHECK(dev, NULL);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_dev_cfg);
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    const audio_codec_data_if_t *i2s_data_if = bsp_audio_get_codec_itf();
    if (i2s_data_if == NULL) {

        /* Initilize I2C */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2c_init());
        /* Configure I2S peripheral and Power Amplifier */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_audio_init(NULL));
        i2s_data_if = bsp_audio_get_codec_itf();
    }
    assert(i2s_data_if);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES7210_CODEC_DEFAULT_ADDR,
        .bus_handle = bsp_i2c_get_handle(),
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    BSP_NULL_CHECK(i2c_ctrl_if, NULL);

    es7210_codec_cfg_t codec_cfg = {
        .ctrl_if = i2c_ctrl_if,
    };
    const audio_codec_if_t *dev = es7210_codec_new(&codec_cfg);
    BSP_NULL_CHECK(dev, NULL);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_dev_cfg);
}

#if 1//M5Tab5-UserDemo
static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;
static bsp_codec_config_t     g_codec_handle;
static int                    volume;

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

static esp_err_t bsp_codec_es8388_set( uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t mode )
{
    esp_err_t ret = ESP_OK;

    uint8_t ch = 0;
    switch ( mode ) {
        case I2S_SLOT_MODE_MONO:
            ch = 1;
            break;
        case I2S_SLOT_MODE_STEREO:
            ch = 2;
            break;
        default:
            ch = 2;
            break;
    }

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

static esp_err_t bsp_codec_es7210_set( uint32_t rate, uint32_t bps, i2s_slot_mode_t mode )
{
    esp_err_t ret = ESP_OK;
    uint8_t ch = 0;
    uint8_t ch_mask = 0;
    // 1,2 on device, 3,4 headphone jack 
    switch ( mode ) {
        case I2S_SLOT_MODE_MONO:
            ch = 2;
            ch_mask = 0x03;
            break;
        case I2S_SLOT_MODE_STEREO:
            ch = 4;
            ch_mask = 0x0F;
            break;
        default:
            ch = 4;
            ch_mask = 0x0F;
            break;
    }

    esp_codec_dev_sample_info_t fs = {
        .sample_rate     = rate,
        .channel         = ch,
        .channel_mask    = ch_mask,
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
    bsp_codec_es7210_set( BSP_I2S_SAMPLE_RATE, 16, I2S_SLOT_MODE_STEREO );
    bsp_codec_es8388_set( BSP_I2S_SAMPLE_RATE, 16, I2S_SLOT_MODE_STEREO );

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

#endif