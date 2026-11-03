/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal/hal_esp32.h"
#include <app.h>
#include <hal/hal.h>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" void app_main(void)
{
    // Application layer initialization callback
    app::InitCallback_t callback;

    callback.onHalInjection = []() {
        // Inject hardware abstraction for desktop platform
        hal::Inject(std::make_unique<HalEsp32>());
    };

    // Start application layer
    app::Init(callback);
    while (!app::IsDone()) {
        app::Update();
        vTaskDelay(1);
    }
    app::Destroy();
}
