/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_launcher.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>

using namespace mooncake;

void AppLauncher::onLauncherCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");

    open();
}

void AppLauncher::onLauncherOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

#if APP_LAUNCHER_EXIT_ANIMATION
    _app_open_transition_started = false;
#endif
    _view = std::make_unique<view::LauncherView>();
    _view->init(getAppProps());
}

void AppLauncher::onLauncherRunning()
{
    _view->update();
#if APP_LAUNCHER_EXIT_ANIMATION
    if (_app_open_transition_started) return;
#endif

    const int appID = _view->consumeSelectedAppId();
    if (appID >= 0) {
        mclog::tagInfo(getAppInfo().name, "handle open app, app id: {}", appID);
#if APP_LAUNCHER_EXIT_ANIMATION
        _app_open_transition_started = true;
        _view->animateOut([this, appID]() {
            openApp(appID);
        });
#else
        openApp(appID);
#endif
    }
}

void AppLauncher::onLauncherClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    _view.reset();
}

void AppLauncher::onLauncherDestroy()
{
    mclog::tagInfo(getAppInfo().name, "on destroy");
}
