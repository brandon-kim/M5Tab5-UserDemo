/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
//#include <smooth_ui_toolkit.hpp>
//#include <uitk/short_namespace.hpp>
//#include <smooth_lvgl.hpp>
#include <lvgl.h>

#ifndef APP_LAUNCHER_EXIT_ANIMATION
#define APP_LAUNCHER_EXIT_ANIMATION 1
#endif

#if APP_LAUNCHER_EXIT_ANIMATION
#include <functional>
#endif

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
#if APP_LAUNCHER_EXIT_ANIMATION
    void animateOut(std::function<void()> completed);
#endif

private:
    int _selected_app_id = -1;
    lv_obj_t* _base = nullptr;

    static void _card_event_handler(lv_event_t* e);
#if APP_LAUNCHER_EXIT_ANIMATION
    bool _exit_animating = false;
    std::function<void()> _exit_completed;
    static void _exitAnimationExec(void* var, int32_t value);
    static void _exitAnimationCompleted(lv_anim_t* anim);
#endif
};

}  // namespace view
