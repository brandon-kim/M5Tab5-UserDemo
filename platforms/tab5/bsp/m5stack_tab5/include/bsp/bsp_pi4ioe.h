
#ifndef __BSP_BSP_PI4IOE_H__
#define __BSP_BSP_PI4IOE_H__


#include "esp_err.h"
#include "bsp/config.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#define I2C_DEV_ADDR_PI4IOE1  0x43  // addr pin low
#define I2C_DEV_ADDR_PI4IOE2  0x44  // addr pin high
#define I2C_MASTER_TIMEOUT_MS 50


#define PI4IO_REG_CHIP_RESET 0x01
#define PI4IO_REG_IO_DIR     0x03
#define PI4IO_REG_OUT_SET    0x05
#define PI4IO_REG_OUT_H_IM   0x07
#define PI4IO_REG_IN_DEF_STA 0x09
#define PI4IO_REG_PULL_EN    0x0B
#define PI4IO_REG_PULL_SEL   0x0D
#define PI4IO_REG_IN_STA     0x0F
#define PI4IO_REG_INT_MASK   0x11
#define PI4IO_REG_IRQ_STA    0x13


extern i2c_master_dev_handle_t i2c_dev_handle_pi4ioe1;
extern i2c_master_dev_handle_t i2c_dev_handle_pi4ioe2;

#define setbit(x, y) x |= (0x01 << y)
#define clrbit(x, y) x &= ~(0x01 << y)

extern void bsp_io_expander_pi4ioe_init(i2c_master_bus_handle_t bus_handle);

extern esp_err_t bsp_io_expander1_set_bit( uint8_t bit_mask, uint8_t level) ;
extern esp_err_t bsp_io_expander2_set_bit( uint8_t bit_mask, uint8_t level) ;

extern uint8_t  bsp_io_expander1_read_bit( uint8_t bit_mask );
extern uint8_t  bsp_io_expander2_read_bit( uint8_t bit_mask );

extern esp_err_t  bsp_io_expander1_read_output( uint8_t *output );
extern esp_err_t  bsp_io_expander2_read_output( uint8_t *output );



#endif // __BSP_BSP_PI4IOE_H__
