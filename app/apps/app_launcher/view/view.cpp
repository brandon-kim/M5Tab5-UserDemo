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
#include <src/core/lv_obj.h>
#include <cstdint>

using namespace view;
using namespace smooth_ui_toolkit;
using namespace smooth_ui_toolkit::lvgl_cpp;

static const std::string _tag = "launcher-view";

LauncherView::~LauncherView()
{
#if APP_LAUNCHER_EXIT_ANIMATION
    lv_anim_delete(_base, _exitAnimationExec);
#endif
    if (_base) {
        lv_obj_delete(_base);  // child objects will be deleted automatically
        _base = nullptr;
    }
}

// Home background image (1280x720 RGB565) defined in app/assets/images/home_bg.c
// (LVGL image descriptors use a plain top-level symbol — same name in C and C++.)
extern const lv_image_dsc_t home_bg;
static constexpr int BTN_H      = 100;
static constexpr int BTN_W      = 140;
static constexpr int BTN_GAP    = 30;
static constexpr int SIDE_PAD   = 60;   // left/right margin

lv_obj_t* make_card(lv_obj_t* parent, int w, int h, const mooncake::AppProps_t& app_prop)
{
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);

    // Electric-blue gradient pill matching the background artwork's palette.
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E6FC2), 0);          // top: electric blue
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x0A2A54), 0);     // bottom: deep navy
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);

    // Pressed: brighten the whole gradient.
    lv_obj_set_style_bg_color(card, lv_color_hex(0x3A9BE8), LV_STATE_PRESSED);
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x14467E), LV_STATE_PRESSED);
    lv_obj_set_style_radius(card, 22, 0);

    // Cyan neon edge + soft outer glow.
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x5BD5FF), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_80, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x2E8BE6), 0);
    lv_obj_set_style_shadow_width(card, 18, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_40, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        
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
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    // Subtle dark text shadow for legibility over the bright gradient.
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);


    return card;
}

void LauncherView::init(std::vector<mooncake::AppProps_t> appProps)
{
    mclog::tagInfo(_tag, "init");


    // Base screen
    lv_obj_t *scr = lv_screen_active();
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Base object 
    _base = lv_obj_create(scr);
    lv_obj_remove_style_all(_base);
    lv_obj_set_size(_base, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(_base, 0, 0);

    // Full-screen background image (created first → stays at the bottom layer).
    lv_obj_t *bg = lv_image_create(_base);
    lv_image_set_src(bg, &home_bg);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_remove_flag(bg, LV_OBJ_FLAG_CLICKABLE);

    // Create card container (horizontal scrollable)
    lv_obj_t* container = nullptr;    
    container = lv_obj_create(_base);
    lv_obj_set_size(container, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(container, 0, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(container, SIDE_PAD, 0);
    lv_obj_set_style_pad_gap(container, BTN_GAP, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(container, 0, 0);
 
    // Load and create app cards
    mclog::tagInfo(_tag, "creating app cards");
    
    for (auto& app_prop : appProps) {        
        // Create card
        lv_obj_t* card = make_card(container, BTN_W, BTN_H, app_prop);

        // Store app ID in user data
        lv_obj_set_user_data(card, (void*)(intptr_t)app_prop.appID);

        // Add event listener
        lv_obj_add_event_cb(card, _card_event_handler, LV_EVENT_CLICKED, this);
        
        // Create flexbox for card content
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        mclog::tagInfo(_tag, "Created card for app: {} (ID: {})", app_prop.info.name, app_prop.appID);
    }

    // ── Top system status bar (clock / WiFi / battery) ──
    lv_obj_t* sbar = lv_obj_create(_base);
    lv_obj_set_pos(sbar, 0, 0);
    lv_obj_set_size(sbar, 1280, 56);
    lv_obj_set_style_bg_color(sbar, lv_color_hex(0x07111E), 0);
    lv_obj_set_style_bg_opa(sbar, LV_OPA_60, 0);   // semi-transparent over the artwork
    lv_obj_set_style_border_width(sbar, 0, 0);
    lv_obj_set_style_radius(sbar, 0, 0);
    lv_obj_set_style_pad_all(sbar, 0, 0);
    lv_obj_remove_flag(sbar, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

    lv_obj_t* status_time = lv_label_create(sbar);
    lv_obj_set_style_text_font(status_time, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(status_time, lv_color_hex(0x4FA3FF), 0);
    lv_label_set_text(status_time, "--:--");
    lv_obj_align(status_time, LV_ALIGN_LEFT_MID, 28, 0);    

    // Date — to the right of the clock, same size as time (28px Chinese font).
    lv_obj_t* status_date = lv_label_create(sbar);
    lv_obj_set_style_text_font(status_date, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(status_date, lv_color_hex(0x4FA3FF), 0);
    lv_label_set_text(status_date, "");
    lv_obj_align(status_date, LV_ALIGN_LEFT_MID, 138, 0);


}

void LauncherView::update()
{
    
}
int LauncherView::consumeSelectedAppId()
{
    int id = _selected_app_id;
    _selected_app_id = -1;
    return id;
}

#if APP_LAUNCHER_EXIT_ANIMATION
void LauncherView::animateOut(std::function<void()> completed)
{
    if (_exit_animating || !_base) return;

    _exit_animating = true;
    _exit_completed = std::move(completed);

    lv_anim_delete(_base, _exitAnimationExec);

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, _base);
    lv_anim_set_exec_cb(&anim, _exitAnimationExec);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&anim, 180);
    lv_anim_set_completed_cb(&anim, _exitAnimationCompleted);
    lv_anim_set_user_data(&anim, this);
    lv_anim_start(&anim);
}

void LauncherView::_exitAnimationExec(void* var, int32_t value)
{
    lv_obj_set_style_opa(static_cast<lv_obj_t*>(var), static_cast<lv_opa_t>(value), 0);
}

void LauncherView::_exitAnimationCompleted(lv_anim_t* anim)
{
    auto* self = static_cast<LauncherView*>(lv_anim_get_user_data(anim));
    if (!self || !self->_exit_completed) return;

    auto completed = std::move(self->_exit_completed);
    self->_exit_completed = {};
    completed();
}
#endif

void LauncherView::_card_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = (lv_obj_t*)lv_event_get_target(e);
    auto* self = (LauncherView*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        // Get app ID from card user data
        int app_id = (int)(intptr_t)lv_obj_get_user_data(target);
        mclog::tagInfo(_tag, "Card clicked: app ID = {}", app_id);
        self->_selected_app_id = app_id;
    }
}
