/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_launcher.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>

static const std::string TAG = "launcher";

using namespace mooncake;

void AppLauncher::onLauncherCreate()
{
    mclog::tagInfo(TAG, "on create");

    open();
}

void AppLauncher::onLauncherOpen()
{
    mclog::tagInfo(TAG, "on open");

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
        mclog::tagInfo(TAG, "handle open app, app id: {}", appID);
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
    mclog::tagInfo(TAG, "on close");

    _view.reset();
}

void AppLauncher::onLauncherDestroy()
{
    mclog::tagInfo(TAG, "on destroy");
}
