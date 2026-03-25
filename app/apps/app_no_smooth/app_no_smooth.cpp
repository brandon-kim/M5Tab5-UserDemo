/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_no_smooth.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <smooth_lvgl.hpp>

using namespace mooncake;

AppNoSmooth::AppNoSmooth()
{
    // Configure App Info
    setAppInfo().name = "AppNoSmooth";
    setAppInfo().icon = (void*)LV_SYMBOL_FILE;
}

AppNoSmooth::~AppNoSmooth() = default;

void AppNoSmooth::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");
}

void AppNoSmooth::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    _view = std::make_unique<view::NoSmoothView>();
    _view->init();
    _view->setOnQuit([this]() {
        close();
    });
}

static uint32_t _time_count;
void AppNoSmooth::onRunning()
{
    _view->update();

    // Print "hi" every 1 second
    if (GetHAL()->millis() - _time_count > 1000) {
        mclog::tagInfo(getAppInfo().name, "hi");
        _time_count = GetHAL()->millis();
    }    
}

void AppNoSmooth::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
    _view.reset();            
}
