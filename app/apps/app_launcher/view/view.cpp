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
#include <smooth_ui_toolkit.hpp>
#include <smooth_lvgl.hpp>
#include <apps/utils/audio/audio.h>
#include <cstdint>

using namespace launcher_view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "launcher-view";

void LauncherView::init()
{
    mclog::tagInfo(_tag, "init launcher view with card layout");
    ui::signal_window_opened().clear();
    ui::signal_window_opened().connect([&](bool opened) { _is_stacked = opened; });

    LvglLockGuard lock;

    // Base screen
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    
    // Create card container (horizontal scrollable)
    _card_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_card_container, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(_card_container, 0, 0);
    lv_obj_set_flex_flow(_card_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_card_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(_card_container, 10, 0);
    lv_obj_set_style_pad_gap(_card_container, 15, 0);
    lv_obj_set_style_bg_opa(_card_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(_card_container, 0, 0);
    
    // Load and create app cards
    _load_installed_apps();
    _create_app_cards();
}

int LauncherView::consumeSelectedAppId()
{
    int id = _selected_app_id;
    _selected_app_id = -1;
    return id;
}

void LauncherView::_load_installed_apps()
{
    mclog::tagInfo(_tag, "loading installed apps");
    
    _app_list.clear();
    
    // Get all installed apps from Mooncake
    auto& mc = mooncake::GetMooncake();
    _app_list = mc.getAllAppProps();
    
    mclog::tagInfo(_tag, "Loaded %d apps", (int)_app_list.size());
    
    for (auto& app_prop : _app_list) {
        mclog::tagInfo(_tag, "Added app: %s (ID: %d)", app_prop.info.name.c_str(), app_prop.appID);
    }
}

void LauncherView::_create_app_cards()
{
    mclog::tagInfo(_tag, "creating app cards");
    
    for (auto& app_prop : _app_list) {
        // Skip AppLauncher itself to avoid recursion
        if (app_prop.info.name == "AppLauncher") {
            continue;
        }
        
        // Create card
        lv_obj_t* card = lv_obj_create(_card_container);
        lv_obj_set_size(card, 120, 140);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x2C3E50), 0);
        lv_obj_set_style_border_width(card, 2, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x3498DB), 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        
        // Store app ID in user data
        lv_obj_set_user_data(card, (void*)(intptr_t)app_prop.appID);
        
        // Add event listener
        lv_obj_add_event_cb(card, _card_event_handler, LV_EVENT_CLICKED, this);
        
        // Create flexbox for card content
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        
        // App icon
        lv_obj_t* icon = lv_label_create(card);
        if (app_prop.info.icon) {
            lv_label_set_text(icon, (const char*)app_prop.info.icon);
        } else {
            lv_label_set_text(icon, "?");
        }
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFF), 0);
        
        // App name label
        lv_obj_t* label = lv_label_create(card);
        lv_label_set_text(label, app_prop.info.name.c_str());
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        
        mclog::tagInfo(_tag, "Created card for app: %s (ID: %d)", app_prop.info.name.c_str(), app_prop.appID);
    }
}

void LauncherView::_card_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = (lv_obj_t*)lv_event_get_target(e);
    auto* self = (LauncherView*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        // Get app ID from card user data
        int app_id = (int)(intptr_t)lv_obj_get_user_data(target);
        mclog::tagInfo(_tag, "Card clicked: app ID = %d", app_id);
        self->_selected_app_id = app_id;
    }
}

void LauncherView::update()
{
    LvglLockGuard lock;
    // Update logic if needed
}

LauncherView::~LauncherView()
{
    LvglLockGuard lock;
    if (_card_container) {
        lv_obj_delete(_card_container);  // child objects will be deleted automatically
        _card_container = nullptr;
    }
}

