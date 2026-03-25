/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <mooncake.h>
#include <memory>
#include <hal/hal.h>
#include "app_template/app_template.h"
#include "app_launcher/app_launcher.h"
#include "app_demo/app_demo.h"
#include "app_startup_anim/app_startup_anim.h"
#include "app_test/app_test.h"
#include "app_no_smooth/app_no_smooth.h"
/* Header files locator (Don't remove) */

// Start boot anim app and wait for it to finish
inline void on_startup_anim()
{
    auto app_id = mooncake::GetMooncake().installApp(std::make_unique<AppStartupAnim>());
    mooncake::GetMooncake().openApp(app_id);
    while (1) {
        mooncake::GetMooncake().update();
        if (mooncake::GetMooncake().getAppCurrentState(app_id) == mooncake::AppAbility::StateSleeping) {
            break;
        }
        GetHAL()->delay(1);
    }
    mooncake::GetMooncake().uninstallApp(app_id);
}

/**
 * @brief App install callback (App Installation Callback)
 * Plan to add a new AppLauncher when time permits
 *
 * @param mooncake
 */
inline void on_install_apps()
{
    // Install App
    // Currently using card-based AppLauncher as the default launcher
    // mooncake::GetMooncake().installApp(std::make_unique<AppTemplate>());
    mooncake::GetMooncake().installApp(std::make_unique<AppLauncher>());  // Card-based app launcher
    mooncake::GetMooncake().installApp(std::make_unique<AppDemo>());
    mooncake::GetMooncake().installApp(std::make_unique<AppTest>());
    mooncake::GetMooncake().installApp(std::make_unique<AppNoSmooth>());
    /* Install app locator (Don't remove) */
}
