/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal/hal_esp32.h"
#include <mooncake_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <bsp/m5stack_tab5.h>
#include <esp_sleep.h>
#include <esp_check.h>
#include <esp_pm.h>

static const std::string _tag = "power";

void HalEsp32::updatePowerMonitorData()
{
    // mclog::tagInfo(_tag, "update power monitor");
    powerMonitorData.busVoltage   = ina226_readBusVoltage();
    powerMonitorData.shuntVoltage = ina226_readShuntVoltage();
    powerMonitorData.busPower     = ina226_readBusPower();
    powerMonitorData.shuntCurrent = ina226_readShuntCurrent();
}

void HalEsp32::setChargeQcEnable(bool enable)
{
    _charge_qc_enable = enable;
    mclog::tagInfo(_tag, "set charge qc enable: {}", _charge_qc_enable);
    bsp_set_charge_qc_en(_charge_qc_enable);
}

bool HalEsp32::getChargeQcEnable()
{
    return _charge_qc_enable;
}

void HalEsp32::setChargeEnable(bool enable)
{
    _charge_enable = enable;
    mclog::tagInfo(_tag, "set charge enable: {}", _charge_enable);
    bsp_set_charge_en(_charge_enable);
}

bool HalEsp32::getChargeEnable()
{
    return _charge_enable;
}

void HalEsp32::setUsb5vEnable(bool enable)
{
    _usba_5v_enable = enable;
    mclog::tagInfo(_tag, "set usb 5v enable: {}", _usba_5v_enable);
    bsp_set_usb_5v_en(_usba_5v_enable);
}

bool HalEsp32::getUsb5vEnable()
{
    return _usba_5v_enable;
}

void HalEsp32::setExt5vEnable(bool enable)
{
    _ext_5v_enable = enable;
    mclog::tagInfo(_tag, "set ext 5v enable: {}", _ext_5v_enable);
    bsp_set_ext_5v_en(_ext_5v_enable);
}

bool HalEsp32::getExt5vEnable()
{
    return _ext_5v_enable;
}

void HalEsp32::powerOff()
{
    mclog::tagInfo(_tag, "power off");

    playShutdownSfx();
    setDisplayBrightness(0);

    delay(100);
    while (1) {
        if (getMusicPlayTestState() == hal::HalBase::MUSIC_PLAY_IDLE) {
            break;
        }
        delay(100);
    }

    bsp_generate_poweroff_signal();
}

void HalEsp32::sleepAndTouchWakeup()
{
    mclog::tagInfo(_tag, "sleep and touch wakeup");

    lvglLock();
    auto brightness = getDisplayBrightness();
    setDisplayBrightness(0);

    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint16_t touch_strength[1];
    uint8_t touch_cnt = 0;

    esp_lcd_touch_handle_t touch_handle = bsp_display_get_touch_handle();

    while (1) {
        esp_lcd_touch_read_data(touch_handle);
        bool touchpad_pressed =
            esp_lcd_touch_get_coordinates(touch_handle, touch_x, touch_y, touch_strength, &touch_cnt, 1);
        // mclog::tagInfo(_tag, "touchpad pressed: {}", touchpad_pressed);
        if (!touchpad_pressed) {
            break;
        }
        delay(100);
    }

    while (1) {
        esp_lcd_touch_read_data(touch_handle);
        bool touchpad_pressed =
            esp_lcd_touch_get_coordinates(touch_handle, touch_x, touch_y, touch_strength, &touch_cnt, 1);
        // mclog::tagInfo(_tag, "touchpad pressed: {}", touchpad_pressed);
        if (touchpad_pressed) {
            break;
        }
        delay(100);
    }

    // esp_restart();

    lvglUnlock();
    delay(500);
    setDisplayBrightness(brightness);
}

void HalEsp32::sleepAndRtcWakeup()
{
    mclog::tagInfo(_tag, "start sleep and rtc wakeup");

    clearRtcIrq();
    clearImuIrq();

    delay(200);

    mclog::tagInfo(_tag, "set rtc alarm");
    struct tm time;
    rx8130.getTime(&time);
    time.tm_hour = 0;
    time.tm_min  = 1;
    time.tm_sec  = 49;
    rx8130.setTime(&time);
    time.tm_hour = 0;
    time.tm_min  = 2;
    time.tm_sec  = 0;
    rx8130.setAlarmIrq(&time);

    // delay(800);
    powerOff();
    while (1) {
        delay(100);
    }
}
