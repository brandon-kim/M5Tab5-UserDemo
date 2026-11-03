
file(GLOB_RECURSE SRC src/*.c)

set(REQ esp_driver_gpio esp_driver_i2c esp_driver_i2s spiffs fatfs vfs esp_driver_sdspi esp_driver_sdmmc esp_adc esp_wifi esp_event esp_netif )
set(PRIV_REQ esp_psram esp_lcd esp_driver_ledc esp_driver_tsens esp_driver_spi )

idf_component_register(
    SRCS ${SRC}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES  ${REQ}  
    PRIV_REQUIRES ${PRIV_REQ}
)
