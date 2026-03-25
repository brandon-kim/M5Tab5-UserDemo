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
    // Clear callbacks to avoid accidental calls
    _on_quit = nullptr;

    if (_container) {
        lv_obj_delete(_container);
        _container = nullptr;
    }
}

void NoSmoothView::init()
{
    mclog::tagInfo(_tag, "init");

    LvglLockGuard lock;

    // Base screen
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    
    // Create card container (horizontal scrollable)
    _container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_container, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_pad_all(_container, 10, 0);
    lv_obj_set_style_bg_color(_container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_COVER, 0);

    // test code
    lv_obj_t *label = lv_label_create(_container);
    lv_label_set_text(label, "Hello, AppTest!");
    lv_obj_set_pos(label, 10, 10);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);

    // Create a quit button
    lv_obj_t *button_quit = lv_btn_create(_container);
    lv_obj_align(button_quit, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_t *label_quit = lv_label_create(button_quit);
    lv_label_set_text(label_quit, "QUIT");
    lv_obj_center(label_quit);
    lv_obj_add_event_cb(button_quit, [](lv_event_t * e) {
        NoSmoothView *view = static_cast<NoSmoothView *>(lv_event_get_user_data(e));
        if (view->_on_quit) {  view->_on_quit();}
    }, LV_EVENT_CLICKED, this);


}


void NoSmoothView::update()
{
    LvglLockGuard lock;
    // Update logic if needed
}



