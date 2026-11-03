/**
 * @file app_test.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2025-12-18
 *
 */
#include "app_test.h"

#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
//#include <smooth_lvgl.hpp>
#include <assets/assets.h>

using namespace mooncake;

AppTest::AppTest()
{
    // Configure App Info
    setAppInfo().name = "App Test";
    setAppInfo().icon = (void*)LV_SYMBOL_FILE;
}

AppTest::~AppTest()
{
}

void AppTest::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");

    // App is ready, but won't auto-open
    // Will be opened by AppLauncher when user clicks the card
}


void AppTest::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    _view = std::make_unique<view::TestView>();
    _view->init();
    _view->setOnQuit([this]() {
        close();
    });

}

static uint32_t _time_count;
void AppTest::onRunning()
{
    _view->update();

    // Print "hi" every 1 second
    if (GetHAL()->millis() - _time_count > 1000) {
        mclog::tagInfo(getAppInfo().name, "hi");
        _time_count = GetHAL()->millis();
    }    
}

void AppTest::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
    _view.reset();
}