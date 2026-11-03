/*

The MIT License

Copyright (c) 2014-2023 Korneliusz Jarzębski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include "ina226.h"

#include "esp_err.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>


#define I2C_MASTER_TIMEOUT_MS           50     // 

/* internal helpers (file-local) */
static int16_t readRegister16_internal(uint8_t reg);
static esp_err_t writeRegister16_internal(uint8_t reg, uint16_t val);

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int8_t inaAddress;
    i2c_master_dev_handle_t i2c_dev_handle;
    float currentLSB;
    float powerLSB;
    float vShuntMax;
    float vBusMax;
    float rShunt;
} ina226_dev_t, *ina226_handle_t;


static ina226_handle_t s_ina226_handle = NULL;


bool ina226_init(i2c_master_bus_handle_t bus_handle, uint8_t address)
{
    ina226_handle_t dev_handle; 

    if( s_ina226_handle == NULL ) {
        dev_handle = (ina226_handle_t)malloc(sizeof(ina226_dev_t));
        if (dev_handle == NULL) {
            return false;
        }
    } else {
        dev_handle = s_ina226_handle;
    }

    dev_handle->inaAddress = address;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = address,
        .scl_speed_hz    = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle->i2c_dev_handle));

    if (dev_handle->i2c_dev_handle == NULL) {
        return false;
    }

    s_ina226_handle = dev_handle;

    return true;
}

bool ina226_configure(ina226_averages_t avg, ina226_busConvTime_t busConvTime, ina226_shuntConvTime_t shuntConvTime,
                      ina226_mode_t mode)
{
    if ( s_ina226_handle == NULL ) {
        return false;
    }
    ina226_handle_t dev_handle = s_ina226_handle;
    uint16_t config = 0;

    config |= (avg << 9 | busConvTime << 6 | shuntConvTime << 3 | mode);

    dev_handle->vBusMax   = 36.0f;
    dev_handle->vShuntMax = 0.08192f;

    writeRegister16_internal(INA226_REG_CONFIG, config);

    return true;
}

bool ina226_calibrate(float rShuntValue, float iMaxExpected)
{
    if ( s_ina226_handle == NULL ) {
        return false;
    }
    ina226_handle_t dev_handle = s_ina226_handle;

    uint16_t calibrationValue;

    dev_handle->rShunt = rShuntValue;
    // float iMaxPossible;
    float minimumLSB;
    // iMaxPossible = vShuntMax / rShunt;
    minimumLSB = iMaxExpected / 32767.0f;

    float current_lsb; 
    current_lsb = (uint32_t)(minimumLSB * 100000000);
    current_lsb /= 100000000;
    current_lsb /= 0.0001;
    current_lsb = ceil(current_lsb);
    current_lsb *= 0.0001;
    
    dev_handle->currentLSB = current_lsb;

    dev_handle->powerLSB = current_lsb * 25.0f;

    calibrationValue = (uint16_t)((0.00512f) / (current_lsb * dev_handle->rShunt));

    writeRegister16_internal(INA226_REG_CALIBRATION, calibrationValue);

    return true;
}

float ina226_getMaxPossibleCurrent(void)
{
    if ( s_ina226_handle == NULL ) {
        return 0.0f;
    }   
    ina226_handle_t dev_handle = s_ina226_handle;
    return (dev_handle->vShuntMax / dev_handle->rShunt);    
}

float ina226_getMaxCurrent(void)
{
    if ( s_ina226_handle == NULL ) {
        return 0.0f;
    }
    ina226_handle_t dev_handle = s_ina226_handle;   

    float maxCurrent  = (dev_handle->currentLSB * 32767.0f);
    float maxPossible = ina226_getMaxPossibleCurrent();

    return (maxCurrent > maxPossible) ? maxPossible : maxCurrent;
}

float ina226_getMaxShuntVoltage(void)
{
    if ( s_ina226_handle == NULL ) {
        return 0.0f;
    }
    ina226_handle_t dev_handle = s_ina226_handle;    
    float maxVoltage = ina226_getMaxCurrent() * dev_handle->rShunt;
    return (maxVoltage >= dev_handle->vShuntMax) ? dev_handle->vShuntMax : maxVoltage;
}

float ina226_getMaxPower(void)
{
    if ( s_ina226_handle == NULL ) {
        return 0.0f;
    }
    ina226_handle_t dev_handle = s_ina226_handle;
    return (ina226_getMaxCurrent() * dev_handle->vBusMax);
}

float ina226_readBusPower(void)
{
    if ( s_ina226_handle == NULL ) {
        return 0.0f;
    } 
    
    ina226_handle_t dev_handle = s_ina226_handle;

    return (readRegister16_internal(INA226_REG_POWER) * dev_handle->powerLSB);
}

float ina226_readShuntCurrent(void)
{
    if ( s_ina226_handle == NULL ) {
        return 0.0f;
    }
    ina226_handle_t dev_handle = s_ina226_handle;
    return (readRegister16_internal(INA226_REG_CURRENT) * dev_handle->currentLSB);
}

int16_t ina226_readRawShuntCurrent(void)
{
    return readRegister16_internal(INA226_REG_CURRENT);
}

float ina226_readShuntVoltage(void)
{
    int16_t v = readRegister16_internal(INA226_REG_SHUNTVOLTAGE);
    return (v * 0.0000025f);
}

