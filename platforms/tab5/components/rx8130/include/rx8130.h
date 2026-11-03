/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
//#pragma once
#ifndef __RX8130_H__
#define __RX8130_H__

#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// https://download.epsondevice.com/td/pdf/app/RX8130CE_en.pdf
// https://github.com/alexreinert/piVCCU/blob/master/kernel/rtc-rx8130.c

// Public functions
bool rx8130_init(i2c_master_bus_handle_t busHandle, uint8_t addr);
void rx8130_initBat();
void rx8130_setTime( struct tm* time);
void rx8130_getTime(struct tm* time);
void rx8130_clearIrqFlags();
void rx8130_disableIrq();
void rx8130_setAlarmIrq(struct tm* time);
void rx8130_setTimerIrq(uint16_t seconds);


#ifdef __cplusplus
}
#endif

#endif /* __RX8130_H__ */