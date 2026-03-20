/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "view/view.h"
#include <mooncake.h>
#include <memory>

/**
 * @brief Derived App
 *
 */
class AppLauncher : public mooncake::AppAbility {
public:
    AppLauncher();

    // Override lifecycle callbacks
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    std::unique_ptr<launcher_view::LauncherView> _view;
};
