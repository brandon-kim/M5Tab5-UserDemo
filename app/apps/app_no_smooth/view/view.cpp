/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <hal/hal.h>
#include <mooncake_log.h>
#include <assets/assets.h>

using namespace view;

static const std::string _tag = "no-smooth-view";

NoSmoothView::~NoSmoothView()
{
    LvglLockGuard lock;
    if (_container) {
        _container->removeSwipeGesture();
        lv_obj_clean(_container->get());
    }
}

void NoSmoothView::init()
{
    mclog::tagInfo(_tag, "init");

    LvglLockGuard lock;

    _container = std::make_unique<ui::PageBaseContainer>();
    lv_obj_t *base = _container->get();
    lv_obj_set_style_pad_all(base, 10, 0);
    lv_obj_set_style_bg_color(base, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);

    lv_obj_t* label = lv_label_create(base);
    lv_label_set_text(label, "Hello, AppTest!\nSwipe Up to Close");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);

}


void NoSmoothView::update()
{
    LvglLockGuard lock;
    // Update logic if needed
}



