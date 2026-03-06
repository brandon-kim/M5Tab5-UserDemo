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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c_master.h"

#define INA226_ADDRESS (0x40)

#define INA226_REG_CONFIG           (0x00)
#define INA226_REG_SHUNTVOLTAGE     (0x01)
#define INA226_REG_BUSVOLTAGE       (0x02)
#define INA226_REG_POWER            (0x03)
#define INA226_REG_CURRENT          (0x04)
#define INA226_REG_CALIBRATION      (0x05)
#define INA226_REG_MASKENABLE       (0x06)
#define INA226_REG_ALERTLIMIT       (0x07)
#define INA226_REG_MANUFACTURER_ID  (0xFE)
#define INA226_REG_DIE_ID           (0xFF)

/* mask/enalbe Register(06h) */
#define INA226_BIT_SOL  (1<<15)   // Shunt voltage over-voltage limit
#define INA226_BIT_SUL  (1<<14)   // Shunt voltage under-voltage limit
#define INA226_BIT_BOL  (1<<13)   // Bus voltage over-voltage limit
#define INA226_BIT_BUL  (1<<12)   // Bus voltage under-voltage limit
#define INA226_BIT_POL  (1<<11)   // Power over-voltage limit
#define INA226_BIT_CNVR (1<<10)   // Conversion ready
#define INA226_BIT_AFF  (1<<4)    // Alert function flag
#define INA226_BIT_CVRF (1<<3)    // Conversion ready flag
#define INA226_BIT_OVF  (1<<2)    // Overflow flag
#define INA226_BIT_APOL (1<<1)    // Alert polarity
#define INA226_BIT_LEN  (1<<0)    // Alert enable

typedef enum {
    INA226_AVERAGES_1    = 0x00,
    INA226_AVERAGES_4    = 0x01,
    INA226_AVERAGES_16   = 0x02,
    INA226_AVERAGES_64   = 0x03,
    INA226_AVERAGES_128  = 0x04,
    INA226_AVERAGES_256  = 0x05,
    INA226_AVERAGES_512  = 0x06,
    INA226_AVERAGES_1024 = 0x07
} ina226_averages_t;

typedef enum {
    INA226_BUS_CONV_TIME_140US  = 0x00,
    INA226_BUS_CONV_TIME_204US  = 0x01,
    INA226_BUS_CONV_TIME_332US  = 0x02,
    INA226_BUS_CONV_TIME_588US  = 0x03,
    INA226_BUS_CONV_TIME_1100US = 0x04,
    INA226_BUS_CONV_TIME_2116US = 0x05,
    INA226_BUS_CONV_TIME_4156US = 0x06,
    INA226_BUS_CONV_TIME_8244US = 0x07
} ina226_busConvTime_t;

typedef enum {
    INA226_SHUNT_CONV_TIME_140US  = 0x00,
    INA226_SHUNT_CONV_TIME_204US  = 0x01,
    INA226_SHUNT_CONV_TIME_332US  = 0x02,
    INA226_SHUNT_CONV_TIME_588US  = 0x03,
    INA226_SHUNT_CONV_TIME_1100US = 0x04,
    INA226_SHUNT_CONV_TIME_2116US = 0x05,
    INA226_SHUNT_CONV_TIME_4156US = 0x06,
    INA226_SHUNT_CONV_TIME_8244US = 0x07
} ina226_shuntConvTime_t;


typedef enum {
    INA226_MODE_POWER_DOWN     = 0x00,
    INA226_MODE_SHUNT_TRIG     = 0x01,
    INA226_MODE_BUS_TRIG       = 0x02,
    INA226_MODE_SHUNT_BUS_TRIG = 0x03,
    INA226_MODE_ADC_OFF        = 0x04,
    INA226_MODE_SHUNT_CONT     = 0x05,
    INA226_MODE_BUS_CONT       = 0x06,
    INA226_MODE_SHUNT_BUS_CONT = 0x07,
} ina226_mode_t;

#include <stdbool.h>

/*
 * C API — single static driver instance inside implementation.
 * Function names follow `ina226_` prefix; no instance pointer required.
 */

bool ina226_init(i2c_master_bus_handle_t bus_handle, uint8_t address /*= INA226_ADDRESS*/);
bool ina226_configure(ina226_averages_t avg                /*= INA226_AVERAGES_1*/,
                      ina226_busConvTime_t busConvTime     /*= INA226_BUS_CONV_TIME_1100US*/,
                      ina226_shuntConvTime_t shuntConvTime /*= INA226_SHUNT_CONV_TIME_1100US*/,
                      ina226_mode_t mode                   /*= INA226_MODE_SHUNT_BUS_CONT*/);
bool ina226_calibrate(float rShuntValue /*= 0.1*/, float iMaxExpected /*= 2*/);

ina226_averages_t ina226_getAverages(void);
ina226_busConvTime_t ina226_getBusConversionTime(void);
ina226_shuntConvTime_t ina226_getShuntConversionTime(void);
ina226_mode_t ina226_getMode(void);

void ina226_enableShuntOverLimitAlert(void);
void ina226_enableShuntUnderLimitAlert(void);
void ina226_enableBusOvertLimitAlert(void);
void ina226_enableBusUnderLimitAlert(void);
void ina226_enableOverPowerLimitAlert(void);
void ina226_enableConversionReadyAlert(void);

void ina226_disableAlerts(void);

void ina226_setBusVoltageLimit(float voltage);
void ina226_setShuntVoltageLimit(float voltage);
void ina226_setPowerLimit(float watts);

void ina226_setAlertInvertedPolarity(bool inverted);
void ina226_setAlertLatch(bool latch);

bool ina226_isMathOverflow(void);
bool ina226_isAlert(void);

float ina226_readShuntCurrent(void);
float ina226_readShuntVoltage(void);
float ina226_readBusPower(void);
float ina226_readBusVoltage(void);
int16_t ina226_readRawShuntCurrent(void);

float ina226_getMaxPossibleCurrent(void);
float ina226_getMaxCurrent(void);
float ina226_getMaxShuntVoltage(void);
float ina226_getMaxPower(void);

uint16_t ina226_getMaskEnable(void);

/* internal helpers remain private to implementation */


#ifdef __cplusplus
}
#endif