float ina226_readBusVoltage(void)
{
    int16_t voltage = readRegister16_internal(INA226_REG_BUSVOLTAGE);
    return (voltage * 0.00125f);
}

ina226_averages_t ina226_getAverages(void)
{
    uint16_t value = readRegister16_internal(INA226_REG_CONFIG);
    value &= 0b0000111000000000u;
    value >>= 9;
    return (ina226_averages_t)value;
}

ina226_busConvTime_t ina226_getBusConversionTime(void)
{
    uint16_t value = readRegister16_internal(INA226_REG_CONFIG);
    value &= 0b0000000111000000u;
    value >>= 6;
    return (ina226_busConvTime_t)value;
}

ina226_shuntConvTime_t ina226_getShuntConversionTime(void)
{
    uint16_t value = readRegister16_internal(INA226_REG_CONFIG);
    value &= 0b0000000000111000u;
    value >>= 3;
    return (ina226_shuntConvTime_t)value;
}

ina226_mode_t ina226_getMode(void)
{
    uint16_t value = readRegister16_internal(INA226_REG_CONFIG);
    value &= 0b0000000000000111u;
    return (ina226_mode_t)value;
}

void ina226_setMaskEnable(uint16_t mask)
{
    writeRegister16_internal(INA226_REG_MASKENABLE, mask);
}

uint16_t ina226_getMaskEnable(void)
{
    return (uint16_t)readRegister16_internal(INA226_REG_MASKENABLE);
}

void ina226_enableShuntOverLimitAlert(void)
{
    writeRegister16_internal(INA226_REG_MASKENABLE, INA226_BIT_SOL);
}

void ina226_enableShuntUnderLimitAlert(void)
{
    writeRegister16_internal(INA226_REG_MASKENABLE, INA226_BIT_SUL);
}

void ina226_enableBusOvertLimitAlert(void)
{
    writeRegister16_internal(INA226_REG_MASKENABLE, INA226_BIT_BOL);
}

void ina226_enableBusUnderLimitAlert(void)
{
    writeRegister16_internal(INA226_REG_MASKENABLE, INA226_BIT_BUL);
}

void ina226_enableOverPowerLimitAlert(void)
{
    writeRegister16_internal(INA226_REG_MASKENABLE, INA226_BIT_POL);
}

void ina226_enableConversionReadyAlert(void)
{
    writeRegister16_internal(INA226_REG_MASKENABLE, INA226_BIT_CNVR);
}

void ina226_disableAlerts(void)
{
    writeRegister16_internal(INA226_REG_MASKENABLE, 0);
}

void ina226_setBusVoltageLimit(float voltage)
{
    uint16_t value = (uint16_t)(voltage / 0.00125f);
    writeRegister16_internal(INA226_REG_ALERTLIMIT, value);
}

void ina226_setShuntVoltageLimit(float voltage)
{
    uint16_t value = (uint16_t)(voltage / 0.0000025f);
    writeRegister16_internal(INA226_REG_ALERTLIMIT, value);
}

void ina226_setPowerLimit(float watts)
{
    if ( s_ina226_handle == NULL ) {
        return;
    }
    ina226_handle_t dev_handle = s_ina226_handle;
    uint16_t value = (uint16_t)(watts / dev_handle->powerLSB);
    writeRegister16_internal(INA226_REG_ALERTLIMIT, value);
}

void ina226_setAlertInvertedPolarity(bool inverted)
{
    uint16_t temp = ina226_getMaskEnable();

    if (inverted) {
        temp |= INA226_BIT_APOL;
    } else {
        temp &= ~INA226_BIT_APOL;
    }

    ina226_setMaskEnable(temp);
}

void ina226_setAlertLatch(bool latch)
{
    uint16_t temp = ina226_getMaskEnable();

    if (latch) {
        temp |= INA226_BIT_LEN;
    } else {
        temp &= ~INA226_BIT_LEN;
    }

    ina226_setMaskEnable(temp);
}

bool ina226_isMathOverflow(void)
{
    return ((ina226_getMaskEnable() & INA226_BIT_OVF) == INA226_BIT_OVF);
}

bool ina226_isAlert(void)
{
    return ((ina226_getMaskEnable() & INA226_BIT_AFF) == INA226_BIT_AFF);
}

/* file-local i2c helpers */
static int16_t readRegister16_internal(uint8_t reg)
{
    if ( s_ina226_handle == NULL ) {
        return 0;
    }   
    ina226_handle_t dev_handle = s_ina226_handle;
    uint8_t r_buffer[2] = {0};
    esp_err_t ret;
    ret = i2c_master_transmit_receive(dev_handle->i2c_dev_handle, &reg, 1, r_buffer, 2, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        return 0;
    }
    return (int16_t)(r_buffer[0] << 8 | r_buffer[1]);
}

static esp_err_t writeRegister16_internal(uint8_t reg, uint16_t val)
{
    if ( s_ina226_handle == NULL ) {
        return ESP_ERR_INVALID_STATE;
    }   
    ina226_handle_t dev_handle = s_ina226_handle;

    uint8_t w_buffer[3] = {0};
    w_buffer[0] = reg;
    w_buffer[1] = (val >> 8) & 0xFF;
    w_buffer[2] = val & 0xFF;
    return i2c_master_transmit(dev_handle->i2c_dev_handle, w_buffer, 3, I2C_MASTER_TIMEOUT_MS);
}

#ifdef __cplusplus
}
#endif

