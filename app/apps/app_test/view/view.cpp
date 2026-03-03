/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <assets/assets.h>
#include <smooth_ui_toolkit.hpp>
#include <smooth_lvgl.hpp>
#include <apps/utils/audio/audio.h>
#include <cstdint>

using namespace test_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "test-view";

void TestView::init()
{
    mclog::tagInfo(_tag, "init test view with card layout");

    ui::signal_window_opened().clear();
    ui::signal_window_opened().connect([&](bool opened) { _is_stacked = opened; });

    LvglLockGuard lock;

    // Base screen
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    
    // Create card container (horizontal scrollable)
    _card_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_card_container, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(_card_container, 0, 0);
    lv_obj_set_flex_flow(_card_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_card_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(_card_container, 10, 0);
    lv_obj_set_style_pad_gap(_card_container, 15, 0);
    lv_obj_set_style_bg_color(_card_container, lv_color_hex(0x1a1a1a), 0);  // 배경색 설정
    lv_obj_set_style_bg_opa(_card_container, LV_OPA_COVER, 0);  // 불투명으로 변경
    lv_obj_set_style_border_opa(_card_container, 0, 0);

    // test code
    lv_obj_t * label = lv_label_create(_card_container);
    lv_label_set_text(label, "Hello, AppTest!");
    

}


void TestView::update()
{
    LvglLockGuard lock;
    // Update logic if needed
}

