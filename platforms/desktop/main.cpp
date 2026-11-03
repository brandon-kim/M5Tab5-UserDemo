/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal/hal_desktop.h"
#include <app.h>
#include <memory>
#include <hal/hal.h>

int main()
{
    // Application layer initialization callback
    app::InitCallback_t callback;

    callback.onHalInjection = []() {
        // Inject hardware abstraction for desktop platform
        hal::Inject(std::make_unique<HalDesktop>());
    };

    // Initialize application layer with the callback
    app::Init(callback);
    while (!app::IsDone()) {
        app::Update();
    }
    app::Destroy();

    return 0;
}
