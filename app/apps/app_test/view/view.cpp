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

    // Base container 
#if USE_APP_LOADING_ANIMATION  > 0
    lv_obj_t *new_scr = lv_obj_create(NULL);
    _container = std::make_unique<ui::PageBaseContainer>(new_scr);
#else
    _container = std::make_unique<ui::PageBaseContainer>();
#endif
    lv_obj_set_style_bg_color(_container->get(), lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_opa(_container->get(), LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(_container->get(), 10, 0);
    lv_obj_set_style_pad_right(_container->get(), 15, 0);

    // test code
    _label = std::make_unique<uitk::lvgl_cpp::Label>(_container->get());
    _label->setText("Hello, AppTest!\nSwipe Up to Close");
    _label->align(LV_ALIGN_CENTER, 0, 0);
    _label->setTextColor(lv_color_hex(0xFFFFFF));
    _label->setTextFont(&lv_font_montserrat_14, 0);

#if USE_APP_LOADING_ANIMATION  > 0    
    //load screen
    lv_screen_load_anim(new_scr, LV_SCREEN_LOAD_ANIM_OVER_TOP, 300, 0, true);
#endif
}


void TestView::update()
{
    // Update logic if needed
}

