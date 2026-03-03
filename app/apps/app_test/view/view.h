/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <cstdint>
#include <memory>
#include <lvgl.h>
#include <mooncake.h>
#include <apps/utils/ui/window.h>
#include <smooth_ui_toolkit.hpp>
#include <smooth_lvgl.hpp>
#include <vector>

namespace test_view {

/**
 * @brief  App Test View
 */
class TestView {
public:
    void init();
    void update();

private:
    bool _is_stacked = false;
    lv_obj_t* _card_container = nullptr;

    void _create_app_cards();
    void _load_installed_apps();
    static void _card_event_handler(lv_event_t* e);
};

}  // namespace test_view
