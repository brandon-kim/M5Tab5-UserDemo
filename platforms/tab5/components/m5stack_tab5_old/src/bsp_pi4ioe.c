
#include "bsp/bsp_pi4ioe.h"

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_check.h"

#include "bsp/m5stack_tab5.h"

// PI4IO registers

//==================================================================================
// I/O Expander PI4IOE5V6416
//==================================================================================

i2c_master_dev_handle_t i2c_dev_handle_pi4ioe1;
i2c_master_dev_handle_t i2c_dev_handle_pi4ioe2;

#define TAG "BSP_PI4IOE"

static esp_err_t reg_write1( uint8_t reg_addr, uint8_t data )
{
    uint8_t write_buf[ 2 ] = { 0 };

    write_buf[ 0 ] = reg_addr;
    write_buf[ 1 ] = data;

    return i2c_master_transmit( i2c_dev_handle_pi4ioe1, write_buf, 2, I2C_MASTER_TIMEOUT_MS );
}

static esp_err_t reg_write2( uint8_t reg_addr, uint8_t data )
{
    uint8_t write_buf[ 2 ] = { 0 };

    write_buf[ 0 ] = reg_addr;
    write_buf[ 1 ] = data;

    return i2c_master_transmit( i2c_dev_handle_pi4ioe2, write_buf, 2, I2C_MASTER_TIMEOUT_MS );
}

void bsp_io_expander_pi4ioe_init( i2c_master_bus_handle_t bus_handle )
{
    // Initialize PI4IOE1 (address 0x43)
    i2c_device_config_t dev_cfg1 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = I2C_DEV_ADDR_PI4IOE1,
        .scl_speed_hz    = 400000,
    };
    ESP_ERROR_CHECK( i2c_master_bus_add_device( bus_handle, &dev_cfg1, &i2c_dev_handle_pi4ioe1 ) );

    reg_write1( PI4IO_REG_CHIP_RESET, 0xFF );
    reg_write1( PI4IO_REG_IO_DIR, 0b01110111 ); // 0: input 1: output
    reg_write1( PI4IO_REG_OUT_H_IM, 0b00000000 ); // Disable High-Impedance for used pins
    reg_write1( PI4IO_REG_PULL_SEL, 0b01111111 ); // pull up/down select, 0 down, 1 up
    reg_write1( PI4IO_REG_PULL_EN, 0b01111111 ); // P7 interrupt activatiion: 0 enable, 1 disable
    /* Set output port registers P1(SPK_EN), P2(EXT5V_EN), P4(LCD_RST), P5(TP_RST), P6(CAM)RST to HIGH */
    uint8_t initial = IO_EXPANDER1_SPK_EN | IO_EXPANDER1_EXT5V_EN | IO_EXPANDER1_LCD_RST | IO_EXPANDER1_TP_RST | IO_EXPANDER1_CAM_RST;
    reg_write1( PI4IO_REG_OUT_SET, initial );

    // Initialize PI4IOE2 (address 0x44)
    i2c_device_config_t dev_cfg2 = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = I2C_DEV_ADDR_PI4IOE2,
        .scl_speed_hz    = 400000,
    };
    ESP_ERROR_CHECK( i2c_master_bus_add_device( bus_handle, &dev_cfg2, &i2c_dev_handle_pi4ioe2 ) );

    reg_write2( PI4IO_REG_CHIP_RESET, 0xFF );

    reg_write2( PI4IO_REG_IO_DIR, 0b10111001 ); // 0: input 1: output
    reg_write2( PI4IO_REG_OUT_H_IM, 0b00000110 ); // Disable High-Impedance for used pins
    reg_write2( PI4IO_REG_PULL_SEL, 0b10111001 ); // pull up/down select, 0 down, 1 up
    reg_write2( PI4IO_REG_PULL_EN, 0b11111001 ); // pull up/down enable, 0 disable, 1 enable
    reg_write2( PI4IO_REG_IN_DEF_STA, 0b01000000 ); // P6 default high level
    reg_write2( PI4IO_REG_INT_MASK, 0b10111111 ); // P6 interrupt enable 0 enable, 1 disable
    /* Output Port Register P0(WLAN_PWR_EN), P3(USB5V_EN), output high level */

    reg_write2( PI4IO_REG_OUT_SET, 0b00001001 );
}

