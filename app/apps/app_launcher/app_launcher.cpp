/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_launcher.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <smooth_lvgl.hpp>

using namespace mooncake;

void AppLauncher::onLauncherCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");

    open();
}

void AppLauncher::onLauncherOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    //LvglLockGuard lock;
    _view = std::make_unique<view::LauncherView>();
    _view->init(getAppProps());
	
}

void AppLauncher::onLauncherRunning()
{
    _view->update();

     int selected = _view->consumeSelectedAppId();
    if (selected >= 0) {
        auto& mc = mooncake::GetMooncake();
        if (mc.isAppExist(selected)) {
            mclog::tagInfo(getAppInfo().name, "Opening app ID: {}", selected);
            mc.openApp(selected);
            // no app switching isn't implemented yet, so close app launcher after click
            mclog::tagInfo(getAppInfo().name, "Closing AppLauncher");
            close();  
        }
    }

}

void AppLauncher::onLauncherClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    //LvglLockGuard lock;

    _view.reset();
}

void AppLauncher::onLauncherDestroy()
{
    mclog::tagInfo(getAppInfo().name, "on destroy");
}
