/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <hal/hal.h>
#include <mooncake_log.h>
#include <assets/assets.h>
#include <apps/utils/audio/audio.h>

using namespace view;
using namespace uitk::lvgl_cpp;

static const std::string _tag = "test-view";

void TestView::init()
{
    mclog::tagInfo(_tag, "init test view with card layout");

    LvglLockGuard lock;

    // Base screen
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    
    // Create card container (horizontal scrollable)
    _container = std::make_unique<uitk::lvgl_cpp::Container>(lv_screen_active());
    _container->setSize(lv_pct(100), lv_pct(100));
    _container->setPos(0, 0);
    _container->setPadding(10, 0, 15, 0);
    _container->setBgColor(lv_color_hex(0x1a1a1a)); 
    _container->setBgOpa(LV_OPA_COVER, 0);  

    // test code
    _label = std::make_unique<uitk::lvgl_cpp::Label>(_container->get());
    _label->setText("Hello, AppTest!");
    _label->setPos(10, 10);
    _label->setTextColor(lv_color_hex(0xFFFFFF));
    _label->setTextFont(&lv_font_montserrat_14, 0);

    // Create a quit button
    _button_quit = std::make_unique<uitk::lvgl_cpp::Button>(_container->get());
    //_button_quit->setAlign(LV_ALIGN_CENTER);
    _button_quit->align(LV_ALIGN_TOP_RIGHT, 0, 0);
    _button_quit->label().setText("QUIT");
    _button_quit->onClick().connect([this]() {
        if (_on_quit) _on_quit();
    });


}


void TestView::update()
{
    LvglLockGuard lock;
    // Update logic if needed
}

