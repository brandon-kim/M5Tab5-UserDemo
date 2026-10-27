#include "bsp/bsp_p4_tsense.h"

#include "driver/temperature_sensor.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "TSENS";

static temperature_sensor_handle_t tsens = NULL;

esp_err_t bsp_tsense_init(void)
{
    if ( tsens != NULL ){
        ESP_LOGW(TAG, "already initialized");
        return ESP_OK;
    }
    
    temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);  
    ESP_RETURN_ON_ERROR(temperature_sensor_install(&config, &tsens), TAG, "Temperature sensor install failed");
    ESP_RETURN_ON_ERROR(temperature_sensor_enable(tsens), TAG, "Temperature sensor enable failed");    
    
    return ESP_OK;
}

esp_err_t bsp_tsense_deinit(void)
{
    if ( tsens == NULL ) {
        ESP_LOGW(TAG, "not initialized");
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(temperature_sensor_disable(tsens), TAG, "Temperature sensor disable failed");
    ESP_RETURN_ON_ERROR(temperature_sensor_uninstall(tsens), TAG, "Temperature sensor uninstall failed");
    tsens = NULL;
    return ESP_OK;
}

esp_err_t bsp_tsense_read_x100(uint32_t *tempx100)
{
    if ( tsens == NULL ) {
        ESP_LOGE(TAG, "not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret;
    float celsius = 0; 
    uint32_t celsiusx100;
    ret =  temperature_sensor_get_celsius(tsens, &celsius);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %d", ret);
        return ret;
    }

    celsiusx100 = (uint32_t)((celsius * 100)+0.5); // round to nearest integer
    ESP_LOGI(TAG, "Celsius: %d(x100)", celsiusx100);
    *tempx100 = celsiusx100;
    return ESP_OK;
}