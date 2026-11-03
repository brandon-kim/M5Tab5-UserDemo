/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#ifndef APP_LAUNCHER_EXIT_ANIMATION
#define APP_LAUNCHER_EXIT_ANIMATION 1
#endif

#include "view/view.h"
#include <mooncake.h>
#include <mooncake_templates.h>
#include <cstdint>
#include <memory>

/**
 * @brief Derived App
 *
 */
class AppLauncher : public mooncake::templates::AppLauncherBase {
public:
    void onLauncherCreate() override;
    void onLauncherOpen() override;
    void onLauncherRunning() override;
    void onLauncherClose() override;
    void onLauncherDestroy() override;

private:
    std::unique_ptr<view::LauncherView> _view;
#if APP_LAUNCHER_EXIT_ANIMATION
    bool _app_open_transition_started = false;
#endif
};