esp_err_t bsp_io_expander1_set_bit( uint8_t bit_mask, uint8_t level )
{
    uint8_t write_buf[ 2 ] = { 0 };
    uint8_t read_buf[ 1 ]  = { 0 };

    write_buf[ 0 ] = PI4IO_REG_OUT_SET;

    if ( ESP_OK != i2c_master_transmit_receive( i2c_dev_handle_pi4ioe1, write_buf, 1, read_buf, 1, I2C_MASTER_TIMEOUT_MS ) ) {
        return ESP_FAIL;
    }

    write_buf[ 1 ] = read_buf[ 0 ];
    if ( level ) {
        write_buf[ 1 ] |= bit_mask;
    }
    else {
        write_buf[ 1 ] &= ~bit_mask;
    }

    return i2c_master_transmit( i2c_dev_handle_pi4ioe1, write_buf, 2, I2C_MASTER_TIMEOUT_MS );
}

esp_err_t bsp_io_expander2_set_bit( uint8_t bit_mask, uint8_t level )
{
    uint8_t write_buf[ 2 ] = { 0 };
    uint8_t read_buf[ 1 ]  = { 0 };

    write_buf[ 0 ] = PI4IO_REG_OUT_SET;
    if ( ESP_OK != i2c_master_transmit_receive( i2c_dev_handle_pi4ioe2, write_buf, 1, read_buf, 1, I2C_MASTER_TIMEOUT_MS ) ) {
        return ESP_FAIL;
    }

    write_buf[ 1 ] = read_buf[ 0 ];
    if ( level ) {
        write_buf[ 1 ] |= bit_mask;
    }
    else {
        write_buf[ 1 ] &= ~bit_mask;
    }

    return i2c_master_transmit( i2c_dev_handle_pi4ioe2, write_buf, 2, I2C_MASTER_TIMEOUT_MS );
}

uint8_t bsp_io_expander1_read_bit( uint8_t bit_mask )
{
    uint8_t write_buf[ 2 ] = { 0 };
    uint8_t read_buf[ 1 ]  = { 0 };

    write_buf[ 0 ] = PI4IO_REG_IN_STA;
    if ( ESP_OK != i2c_master_transmit_receive( i2c_dev_handle_pi4ioe1, write_buf, 1, read_buf, 1, I2C_MASTER_TIMEOUT_MS ) ) {
        return 0xFF;
    }

    return ( read_buf[ 0 ] & bit_mask ) ? 1 : 0;
}

uint8_t bsp_io_expander2_read_bit( uint8_t bit_mask )
{
    uint8_t write_buf[ 2 ] = { 0 };
    uint8_t read_buf[ 1 ]  = { 0 };

    write_buf[ 0 ] = PI4IO_REG_IN_STA;
    if ( ESP_OK != i2c_master_transmit_receive( i2c_dev_handle_pi4ioe2, write_buf, 1, read_buf, 1, I2C_MASTER_TIMEOUT_MS ) ) {
        return 0xFF;
    }

    return ( read_buf[ 0 ] & bit_mask ) ? 1 : 0;
}

esp_err_t bsp_io_expander1_read_output( uint8_t *output )
{
    uint8_t write_buf[ 2 ] = { 0 };
    uint8_t read_buf[ 1 ]  = { 0 };

    write_buf[ 0 ] = PI4IO_REG_OUT_SET;
    if ( ESP_OK != i2c_master_transmit_receive( i2c_dev_handle_pi4ioe1, write_buf, 1, read_buf, 1, I2C_MASTER_TIMEOUT_MS ) ) {
        return ESP_FAIL;
    }

    *output = read_buf[ 0 ];
    return ESP_OK;
}

esp_err_t bsp_io_expander2_read_output( uint8_t *output )
{
    uint8_t write_buf[ 2 ] = { 0 };
    uint8_t read_buf[ 1 ]  = { 0 };

    write_buf[ 0 ] = PI4IO_REG_OUT_SET;

    if ( ESP_OK != i2c_master_transmit_receive( i2c_dev_handle_pi4ioe2, write_buf, 1, read_buf, 1, I2C_MASTER_TIMEOUT_MS ) ) {
        return ESP_FAIL;
    }

    *output = read_buf[ 0 ];
    return ESP_OK;
}