/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_template.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>

using namespace mooncake;

AppTemplate::AppTemplate()
{
    // Configure App name
    setAppInfo().name = "AppTemplate";
    // Configure App icon
    // setAppInfo().icon = (void*)&icon_app_dummy;
}

void AppTemplate::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");
}

void AppTemplate::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");
}

void AppTemplate::onRunning()
{
    // mclog::tagInfo(getAppInfo().name, "on running");
}

void AppTemplate::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
}
