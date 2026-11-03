/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "rx8130.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"

#define TAG "rx8130"


// RX-8130 Register definitions
#define RX8130_REG_SEC   0x10
#define RX8130_REG_MIN   0x11
#define RX8130_REG_HOUR  0x12
#define RX8130_REG_WDAY  0x13
#define RX8130_REG_MDAY  0x14
#define RX8130_REG_MONTH 0x15
#define RX8130_REG_YEAR  0x16

#define RX8130_REG_ALMIN   0x17
#define RX8130_REG_ALHOUR  0x18
#define RX8130_REG_ALWDAY  0x19
#define RX8130_REG_TCOUNT0 0x1A
#define RX8130_REG_TCOUNT1 0x1B
#define RX8130_REG_EXT     0x1C
#define RX8130_REG_FLAG    0x1D
#define RX8130_REG_CTRL0   0x1E
#define RX8130_REG_CTRL1   0x1F

#define RX8130_REG_END 0x23

// Extension Register (1Ch) bit positions
#define RX8130_BIT_EXT_TSEL (7 << 0)
#define RX8130_BIT_EXT_WADA (1 << 3)
#define RX8130_BIT_EXT_TE   (1 << 4)
#define RX8130_BIT_EXT_USEL (1 << 5)
#define RX8130_BIT_EXT_FSEL (3 << 6)

// Flag Register (1Dh) bit positions
#define RX8130_BIT_FLAG_VLF (1 << 1)
#define RX8130_BIT_FLAG_AF  (1 << 3)
#define RX8130_BIT_FLAG_TF  (1 << 4)
#define RX8130_BIT_FLAG_UF  (1 << 5)

// Control 0 Register (1Еh) bit positions
#define RX8130_BIT_CTRL_TSTP (1 << 2)
#define RX8130_BIT_CTRL_AIE  (1 << 3)
#define RX8130_BIT_CTRL_TIE  (1 << 4)
#define RX8130_BIT_CTRL_UIE  (1 << 5)
#define RX8130_BIT_CTRL_STOP (1 << 6)
#define RX8130_BIT_CTRL_TEST (1 << 7)

// Control 1 Register (1Fh) bit positions
#define RX8130_1F_BIT_BF_VSEL0   ( 1 << 0)
#define RX8130_1F_BIT_BF_VSEL1   ( 1 << 1)
#define RX8130_1F_BIT_RS_VSEL    ( 1 << 2)
#define RX8130_1F_BIT_INIEN      ( 1 << 4)
#define RX8130_1F_BIT_CHG_EN     ( 1 << 5)
#define RX8130_1F_BIT_SMP_TSEL0  ( 1 << 6)
#define RX8130_1F_BIT_SMP_TSEL1  ( 1 << 7)



#define setbit(x, y)        x |= (0x01 << y)
#define clrbit(x, y)        x &= ~(0x01 << y)
#define reversebit(x, y)    x ^= (0x01 << y)
#define getbit(x, y)        ((x) >> (y)&0x01)


typedef struct {
    i2c_master_dev_handle_t _i2c_device_handle;
} rx8130_dev, *rx8130_t;

#define I2C_MASTER_TIMEOUT_MS           50     // 

static uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0f);
}

static uint8_t read_reg8(rx8130_t handle, uint8_t reg);
static esp_err_t write_reg8(rx8130_t handle, uint8_t reg, uint8_t value);
static esp_err_t read_reg(rx8130_t handle, uint8_t reg, uint8_t* buf, uint8_t len);
static esp_err_t write_reg(rx8130_t handle, uint8_t reg, uint8_t* buf, uint8_t len);

static uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

static rx8130_t s_rx8130_handle = NULL;

/****************************************************************************************
 * @brief Initialize the RX8130 RTC device
 *
 * @param busHandle I2C bus handle
 * @param addr I2C device address
 * @return true if initialization is successful, false otherwise
 ***************************************************************************************/
bool rx8130_init( i2c_master_bus_handle_t busHandle, uint8_t addr )
{
    if ( s_rx8130_handle == NULL ) {
        s_rx8130_handle = malloc(sizeof(rx8130_dev));
        if ( s_rx8130_handle == NULL ) {
            return false;
        }
    }

    rx8130_t dev_handle = s_rx8130_handle;
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(busHandle, &dev_cfg, &s_rx8130_handle->_i2c_device_handle);
    if ( ret != ESP_OK || s_rx8130_handle->_i2c_device_handle == NULL ) {
        return false;
    }

    return true;
}


void rx8130_initBat( void )
{
    rx8130_t handle = s_rx8130_handle;

    uint8_t data = read_reg8(handle, RX8130_REG_CTRL1);
    setbit(data, 4);
    setbit(data, 5);
    write_reg8(handle, RX8130_REG_CTRL1, data);
    data = read_reg8(handle, RX8130_REG_CTRL1);
    ESP_LOGI(TAG, "rtc bat init: 0x1F: %02X\n", data);
}

void rx8130_setTime( struct tm* time )
{
    rx8130_t handle = s_rx8130_handle;

    uint8_t rbuf = 0;

    time->tm_year -= 100;

    // set STOP bit before changing clock/calendar
    rbuf = read_reg8(handle, RX8130_REG_CTRL0);
    rbuf = rbuf | RX8130_BIT_CTRL_STOP;
    write_reg8(handle, RX8130_REG_CTRL0, rbuf);

    uint8_t date[7] = {dec2bcd(time->tm_sec),       dec2bcd(time->tm_min),  dec2bcd(time->tm_hour),
                       dec2bcd(time->tm_wday),      dec2bcd(time->tm_mday), dec2bcd(time->tm_mon),
                       dec2bcd(time->tm_year % 100)};

    write_reg(handle, RX8130_REG_SEC, date, 7);

    // clear STOP bit after changing clock/calendar
    rbuf = read_reg8(handle, RX8130_REG_CTRL0);
    rbuf = rbuf & ~RX8130_BIT_CTRL_STOP;
    write_reg8(handle, RX8130_REG_CTRL0, rbuf);
}

