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
#include <smooth_ui_toolkit.hpp>
#include <uitk/short_namespace.hpp>
#include <smooth_lvgl.hpp>
#include <vector>

namespace view {

/**
 * @brief Card-based App Launcher View
 * Displays installed apps as scrollable cards
 */
class LauncherView {
public:
    ~LauncherView();

    void init(std::vector<mooncake::AppProps_t> appProps);
    void update();
     int consumeSelectedAppId();

private:
    int _selected_app_id = -1;
    bool _is_stacked = false;
    lv_obj_t* _card_container = nullptr;
    std::vector<mooncake::AppProps_t> _app_list;
    
    void _create_app_cards();
    void _load_installed_apps();
    static void _card_event_handler(lv_event_t* e);
};

}  // namespace view
