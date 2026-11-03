#include "app_idle_screen.h"
#include <hal/hal.h>
#include <cstdio>
#include <ctime>
#include <mooncake_log.h>
//#include "../app_home/app_home.h"
#include <apps/app_voice_input/app_voice_input.h>
#include <apps/utils/ui/app_theme.h>


static constexpr int SCREEN_W = 1280;
static constexpr int SCREEN_H = 720;
static constexpr uint32_t C_BG   = 0x000000;
static constexpr uint32_t C_TEXT = 0xEAF3FB;
static constexpr uint32_t C_DIM  = 0x8FA8C4;

// Group box the clock/date/weather move around inside — kept well clear of
// the screen edges (burn-in guard: OLED/LCD panels showing the same static
// bright text in the same spot for hours can leave a faint ghost image).
static constexpr int GROUP_W = 600;
static constexpr int GROUP_H = 170;
static constexpr int MARGIN  = 60;
static const struct { int x, y; } POSITIONS[] = {
    {(SCREEN_W - GROUP_W) / 2, (SCREEN_H - GROUP_H) / 2},  // center
    {MARGIN,                    MARGIN},                     // top-left
    {SCREEN_W - GROUP_W - MARGIN, MARGIN},                    // top-right
    {MARGIN,                    SCREEN_H - GROUP_H - MARGIN}, // bottom-left
    {SCREEN_W - GROUP_W - MARGIN, SCREEN_H - GROUP_H - MARGIN}, // bottom-right
};
static constexpr int POSITION_COUNT = sizeof(POSITIONS) / sizeof(POSITIONS[0]);

static const std::string _tag = "idle-screen";

void AppIdleScreen::onCreate() {
    mclog::tagInfo(_tag, "idle screen created");
}

void AppIdleScreen::onResume() {}

void AppIdleScreen::dismiss()
{
    lv_display_trigger_activity(NULL);
    _hide();
}

void AppIdleScreen::onPause()
{
    _hide();
}

void AppIdleScreen::onRunning()
{
    // Don't cover an in-progress voice-input session (recording / result overlay).
    if (AppVoiceInput::isMicActive()) {
        if (_overlay) _hide();
        return;
    }

    uint32_t idle_ms = lv_display_get_inactive_time(nullptr);
    if (!_overlay) {
        if (idle_ms >= IDLE_MS) _show();
        return;
    }

    // Any touch resets LVGL's inactivity timer for the whole display —
    // including a tap on the overlay itself — so a small idle_ms here means
    // "something was touched since we showed it." The overlay's own click
    // handler already hides it instantly; this is just a safety net in case
    // that somehow didn't fire.
    if (idle_ms < IDLE_MS) {
        _hide();
        return;
    }
    _updateClock();
}

void AppIdleScreen::_show()
{
    if (_overlay) return;

    _overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_overlay, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(_overlay, 0, 0);
    lv_obj_set_style_bg_color(_overlay, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_overlay, 0, 0);
    lv_obj_set_style_radius(_overlay, 0, 0);
    lv_obj_set_style_pad_all(_overlay, 0, 0);
    lv_obj_clear_flag(_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(_overlay);

    // Time/date/weather live inside _group so they move together — repositioning
    // just moves this one container, not three separately-aligned labels.
    _group = lv_obj_create(_overlay);
    lv_obj_set_size(_group, GROUP_W, GROUP_H);
    lv_obj_set_style_bg_opa(_group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_group, 0, 0);
    lv_obj_set_style_pad_all(_group, 0, 0);
    lv_obj_clear_flag(_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_group, LV_OBJ_FLAG_CLICKABLE);  // clicks pass through to _overlay

    _time_lbl = lv_label_create(_group);
    lv_obj_set_style_text_font(_time_lbl, &lv_font_montserrat_44, 0);
    lv_obj_set_style_text_color(_time_lbl, lv_color_hex(C_TEXT), 0);
    lv_obj_align(_time_lbl, LV_ALIGN_TOP_MID, 0, 0);

    _date_lbl = lv_label_create(_group);
    lv_obj_set_style_text_font(_date_lbl, app_theme::font_large(), 0);
    lv_obj_set_style_text_color(_date_lbl, lv_color_hex(C_DIM), 0);
    lv_obj_align(_date_lbl, LV_ALIGN_TOP_MID, 0, 70);

    _weather_lbl = lv_label_create(_group);
    lv_obj_set_style_text_font(_weather_lbl, app_theme::font_large(), 0);
    lv_obj_set_style_text_color(_weather_lbl, lv_color_hex(C_DIM), 0);
    lv_obj_align(_weather_lbl, LV_ALIGN_TOP_MID, 0, 116);

    // Touch anywhere on the overlay dismisses it immediately (PRESSED, not
    // CLICKED, so it reacts on touch-down rather than waiting for release).
    lv_obj_add_event_cb(_overlay, [](lv_event_t* e) {
        static_cast<AppIdleScreen*>(lv_event_get_user_data(e))->_hide();
    }, LV_EVENT_PRESSED, this);

    _pos_index = 0;
    lv_obj_set_pos(_group, POSITIONS[0].x, POSITIONS[0].y);
    _last_clock_update_ms = 0;  // force immediate first render
    _last_move_ms = GetHAL()->millis();
    _updateClock();
}

void AppIdleScreen::_hide()
{
    if (!_overlay) return;
    lv_obj_delete(_overlay);
    _overlay = _group = _time_lbl = _date_lbl = _weather_lbl = nullptr;
}

void AppIdleScreen::_reposition()
{
    _pos_index = (_pos_index + 1) % POSITION_COUNT;
    lv_obj_set_pos(_group, POSITIONS[_pos_index].x, POSITIONS[_pos_index].y);
}

void AppIdleScreen::_updateClock()
{
    uint32_t now = GetHAL()->millis();
    if (_last_clock_update_ms != 0 && now - _last_clock_update_ms < 1000) return;
    _last_clock_update_ms = now;

    if (now - _last_move_ms >= MOVE_MS) {
        _last_move_ms = now;
        _reposition();
    }

    time_t t = time(nullptr);
    struct tm lt = {};
#if defined(_WIN32)
    if (localtime_s(&lt, &t) != 0) return;
#else
    localtime_r(&t, &lt);
#endif

    char tbuf[8];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d", lt.tm_hour, lt.tm_min);
    lv_label_set_text(_time_lbl, tbuf);

    static const char* const wd[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char* const months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    char dbuf[40];
    snprintf(dbuf, sizeof(dbuf), "%s %d %s", months[lt.tm_mon], lt.tm_mday, wd[lt.tm_wday]);
    lv_label_set_text(_date_lbl, dbuf);

#if 0//not implemented yet
    std::string temp, cond;
    if (_home) _home->getWeather(temp, cond);
    if (!temp.empty() || !cond.empty()) {
        char wbuf[48];
        snprintf(wbuf, sizeof(wbuf), "%s  %s", cond.c_str(), temp.c_str());
        lv_label_set_text(_weather_lbl, wbuf);
    } else {
        lv_label_set_text(_weather_lbl, "");
    }
#endif    
}
