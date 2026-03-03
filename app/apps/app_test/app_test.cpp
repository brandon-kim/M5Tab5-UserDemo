/**
 * @file app_test.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2025-12-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app_test.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>

using namespace mooncake;

AppTest::AppTest()
{
    // 配置 App 信息
    setAppInfo().name = "AppTest";
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

    _view = std::make_unique<test_view::TestView>();
    _view->init();
}

void AppTest::onRunning()
{
    _view->update();
}

void AppTest::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
    _view.reset();
}