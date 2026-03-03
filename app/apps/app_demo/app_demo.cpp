/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_demo.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <smooth_lvgl.hpp>
#include <assets/assets.h>

using namespace mooncake;

AppDemo::AppDemo()
{
    setAppInfo().name = "AppDemo";
    setAppInfo().icon = (void*)LV_SYMBOL_IMAGE;  // LVGL 내장 심볼 (예시)
}

void AppDemo::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");

    // App is ready, but won't auto-open
    // Will be opened by AppLauncher when user clicks the card
}

void AppDemo::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    _view = std::make_unique<demo_view::LauncherView>();
    _view->init();
}

void AppDemo::onRunning()
{
    _view->update();
}

void AppDemo::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    _view.reset();
}
