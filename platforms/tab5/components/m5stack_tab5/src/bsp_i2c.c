/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp_err_check.h"
#include "bsp/m5stack_tab5.h"

//static const char *TAG = "bsp";

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
