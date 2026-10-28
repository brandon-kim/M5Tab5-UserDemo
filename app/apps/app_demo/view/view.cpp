/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <lvgl.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <assets/assets.h>
#include <apps/utils/audio/audio.h>

using namespace view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "demo-view";

AppDemoView::~AppDemoView()
{
    _panels.clear();
    _img_bg.reset();
    if (_container) {
        _container->removeSwipeGesture();
        lv_obj_clean(_container->get());
    }
}

void AppDemoView::init()
{
    mclog::tagInfo(_tag, "init");

    ui::signal_window_opened().clear();
    ui::signal_window_opened().connect([&](bool opened) { _is_stacked = opened; });


    _container = std::make_unique<ui::PageBaseContainer>();

    lv_obj_t* base = _container->get();
    lv_obj_remove_flag(base, LV_OBJ_FLAG_SCROLLABLE);

    // Background image
    _img_bg = std::make_unique<Image>(_container->get());
    _img_bg->setAlign(LV_ALIGN_CENTER);
    _img_bg->setSrc(&launcher_bg);

    // Install panels
    _panels.push_back(std::make_unique<PanelRtc>());
    _panels.push_back(std::make_unique<PanelLcdBacklight>());
    _panels.push_back(std::make_unique<PanelSpeakerVolume>());
    _panels.push_back(std::make_unique<PanelPowerMonitor>());
    _panels.push_back(std::make_unique<PanelImu>());
    _panels.push_back(std::make_unique<PanelSwitches>());
    _panels.push_back(std::make_unique<PanelPower>());
    _panels.push_back(std::make_unique<PanelCamera>());
    _panels.push_back(std::make_unique<PanelDualMic>());
    _panels.push_back(std::make_unique<PanelHeadphone>());
    _panels.push_back(std::make_unique<PanelSdCard>());
    _panels.push_back(std::make_unique<PanelI2cScan>());
    _panels.push_back(std::make_unique<PanelGpioTest>());
    _panels.push_back(std::make_unique<PanelMusic>());
    _panels.push_back(std::make_unique<PanelComMonitor>());



    for (auto& panel : _panels) {
        panel->init(_container->get());
    }

}

void AppDemoView::update()
{

    for (auto& panel : _panels) {
        panel->update(_is_stacked);
    }
}
