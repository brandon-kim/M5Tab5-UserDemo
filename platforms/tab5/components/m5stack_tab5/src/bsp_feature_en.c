/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_check.h"
#include "driver/gpio.h"

#include "bsp/m5stack_tab5.h"

#include "esp_io_expander.h"

//extern esp_io_expander_handle_t io_expander;
//extern esp_io_expander_handle_t io_expander1; 

static const char *TAG = "M5Stack Tab5";
/* Feature enable */
esp_err_t bsp_feature_enable(bsp_feature_t feature, bool enable)
{
    esp_err_t ret = ESP_OK;

    switch (feature) {
    case BSP_FEATURE_LCD: {
        esp_io_expander_handle_t io_expander = bsp_io_expander_init();
        if (enable) {
            ret |= esp_io_expander_set_pullupdown(io_expander, BSP_LCD_EN, IO_EXPANDER_PULL_UP);
            ret |= esp_io_expander_set_dir(io_expander, BSP_LCD_EN, IO_EXPANDER_INPUT);
            ret |= esp_io_expander_set_output_mode(io_expander, BSP_LCD_EN, IO_EXPANDER_OUTPUT_MODE_OPEN_DRAIN);
        } else {
            ret |= esp_io_expander_set_dir(io_expander, BSP_LCD_EN, IO_EXPANDER_OUTPUT);
            ret |= esp_io_expander_set_level(io_expander, BSP_LCD_EN, 0);
            ret |= esp_io_expander_set_output_mode(io_expander, BSP_LCD_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        }
        break;
    }
    case BSP_FEATURE_TOUCH: {
        esp_io_expander_handle_t io_expander = bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_TOUCH_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_TOUCH_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_TOUCH_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_SPEAKER: {
        esp_io_expander_handle_t io_expander = bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_SPEAKER_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_SPEAKER_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_SPEAKER_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_CAMERA: {
        esp_io_expander_handle_t io_expander = bsp_io_expander_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_CAMERA_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_CAMERA_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_CAMERA_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_USB: {
        esp_io_expander_handle_t io_expander = bsp_io_expander1_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_USB_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_USB_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_USB_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    case BSP_FEATURE_WIFI: {
        esp_io_expander_handle_t io_expander = bsp_io_expander1_init();
        ret |= esp_io_expander_set_dir(io_expander, BSP_WIFI_EN, IO_EXPANDER_OUTPUT);
        ret |= esp_io_expander_set_level(io_expander, BSP_WIFI_EN, enable);
        ret |= esp_io_expander_set_output_mode(io_expander, BSP_WIFI_EN, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL);
        break;
    }
    }

    return ret;
}

bool bsp_headphone_detect( void )
{
	static uint8_t hp_jeck_init = 0;
	
    esp_io_expander_handle_t io_expander = bsp_io_expander_init();
    if( hp_jeck_init == 0){
        esp_err_t ret;
        ret = esp_io_expander_set_dir(io_expander, IO_EXPANDER0_HP_DETECT, IO_EXPANDER_INPUT); 
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        /* The PI4IOE5V6408 powers up with every pin pulled *down* (pull-enable 0xFF, select
        * 0x00), so an open detect switch reads low and the jack looks permanently occupied.
        * The line needs the pull-up for its level to mean anything. */
        ret = esp_io_expander_set_pullupdown(io_expander, IO_EXPANDER0_HP_DETECT, IO_EXPANDER_PULL_UP); 
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        hp_jeck_init = 1;
    }

    uint32_t levels = 0;
    if (esp_io_expander_get_level(io_expander, IO_EXPANDER0_HP_DETECT, &levels) != ESP_OK) {
        return false;
    }

    /* Empty jack reads low. Measured: with the expander's pull-up enabled and nothing
     * plugged in, the line still sits at zero, so the detect switch grounds it while it is
     * idle and releases it to the pull-up once a plug lifts the contact. */
    return (levels & IO_EXPANDER0_HP_DETECT) != 0;
}

bool bsp_usb_c_detect( void )
{
	static uint8_t usb_c_jeck_init = 0;

    esp_io_expander_handle_t io_expander = bsp_io_expander_init();
    if( usb_c_jeck_init == 0){
        esp_err_t ret;
        ret = esp_io_expander_set_dir(io_expander, IO_EXPANDER1_CHG_STAT_LED, IO_EXPANDER_INPUT); 
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        /* The PI4IOE5V6408 powers up with every pin pulled *down* (pull-enable 0xFF, select
        * 0x00), so an open detect switch reads low and the jack looks permanently occupied.
        * The line needs the pull-up for its level to mean anything. */
        ret = esp_io_expander_set_pullupdown(io_expander, IO_EXPANDER1_CHG_STAT_LED, IO_EXPANDER_PULL_UP); 
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        usb_c_jeck_init = 1;
    }


    uint32_t levels = 0;
    if (esp_io_expander_get_level(io_expander, IO_EXPANDER1_CHG_STAT_LED, &levels) != ESP_OK) {
        return false;
    }

    /* Empty jack reads low. Measured: with the expander's pull-up enabled and nothing
     * plugged in, the line still sits at zero, so the detect switch grounds it while it is
     * idle and releases it to the pull-up once a plug lifts the contact. */
    return (levels & IO_EXPANDER0_HP_DETECT) != 0;
}


static esp_err_t drive(esp_io_expander_handle_t expander, uint32_t pin_mask, bool level) {
    ESP_RETURN_ON_ERROR(esp_io_expander_set_dir(expander, pin_mask, IO_EXPANDER_OUTPUT), TAG, "set_dir failed");
    ESP_RETURN_ON_ERROR(esp_io_expander_set_output_mode(expander, pin_mask, IO_EXPANDER_OUTPUT_MODE_PUSH_PULL), TAG, "set_output_mode failed");
    return esp_io_expander_set_level(expander, pin_mask, level);
}

void bsp_set_charge_qc_en( bool en )
{
    esp_io_expander_handle_t io_expander = bsp_io_expander1_init();
    esp_err_t ret = drive( io_expander, IO_EXPANDER1_nCHG_QC_EN, !en );
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
}

void bsp_set_charge_en( bool en )
{
    esp_io_expander_handle_t io_expander = bsp_io_expander1_init();
    esp_err_t ret = drive( io_expander, IO_EXPANDER1_CHG_EN, en );
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);    
}

void bsp_set_usb_5v_en( bool en )
{
    esp_io_expander_handle_t io_expander = bsp_io_expander1_init();
    esp_err_t ret = drive( io_expander, IO_EXPANDER1_USB5V_EN, en );
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret); 
}

void bsp_set_ext_5v_en( bool en )
{
    esp_io_expander_handle_t io_expander = bsp_io_expander_init();
    esp_err_t ret = drive( io_expander, IO_EXPANDER0_EXT5V_EN, en );
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret); 
}

void bsp_generate_poweroff_signal( void )
{
    ESP_LOGW( TAG, "Generate poweroff signal!" );
    esp_io_expander_handle_t io_expander = bsp_io_expander1_init();
    // Try to generate poweroff signal 3 times to make sure it works :)
    for ( int i = 0; i < 3; i++ ) {
        esp_io_expander_set_level( io_expander, IO_EXPANDER1_PWEROFF_PULSE, 1 );
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        esp_io_expander_set_level( io_expander, IO_EXPANDER1_PWEROFF_PULSE, 0 );
        vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}
   

void bsp_set_ext_antenna_enable( bool en )
{
    esp_io_expander_handle_t io_expander = bsp_io_expander_init();
    esp_err_t ret = drive( io_expander, IO_EXPANDER0_EXT5V_EN, en );
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret); 

}

void bsp_set_wifi_power_enable( bool en )
{
    ESP_LOGI( TAG, "set_wifi_power_enable: %d", en );
    bsp_feature_enable( BSP_FEATURE_WIFI, en );
}

void bsp_reset_tp()
{
    ESP_LOGI( TAG, "reset tp" );
    esp_io_expander_handle_t io_expander = bsp_io_expander_init();
    esp_io_expander_set_level( io_expander, BSP_TOUCH_EN, 0 );
    vTaskDelay( pdMS_TO_TICKS( 10 ) );
    esp_io_expander_set_level( io_expander, BSP_TOUCH_EN, 1 );
    vTaskDelay( pdMS_TO_TICKS( 100 ) );
}

void bsp_set_speaker_enable(bool enable)
{   
    esp_err_t ret;
    ret = bsp_feature_enable(BSP_FEATURE_SPEAKER, enable);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable/disable speaker feature: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "Speaker %s (P1=%d)", enable ? "enabled" : "disabled", enable ? 1 : 0);
}
