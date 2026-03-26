
file(GLOB bsp_src src/*.c )

idf_component_register(
    SRCS ${bsp_src}
    INCLUDE_DIRS "include" 
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_i2s esp_driver_sdmmc esp_driver_sdspi vfs fatfs spiffs
    PRIV_REQUIRES esp_psram esp_lcd esp_driver_ledc esp_driver_tsens esp_driver_spi
)