void rx8130_getTime( struct tm* time )
{
    rx8130_t handle = s_rx8130_handle;

    uint8_t date[7];
    read_reg(handle, RX8130_REG_SEC, date, 7);

    time->tm_sec  = bcd2dec(date[RX8130_REG_SEC - 0x10] & 0x7f);
    time->tm_min  = bcd2dec(date[RX8130_REG_MIN - 0x10] & 0x7f);
    time->tm_hour = bcd2dec(date[RX8130_REG_HOUR - 0x10] & 0x3f);  // only 24-hour clock
    time->tm_mday = bcd2dec(date[RX8130_REG_MDAY - 0x10] & 0x3f);
    time->tm_mon  = bcd2dec(date[RX8130_REG_MONTH - 0x10] & 0x1f);
    time->tm_year = bcd2dec(date[RX8130_REG_YEAR - 0x10]);
    time->tm_wday = bcd2dec(date[RX8130_REG_WDAY - 0x10] & 0x7f);

    time->tm_year += 100;
}

void rx8130_clearIrqFlags(void)
{
    rx8130_t handle = s_rx8130_handle;
    write_reg8(handle, RX8130_REG_FLAG, 0);
}

void rx8130_disableIrq(void)
{
    rx8130_t handle = s_rx8130_handle;
    write_reg8(handle, RX8130_REG_CTRL0, 0);
}

void rx8130_setAlarmIrq( struct tm* time )
{
    rx8130_t handle = s_rx8130_handle;
    uint8_t buf = 0;

    // Write 0 to AIE
    buf = read_reg8(handle, RX8130_REG_CTRL0);
    clrbit(buf, 3);
    clrbit(buf, 5);
    write_reg8(handle, RX8130_REG_CTRL0, buf);

    buf = read_reg8(handle, RX8130_REG_CTRL0);
    // debug_print_reg(0x1E, buf);

    // Hour AE, week AE day AE
    buf = 0x80;
    write_reg8(handle, RX8130_REG_ALWDAY, buf);
    buf = read_reg8(handle, RX8130_REG_ALWDAY);
    // debug_print_reg(0x19, buf);

    buf = 0x80;
    write_reg8(handle, RX8130_REG_ALHOUR, buf);
    buf = read_reg8(handle, RX8130_REG_ALHOUR);
    // debug_print_reg(0x18, buf);

    buf = 0x80;
    write_reg8(handle, RX8130_REG_ALMIN, buf);
    buf = read_reg8(handle, RX8130_REG_ALMIN);
    // debug_print_reg(0x17, buf);

    // Write 1 to AIE
    buf = read_reg8(handle, RX8130_REG_CTRL0);
    setbit(buf, 3);
    write_reg8(handle, RX8130_REG_CTRL0, buf);

    buf = read_reg8(handle, RX8130_REG_CTRL0);
    // debug_print_reg(0x1E, buf);
}

void rx8130_setTimerIrq(uint16_t seconds)
{
    rx8130_t handle = s_rx8130_handle;
    uint8_t flag_register = 0;
    uint8_t buffer[2]     = {0};
    buffer[0]             = seconds & 0xFF;         // 定时器低字节
    buffer[1]             = (seconds >> 8) & 0xFF;  // 定时器高字节

    // Step 1: Disable Timer
    flag_register = read_reg8(handle, RX8130_REG_EXT);

    flag_register &= ~(1 << 4);  // 禁用定时器 (TE = 0)
    write_reg8(handle, RX8130_REG_EXT, flag_register);

    // Setp 2: Write Timer Counter Register (1Ah, 1Bh)
    write_reg(handle, RX8130_REG_TCOUNT0, buffer, 2);

    // Step 3: Enable Timer
    flag_register = read_reg8(handle, RX8130_REG_EXT);

    setbit(flag_register, 4);  // 启用定时器 (TE = 1)
    clrbit(flag_register, 2);
    setbit(flag_register, 1);
    clrbit(flag_register, 0);
    write_reg8(handle, RX8130_REG_EXT, flag_register);

    // Step 4: Enable Timer Interrupt
    flag_register = read_reg8(handle, RX8130_REG_CTRL0);
    flag_register |= (1 << 4);  // 启用定时器中断 (TIE = 1)
    write_reg8(handle, RX8130_REG_CTRL0, flag_register);
}

static uint8_t read_reg8( rx8130_t handle, uint8_t reg )
{
    uint8_t value;
    if ( ESP_OK == read_reg(handle, reg, &value, 1) ){
        return value;
    }
    return 0;
}

static esp_err_t write_reg8( rx8130_t handle, uint8_t reg, uint8_t value)
{
    uint8_t buf[1] = {value};
    return write_reg(handle, reg, buf, 1);
}

static esp_err_t read_reg( rx8130_t handle, uint8_t reg, uint8_t* buf, uint8_t len )
{
    uint8_t w_buffer[1] = {0};
    w_buffer[0]         = reg;
    return i2c_master_transmit_receive(handle->_i2c_device_handle, w_buffer, 1, buf, len, I2C_MASTER_TIMEOUT_MS);
}

static esp_err_t write_reg( rx8130_t handle, uint8_t reg, uint8_t* buf, uint8_t len)
{
    uint8_t w_buffer[1 + len];
    w_buffer[0] = reg;
    memcpy(w_buffer + 1, buf, len);
    return i2c_master_transmit(handle->_i2c_device_handle, w_buffer, 1 + len, I2C_MASTER_TIMEOUT_MS);
}