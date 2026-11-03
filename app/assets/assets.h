/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <lvgl.h>
//#include <string_view>

LV_IMG_DECLARE(launcher_bg);
LV_IMG_DECLARE(sw_chg_off);
LV_IMG_DECLARE(sw_chg_on);
LV_IMG_DECLARE(sw_off);
LV_IMG_DECLARE(sw_on);
LV_IMG_DECLARE(sw_qc_off);
LV_IMG_DECLARE(sw_qc_on);
LV_IMG_DECLARE(sw_rf_h);
LV_IMG_DECLARE(sw_rf_l);
LV_IMG_DECLARE(arrow_state_on);
LV_IMG_DECLARE(mouse_cursor);
LV_IMG_DECLARE(internal_i2c_dev_chart);
LV_IMG_DECLARE(porta_i2c_dev_chart);
LV_IMG_DECLARE(porta_i2c_ext5v_on);
LV_IMG_DECLARE(logo_tab);
LV_IMG_DECLARE(logo_5);
LV_IMG_DECLARE(chg_arrow_down);
LV_IMG_DECLARE(chg_arrow_up);
extern const uint8_t canon_in_d_mp3_start[] asm("_binary_canon_in_d_mp3_start");
extern const uint8_t canon_in_d_mp3_end[] asm("_binary_canon_in_d_mp3_end");
extern const uint8_t startup_sfx_mp3_start[] asm("_binary_startup_sfx_mp3_start");
extern const uint8_t startup_sfx_mp3_end[] asm("_binary_startup_sfx_mp3_end");
extern const uint8_t shutdown_sfx_mp3_start[] asm("_binary_shutdown_sfx_mp3_start");
extern const uint8_t shutdown_sfx_mp3_end[] asm("_binary_shutdown_sfx_mp3_end");