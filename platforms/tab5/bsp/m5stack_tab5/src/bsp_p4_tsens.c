#include "bsp/esp32_p4_tsense.h"

#include "driver/temperature_sensor.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "esp32_p4_tsens";

static temperature_sensor_handle_t tsens = NULL;

esp_err_t bsp_tsense_init(void)
{
    if ( tsens != NULL ){
        ESP_LOGW(TAG, "Temperature sensor already initialized");
        return ESP_OK;
    }

    
    temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80); // 원하는 온도 범위        
    ESP_RETURN_ON_ERROR(temperature_sensor_install(&config, &tsens), TAG, "Temperature sensor install failed");
    ESP_RETURN_ON_ERROR(temperature_sensor_enable(tsens), TAG, "Temperature sensor enable failed");    
    
    return ESP_OK;
}

esp_err_t bsp_tsense_deinit(void)
{
    if ( tsens == NULL ) {
        ESP_LOGW(TAG, "Temperature sensor not initialized");
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(temperature_sensor_disable(tsens), TAG, "Temperature sensor disable failed");
    ESP_RETURN_ON_ERROR(temperature_sensor_uninstall(tsens), TAG, "Temperature sensor uninstall failed");
    tsens = NULL;
    return ESP_OK;
}

uint32_t bsp_tsense_read_x100(void)
{
    if ( tsens == NULL ) {
        ESP_LOGE(TAG, "Temperature sensor not initialized");
        return 0;
    }
    esp_err_t ret;
    float celsius = 0; 
    uint32_t celsiusx100;
    ret =  temperature_sensor_get_celsius(tsens, &celsius);
    if (ret != ESP_OK) {
        ESP_LOGE("TSENS", "Read failed: %d", ret);
        return 0;
    }

    celsiusx100 = (uint32_t)(celsius * 100);
    ESP_LOGI("TSENS", "Celsius: %d(x100)", celsiusx100);
    return celsiusx100;
}