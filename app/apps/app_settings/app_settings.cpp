#include "app_settings.h"
#include <hal/hal.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <src/font/lv_font.h>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cctype>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>
#ifndef PLATFORM_BUILD_DESKTOP
#include <cbin_font.h>
#else
//extern "C" const lv_font_t font_puhui_20_4;
#endif

static const std::string _tag = "app-tools";

// Xiaozhi's puhui common 30px cbin font — GB2312 ~3500 glyphs, XIP from flash.
static const lv_font_t* zh_font_30()
{
#ifndef PLATFORM_BUILD_DESKTOP
    extern const uint8_t font_puhui_common_30_4_bin_start[] asm("_binary_font_puhui_common_30_4_bin_start");
    static lv_font_t* f = nullptr;
    if (!f) f = cbin_font_create((uint8_t*)font_puhui_common_30_4_bin_start);
    return f;
#else
    return &lv_font_montserrat_24;
#endif
}

// Font defined in app_ha/view/font_zh_36.c (includes the tools calculator glyphs).
extern lv_font_t font_zh_36;

// Tool tile icons (ARGB8888) defined in app_settings/calc_icon.c & fx_icon.c & unit_icon.c.
extern const lv_image_dsc_t calc_icon;   // 80x112
extern const lv_image_dsc_t fx_icon;     // 100x100
extern const lv_image_dsc_t unit_icon;   // 200x200
extern const lv_image_dsc_t lora_logo;   // 130x130
extern const lv_image_dsc_t stocks_icon; // 130x130
extern const lv_image_dsc_t reachy_icon; // 130x130 (app_reachy/reachy_icon.c)

// Calculator fonts (Arial Unicode subsets) defined in this app's font_calc_*.c.
extern const lv_font_t font_calc_big;   // 80px result line
extern const lv_font_t font_calc_op;    // 40px button labels (has × ÷)
extern const lv_font_t font_calc_expr;  // 30px expression line

// ── Palette ─────────────────────────────────────────────────────────────────
// Tools-page colors mirror the HA "Lights" dashboard cards (app_ha/view.cpp).
static constexpr uint32_t C_BG       = 0x0A1A2E;  // page background (HA C_BG)
static constexpr uint32_t C_ACCENT   = 0x4FA3FF;
static constexpr uint32_t C_CARD     = 0x13304E;  // HA card background
static constexpr uint32_t C_CARD_PR  = 0x1A3A5C;  // HA pressed fill (C_ACCENT_SOFT)
static constexpr uint32_t C_TEXT     = 0xEAF3FB;  // HA primary text

// Calculator palette (matches the reference iPad calculator).
static constexpr uint32_t CALC_BG   = 0x000000;
static constexpr uint32_t CALC_NUM  = 0x3A3A3C;  // dark grey digit keys
static constexpr uint32_t CALC_FUNC = 0x545456;  // medium grey  ⌫ / AC / %
static constexpr uint32_t CALC_OP   = 0xFF9F0A;  // orange operators
static constexpr uint32_t CALC_TXT  = 0xFFFFFF;
static constexpr uint32_t CALC_EXPR = 0x9A9AA0;  // grey expression line

// ── Currency table + live rate cache ─────────────────────────────────────────
namespace {
struct Ccy { const char* code; const char* name; double per_usd; };
// Static fallback rates (units per 1 USD); overwritten at runtime by the HA
// server's /rate proxy (see _fxFetch). Order must match g_fx_rate below.
const Ccy FX[] = {
    {"USD", "US Dollar",       1.00},
    {"CNY", "Chinese Yuan",    7.25},
    {"EUR", "Euro",            0.92},
    {"JPY", "Japanese Yen",    157.0},
    {"GBP", "British Pound",   0.79},
    {"HKD", "Hong Kong Dollar",7.81},
    {"KRW", "South Korean Won",1380.0},
    {"AUD", "Australian Dollar",1.51},
};
constexpr int FX_N = (int)(sizeof(FX) / sizeof(FX[0]));

// Live per-USD rates (seeded from the static table, overwritten by /rate). The
// detached fetch worker writes under g_fx_mutex; the UI reads under it too.
std::mutex        g_fx_mutex;
double            g_fx_rate[FX_N] = {1.00, 7.25, 0.92, 157.0, 0.79, 7.81, 1380.0, 1.51};
std::atomic<bool> g_fx_updated{false};   // set by worker, consumed in onRunning
std::atomic<bool> g_fx_fetching{false};
std::string       g_fx_source = "static";

// ── Email cache (worker thread fills, UI thread reads) ──────────────────────
// Mirrors the g_fx_* pattern: detached fetch worker writes g_emails under
// g_email_mutex and flips g_email_updated; the UI loop detects the flag in
// onRunning() and rebuilds the lv_list via _emailRefresh().
}  // namespace

// Static-member definitions for the shared email cache (declared in
// app_settings.h). Mirrors the g_fx_* pattern above: detached fetch worker
// writes email_list under email_list_mutex and flips email_updated; the UI
// loop detects the flag in onRunning() and rebuilds the lv_list. AppHome's
// status-bar icon polls fetchEmail() every 60s and reads email_unread_total.
std::atomic<int>        AppSettings::email_unread_total{0};  // total, from j["count"]
std::atomic<bool>       AppSettings::email_updated{false};
std::atomic<bool>       AppSettings::email_fetching{false};
std::atomic<bool>       AppSettings::email_error{false};
std::string             AppSettings::email_error_msg;
std::mutex              AppSettings::email_list_mutex;
std::vector<EmailItem>  AppSettings::email_list;

AppSettings::AppSettings()
{
    setAppInfo().name = "Tools";
    setAppInfo().icon = (void *)LV_SYMBOL_SETTINGS;
}

void AppSettings::onCreate() {

}

void AppSettings::onOpen()
{
    mclog::tagInfo(_tag, "open");
    
    _base_container = std::make_unique<ui::PageBaseContainer>();
    _base = _base_container->get();
    lv_obj_set_style_bg_color(_base, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(_base, LV_OPA_COVER, 0);

    _buildToolsPage();
    _base_container->installSwipeGesture([this]() {
        _handlePageBack();
    });
}

void AppSettings::onRunning()
{
    // Apply freshly fetched live rates (worker thread only flips the flag; the
    // label/recompute must happen on the UI loop).
    if (g_fx_updated.exchange(false) && _fx_page) {
        _fxRecompute();
    }
    // Same pattern for the email cache: worker fills g_emails + flips
    // g_email_updated; we rebuild the lv_list on the UI loop.
    if (email_updated.exchange(false) && _email_page) {
        _emailRefresh();
    }
    if (email_error.exchange(false) && _email_page) {
        _emailShowError();
    }
    // The 500 ms countdown was removed: lv_label_set_text repeatedly reallocates
    // a 1200x16 strip buffer (76.8 KB). On ESP32-P4 these allocations failed,
    // blocking the render pipeline and preventing onRunning from acquiring the
    // LVGL lock. Updating the status once when the worker finishes is sufficient.
}

void AppSettings::onClose()
{
    mclog::tagInfo(_tag, "on close");

    _base_container.reset();
    _base = nullptr;
    _tools_page = _calc_page = _fx_page = _unit_page = nullptr;
    _calc_expr_lbl = _calc_res_lbl = nullptr;
    _fx_from_dd = _fx_to_dd = _fx_amt_lbl = _fx_res_lbl = _fx_rate_lbl = nullptr;
    _unit_cat_dd = _unit_from_dd = _unit_to_dd = _unit_amt_lbl = _unit_res_lbl = _unit_eq_lbl = nullptr;
    _email_page = _email_title = _email_status = _email_list = nullptr;
    // Keep email_unread_total: this app only displays the email list and has no
    // "mark as read" action. Closing the app does not mark mail as read. The
    // unread count reflects server state and is refreshed by polling, so the
    // status-bar mail icon and LED matrix NEW EMAIL indicator should stay accurate.
    email_updated.store(false);
    email_error.store(false);
    email_error_msg.clear();
    email_fetching.store(false);  // Prevent the re-entry guard from staying stuck.
    if (_close_cb) _close_cb();
}

void AppSettings::_handlePageBack()
{
    if (!_base) return;

    if (_email_page) _closeEmail();
    else if (_unit_page) _closeUnit();
    else if (_fx_page) _closeFx();
    else if (_calc_page) _closeCalculator();
    else close();
}

// ════════════════════════════════════════════════════════════════════════════
//  Tools launcher page
// ════════════════════════════════════════════════════════════════════════════
void AppSettings::_buildToolsPage()
{
    _tools_page = lv_obj_create(_base);
    lv_obj_set_size(_tools_page, 1280, 720);
    lv_obj_set_pos(_tools_page, 0, 0);
    lv_obj_set_style_bg_opa(_tools_page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_tools_page, 0, 0);
    lv_obj_set_style_pad_all(_tools_page, 0, 0);
    lv_obj_remove_flag(_tools_page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(_tools_page);
    lv_label_set_text(title, "Tools");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_text_color(title, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_text_font(title, zh_font_30(), 0);

    // Tool card — same style as the HA "Lights" device cards: dark-blue rounded
    // card with a soft shadow, icon on the left, name on the right.
    // If img is nullptr, draw a 6x6 LED-matrix thumbnail for the 8x8 puzzle.
    // y_pos defaults to 140 for row 1; row 2 tiles pass 310.
    // kind 0 draws the LED matrix, kind 1 an envelope for email, and kind 2
    // concentric NFC rings for RFID.
    auto make_tile = [&](int x, const lv_image_dsc_t* img, int img_w,
                         const char* label, lv_event_cb_t cb,
                         int img_scale = 256, int icon_x = 12,
                         int img_scale_y = -1, int y_pos = 140,
                         int kind = 0) {
        lv_obj_t* tile = lv_obj_create(_tools_page);
        lv_obj_set_size(tile, 380, 140);
        lv_obj_align(tile, LV_ALIGN_TOP_LEFT, x, y_pos);
        lv_obj_set_style_bg_color(tile, lv_color_hex(C_CARD), 0);
        lv_obj_set_style_radius(tile, 20, 0);
        lv_obj_set_style_border_width(tile, 0, 0);
        lv_obj_set_style_pad_all(tile, 0, 0);
        lv_obj_set_style_shadow_width(tile, 18, 0);
        lv_obj_set_style_shadow_color(tile, lv_color_hex(0x5A7A9C), 0);
        lv_obj_set_style_shadow_opa(tile, LV_OPA_30, 0);
        lv_obj_set_style_shadow_ofs_y(tile, 4, 0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(C_CARD_PR), LV_STATE_PRESSED);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(tile, cb, LV_EVENT_CLICKED, this);

        lv_obj_t* name = lv_label_create(tile);
        lv_label_set_text(name, label);
        lv_obj_set_style_text_font(name, zh_font_30(), 0);
        lv_obj_set_style_text_color(name, lv_color_hex(C_TEXT), 0);

        if (img) {
            lv_obj_t* icon = lv_image_create(tile);
            lv_image_set_src(icon, img);
            if (img_scale_y > 0) {
                // Non-uniform scale (e.g. stretch a portrait icon to a square box).
                lv_image_set_scale_x(icon, img_scale);
                lv_image_set_scale_y(icon, img_scale_y);
            } else if (img_scale != 256) {
                lv_image_set_scale(icon, img_scale);
            }
            lv_obj_align(icon, LV_ALIGN_LEFT_MID, icon_x, 0);
            lv_obj_align(name, LV_ALIGN_LEFT_MID, icon_x + img_w + 16, 0);
        } else {
            // kind: 0 = LED matrix, 1 = email envelope.
            if (kind == 1) {
                // Email envelope: a white rectangle with a horizontal flap line.
                // Like the LED-matrix icon, it uses plain lv_obj shapes, not a font.
                const int EW = 84, EH = 56;
                const int ex = icon_x + 12;
                const int ey = (140 - EH) / 2;
                lv_obj_t* env_body = lv_obj_create(tile);
                lv_obj_set_size(env_body, EW, EH);
                lv_obj_set_pos(env_body, ex, ey);
                lv_obj_set_style_bg_color(env_body, lv_color_hex(C_TEXT), 0);
                lv_obj_set_style_bg_opa(env_body, LV_OPA_COVER, 0);
                lv_obj_set_style_radius(env_body, 6, 0);
                lv_obj_set_style_border_width(env_body, 0, 0);
                lv_obj_set_style_shadow_width(env_body, 0, 0);
                lv_obj_set_style_pad_all(env_body, 0, 0);
                lv_obj_remove_flag(env_body, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_remove_flag(env_body, LV_OBJ_FLAG_CLICKABLE);
                // Let clicks bubble from the envelope body to the parent tile.
                lv_obj_add_flag(env_body, LV_OBJ_FLAG_EVENT_BUBBLE);
                // Envelope flap line.
                lv_obj_t* flap = lv_obj_create(tile);
                lv_obj_set_size(flap, EW - 16, 2);
                lv_obj_set_pos(flap, ex + 8, ey + 18);
                lv_obj_set_style_bg_color(flap, lv_color_hex(0x6A8AAA), 0);
                lv_obj_set_style_bg_opa(flap, LV_OPA_COVER, 0);
                lv_obj_set_style_border_width(flap, 0, 0);
                lv_obj_set_style_radius(flap, 0, 0);
                lv_obj_set_style_shadow_width(flap, 0, 0);
                lv_obj_set_style_pad_all(flap, 0, 0);
                lv_obj_remove_flag(flap, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_remove_flag(flap, LV_OBJ_FLAG_CLICKABLE);
                // Let clicks bubble from the flap to the parent tile.
                lv_obj_add_flag(flap, LV_OBJ_FLAG_EVENT_BUBBLE);
                // Align the label at 160 px, matching image tiles.
                lv_obj_align(name, LV_ALIGN_LEFT_MID, ex + EW + 52, 0);
            } else if (kind == 2) {
                // NFC icon for the RFID tile: three concentric rings and a chip.
                // Like the envelope and LED-matrix icons, it uses plain lv_obj shapes.
                const int ccx = icon_x + 62;   // Ring center x.
                const int ccy = 70;            // Ring center y (centered in 140 px).
                const int sizes[3] = {96, 68, 40};
                for (int i = 0; i < 3; i++) {
                    lv_obj_t* ring = lv_obj_create(tile);
                    lv_obj_set_size(ring, sizes[i], sizes[i]);
                    lv_obj_set_pos(ring, ccx - sizes[i] / 2, ccy - sizes[i] / 2);
                    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
                    lv_obj_set_style_radius(ring, sizes[i] / 2, 0);   // Circular.
                    lv_obj_set_style_border_width(ring, 4, 0);
                    lv_obj_set_style_border_color(ring, lv_color_hex(C_ACCENT), 0);
                    lv_obj_set_style_shadow_width(ring, 0, 0);
                    lv_obj_set_style_pad_all(ring, 0, 0);
                    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_remove_flag(ring, LV_OBJ_FLAG_CLICKABLE);
                    // Let clicks bubble from the ring to the parent tile.
                    lv_obj_add_flag(ring, LV_OBJ_FLAG_EVENT_BUBBLE);
                }
                // Center chip.
                lv_obj_t* chip = lv_obj_create(tile);
                lv_obj_set_size(chip, 16, 16);
                lv_obj_set_pos(chip, ccx - 8, ccy - 8);
                lv_obj_set_style_bg_color(chip, lv_color_hex(C_ACCENT), 0);
                lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
                lv_obj_set_style_radius(chip, 3, 0);
                lv_obj_set_style_border_width(chip, 0, 0);
                lv_obj_set_style_shadow_width(chip, 0, 0);
                lv_obj_set_style_pad_all(chip, 0, 0);
                lv_obj_remove_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_remove_flag(chip, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_flag(chip, LV_OBJ_FLAG_EVENT_BUBBLE);
                // Align the label at 160 px, matching image tiles (icon_x+130+16=158).
                lv_obj_align(name, LV_ALIGN_LEFT_MID, icon_x + 130 + 18, 0);
            } else {
                // Draw a 3x3 grid of solid dots as an 8x8 LED-matrix thumbnail.
                // Avoid LV_SYMBOL_BULLET, which may render as a box if the font
                // lacks the glyph. Drawing nine rounded rectangles avoids fonts
                // and keeps the icon clear with minimal overhead.
                const int CELL = 24, GAP = 6, COLS = 3, ROWS = 3;
                const int GRID_W = COLS * CELL + (COLS - 1) * GAP;
                const int start_x = icon_x + 8;
                const int start_y = (140 - ROWS * CELL - (ROWS - 1) * GAP) / 2;
                for (int r = 0; r < ROWS; r++) {
                    for (int c = 0; c < COLS; c++) {
                        lv_obj_t* dot = lv_obj_create(tile);
                        lv_obj_set_size(dot, CELL, CELL);
                        lv_obj_set_pos(dot,
                            start_x + c * (CELL + GAP),
                            start_y + r * (CELL + GAP));
                        lv_obj_set_style_bg_color(dot, lv_color_hex(C_ACCENT), 0);
                        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
                        lv_obj_set_style_radius(dot, CELL / 2, 0);  // Circular.
                        lv_obj_set_style_border_width(dot, 0, 0);
                        lv_obj_set_style_shadow_width(dot, 0, 0);
                        lv_obj_set_style_pad_all(dot, 0, 0);
                        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
                        lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
                        // Let clicks bubble from the dots to the parent tile.
                        // The LED-matrix tile uses this same make_tile helper.
                        lv_obj_add_flag(dot, LV_OBJ_FLAG_EVENT_BUBBLE);
                    }
                }
                // Align the label at 160 px, matching image tiles (icon_x+130+16=158).
                lv_obj_align(name, LV_ALIGN_LEFT_MID, start_x + GRID_W + 56, 0);
            }
        }
    };

    // All three tool icons render at 130×130 to match the currency icon.
    //   fx_icon  : 100×100 native × 1.30 (scale 333) → 130×130
    //   unit_icon: 130×130 native × 1.00             → 130×130
    //   calc_icon: 130×130 native × 1.00             → 130×130
    make_tile(35,  &calc_icon, 130, "Calculator",      _toolBtn_cb);
    make_tile(450, &fx_icon,   150, "Currency",        _toolFx_cb,   384);  // 384 = 256 × 1.5x → 150×150
    make_tile(865, &unit_icon, 130, "Unit Converter", _toolUnit_cb);

    // Row 2: LED matrix (Unit-Puzzle 8x8 WS2812 matrix test app).
    //   y = 140 (row 1) + 140 (tile height) + 30 (row gap) = 310.
    //   The nullptr icon uses make_tile's LED-matrix thumbnail branch.
    make_tile(35, nullptr, 130, "LED Matrix", _toolPuzzle_cb, 256, 12, -1, 310);

    // Row 2, column 2: LoRa chat (Unit C6L SX1262, PORT.A UART).
    //   Icon: four signal bars of increasing height.
    {
        const int x = 450, y_pos = 310;
        lv_obj_t* tile = lv_obj_create(_tools_page);
        lv_obj_set_size(tile, 380, 140);
        lv_obj_align(tile, LV_ALIGN_TOP_LEFT, x, y_pos);
        lv_obj_set_style_bg_color(tile, lv_color_hex(C_CARD), 0);
        lv_obj_set_style_radius(tile, 20, 0);
        lv_obj_set_style_border_width(tile, 0, 0);
        lv_obj_set_style_pad_all(tile, 0, 0);
        lv_obj_set_style_shadow_width(tile, 18, 0);
        lv_obj_set_style_shadow_color(tile, lv_color_hex(0x5A7A9C), 0);
        lv_obj_set_style_shadow_opa(tile, LV_OPA_30, 0);
        lv_obj_set_style_shadow_ofs_y(tile, 4, 0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(C_CARD_PR), LV_STATE_PRESSED);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(tile, _toolLoraChat_cb, LV_EVENT_CLICKED, this);

        // Official LoRa logo.
        lv_obj_t* icon = lv_image_create(tile);
        lv_image_set_src(icon, &lora_logo);
        lv_image_set_scale(icon, 256);  // 130×130 native, 1:1
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 12, 0);
        lv_obj_add_flag(icon, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_remove_flag(icon, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t* name = lv_label_create(tile);
        lv_label_set_text(name, "LoRa Chat");
        lv_obj_set_style_text_font(name, zh_font_30(), 0);
        lv_obj_set_style_text_color(name, lv_color_hex(C_TEXT), 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 12 + 130 + 16, 0);
    }

    // Row 3: email (fetch unread messages from hermes :8766/unread_emails).
    //   y = 310 + 140 + 30 = 480; use the kind 1 envelope branch.
    make_tile(35, nullptr, 130, "Email", _toolMail_cb, 256, 12, -1, 480, 1);

    // Row 3, column 2: stock watchlist (hermes :8766/api/stocks/portfolio,
    // backed by mx-selfselect). stocks_icon is native 130×130, matching LoRa
    // and currency image tiles.
    make_tile(450, &stocks_icon, 130, "Stocks", _toolStocks_cb, 256, 12, -1, 480);
#if 0
    // Row 3, column 3: Reachy dashboard (read-only; uses Reachy :8042/api/*).
    //   reachy_icon is 130×130, matching stocks_icon.
    make_tile(865, &reachy_icon, 130, "YRobot Dashboard", _toolReachy_cb, 256, 12, -1, 480);
#endif
    // Row 2, column 3: RFID reader (Unit NFC / MFRC522, PORT A I2C 0x28).
    //   The kind 2 icon uses three concentric rings around a chip to represent NFC.
    make_tile(865, nullptr, 130, "RFID", _toolRfid_cb, 256, 12, -1, 310, 2);
}

void AppSettings::_toolBtn_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_openCalculator();
}

void AppSettings::_toolPuzzle_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    if (self->_puzzle_id > 0) {
        mooncake::GetMooncake().openApp(self->_puzzle_id);
    }
}

void AppSettings::_toolLoraChat_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    if (self->_lora_chat_id > 0) {
        mooncake::GetMooncake().openApp(self->_lora_chat_id);
    }
}

void AppSettings::_toolFx_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_openFx();
}

void AppSettings::_toolUnit_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_openUnit();
}

void AppSettings::_toolMail_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_openEmail();
}

void AppSettings::_toolStocks_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->openStocks();
}

void AppSettings::openStocks()
{
    if (_stocks_id > 0) {
        mooncake::GetMooncake().openApp(_stocks_id);
    }
}

void AppSettings::_toolReachy_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->openReachy();
}

void AppSettings::openReachy()
{
    if (_reachy_id > 0) {
        mooncake::GetMooncake().openApp(_reachy_id);
    }
}

void AppSettings::_toolRfid_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    if (self->_rfid_id > 0) {
        mooncake::GetMooncake().openApp(self->_rfid_id);
    }
}

void AppSettings::_emailRefresh_cb(lv_event_t* e)
{
    // The email page's Refresh button fetches again; exchange(true) skips in-flight requests.
    auto* app = static_cast<AppSettings*>(lv_event_get_user_data(e));
    mclog::tagInfo(_tag, "email refresh button clicked");
    app->fetchEmail();  // (also static; the explicit instance call keeps the
                       //  refresh-button event handler readable)
}

// ════════════════════════════════════════════════════════════════════════════
//  Calculator
// ════════════════════════════════════════════════════════════════════════════
void AppSettings::_openCalculator()
{
    if (_tools_page) lv_obj_add_flag(_tools_page, LV_OBJ_FLAG_HIDDEN);

    _calc_page = lv_obj_create(_base);
    lv_obj_set_size(_calc_page, 1280, 720);
    lv_obj_set_pos(_calc_page, 0, 0);
    lv_obj_set_style_bg_color(_calc_page, lv_color_hex(CALC_BG), 0);
    lv_obj_set_style_border_width(_calc_page, 0, 0);
    lv_obj_set_style_radius(_calc_page, 0, 0);
    lv_obj_set_style_pad_all(_calc_page, 0, 0);
    lv_obj_remove_flag(_calc_page, LV_OBJ_FLAG_SCROLLABLE);

    // ── Display: expression (small, grey) over result (big, white), right-aligned.
    _calc_expr_lbl = lv_label_create(_calc_page);
    lv_label_set_text(_calc_expr_lbl, "");
    lv_obj_set_style_text_font(_calc_expr_lbl, &font_calc_expr, 0);
    lv_obj_set_style_text_color(_calc_expr_lbl, lv_color_hex(CALC_EXPR), 0);
    lv_obj_align(_calc_expr_lbl, LV_ALIGN_TOP_RIGHT, -36, 86);

    _calc_res_lbl = lv_label_create(_calc_page);
    lv_label_set_text(_calc_res_lbl, "0");
    lv_obj_set_style_text_font(_calc_res_lbl, &font_calc_big, 0);
    lv_obj_set_style_text_color(_calc_res_lbl, lv_color_hex(CALC_TXT), 0);
    lv_obj_align(_calc_res_lbl, LV_ALIGN_TOP_RIGHT, -36, 130);

    // ── Button grid: 5 columns × 4 rows ──────────────────────────────────────
    struct Key { const char* label; const char* code; uint32_t color; bool sym; };
    // code is what _calcInput receives. "DEL"/"AC"/"PCT"/"NEG" are special.
    static const Key KEYS[4][5] = {
        {{"7","7",CALC_NUM,false},{"8","8",CALC_NUM,false},{"9","9",CALC_NUM,false},
         {LV_SYMBOL_BACKSPACE,"DEL",CALC_FUNC,true},{"\xC3\xB7","/",CALC_OP,false}},
        {{"4","4",CALC_NUM,false},{"5","5",CALC_NUM,false},{"6","6",CALC_NUM,false},
         {"AC","AC",CALC_FUNC,false},{"\xC3\x97","*",CALC_OP,false}},
        {{"1","1",CALC_NUM,false},{"2","2",CALC_NUM,false},{"3","3",CALC_NUM,false},
         {"%","PCT",CALC_FUNC,false},{"-","-",CALC_OP,false}},
        {{"+/-","NEG",CALC_NUM,false},{"0","0",CALC_NUM,false},{".",".",CALC_NUM,false},
         {"=","=",CALC_OP,false},{"+","+",CALC_OP,false}},
    };

    const int SIDE = 24, GAP = 16, COLS = 5, ROWS = 4;
    const int grid_top = 270;
    const int bw = (1280 - 2 * SIDE - (COLS - 1) * GAP) / COLS;   // ~233
    const int bh = (720 - grid_top - SIDE - (ROWS - 1) * GAP) / ROWS;  // ~95

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            const Key& k = KEYS[r][c];
            lv_obj_t* b = lv_obj_create(_calc_page);
            lv_obj_set_size(b, bw, bh);
            lv_obj_set_pos(b, SIDE + c * (bw + GAP), grid_top + r * (bh + GAP));
            lv_obj_set_style_bg_color(b, lv_color_hex(k.color), 0);
            lv_obj_set_style_radius(b, bh / 2, 0);   // pill
            lv_obj_set_style_border_width(b, 0, 0);
            // Pressed: lighten.
            lv_obj_set_style_bg_color(b,
                lv_color_hex(k.color == CALC_OP ? 0xFFBE5C
                           : k.color == CALC_NUM ? 0x5A5A5C : 0x6E6E70),
                LV_STATE_PRESSED);
            lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_user_data(b, (void*)k.code);
            lv_obj_add_event_cb(b, _calcKey_cb, LV_EVENT_CLICKED, this);

            lv_obj_t* l = lv_label_create(b);
            lv_label_set_text(l, k.label);
            lv_obj_center(l);
            lv_obj_set_style_text_color(l, lv_color_hex(CALC_TXT), 0);
            lv_obj_set_style_text_font(l, k.sym ? &lv_font_montserrat_28 : &font_calc_op, 0);
        }
    }

    _calcReset();
}

void AppSettings::_back_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_closeCalculator();
}

void AppSettings::_closeCalculator()
{
    if (_calc_page) {
        lv_obj_delete(_calc_page);
        _calc_page = nullptr;
        _calc_expr_lbl = _calc_res_lbl = nullptr;
    }
    if (_tools_page) lv_obj_remove_flag(_tools_page, LV_OBJ_FLAG_HIDDEN);
}

// ── Calculator logic ─────────────────────────────────────────────────────────
namespace {
// Group the integer part of a numeric string with thousands separators,
// preserving sign and any fractional part / trailing dot as typed.
std::string group(const std::string& s)
{
    std::string sign, num = s;
    if (!num.empty() && (num[0] == '-' || num[0] == '+')) { sign = num.substr(0, 1); num = num.substr(1); }
    size_t dot = num.find('.');
    std::string ip = (dot == std::string::npos) ? num : num.substr(0, dot);
    std::string fp = (dot == std::string::npos) ? ""  : num.substr(dot);  // includes '.'
    std::string out;
    int cnt = 0;
    for (int i = (int)ip.size() - 1; i >= 0; i--) {
        out.insert(out.begin(), ip[i]);
        if (++cnt % 3 == 0 && i > 0) out.insert(out.begin(), ',');
    }
    if (ip.empty()) out = "0";
    return sign + out + fp;
}

// Format a double for display: up to 10 significant digits, trailing zeros
// trimmed, then thousands-grouped.
std::string fmtNum(double v)
{
    if (std::isnan(v) || std::isinf(v)) return "Error";
    char buf[40];
    snprintf(buf, sizeof(buf), "%.10g", v);
    return group(buf);
}

// Currency-converter formatter: at most 3 decimal places, trailing zeros
// trimmed, then thousands-grouped. Keeps the result line short (full-precision
// values like 0.1476688479 otherwise overflow the card).
std::string fxFmt(double v)
{
    if (std::isnan(v) || std::isinf(v)) return "Error";
    char buf[40];
    snprintf(buf, sizeof(buf), "%.3f", v);
    std::string s = buf;
    if (s.find('.') != std::string::npos) {
        size_t e = s.size();
        while (e > 0 && s[e - 1] == '0') e--;
        if (e > 0 && s[e - 1] == '.') e--;
        s = s.substr(0, e);
    }
    return group(s);
}

double applyOp(double a, double b, char op)
{
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b != 0.0 ? a / b : NAN;
    }
    return b;
}

const char* opSym(char op)
{
    switch (op) {
        case '+': return "+";
        case '-': return "-";
        case '*': return "\xC3\x97";  // ×
        case '/': return "\xC3\xB7";  // ÷
    }
    return "";
}
}  // namespace

void AppSettings::_calcReset()
{
    _entry = "0";
    _acc   = 0.0;
    _op    = 0;
    _fresh = true;
    _lhs_disp.clear();
    _calcRefresh();
}

void AppSettings::_calcRefresh()
{
    if (_calc_res_lbl)  lv_label_set_text(_calc_res_lbl, group(_entry).c_str());
    if (_calc_expr_lbl) {
        std::string ex;
        if (_op) ex = _lhs_disp + opSym(_op) + (_fresh ? "" : group(_entry));
        lv_label_set_text(_calc_expr_lbl, ex.c_str());
    }
}

void AppSettings::_calcInput(const char* key)
{
    std::string k = key;

    if (k == "AC") { _calcReset(); return; }

    if (k == "DEL") {
        if (!_fresh && _entry != "0") {
            _entry.pop_back();
            if (_entry.empty() || _entry == "-") _entry = "0";
        }
        _calcRefresh();
        return;
    }

    if (k == "NEG") {
        if (_entry != "0") {
            if (_entry[0] == '-') _entry = _entry.substr(1);
            else                  _entry = "-" + _entry;
        }
        _calcRefresh();
        return;
    }

    if (k == "PCT") {
        double v = atof(_entry.c_str()) / 100.0;
        _entry = fmtNum(v);
        // strip grouping commas back out so further edits stay numeric
        std::string t; for (char ch : _entry) if (ch != ',') t += ch;
        _entry = t;
        _fresh = true;
        _calcRefresh();
        return;
    }

    if (k == ".") {
        if (_fresh) { _entry = "0."; _fresh = false; }
        else if (_entry.find('.') == std::string::npos) _entry += ".";
        _calcRefresh();
        return;
    }

    // Digit 0-9
    if (k.size() == 1 && k[0] >= '0' && k[0] <= '9') {
        if (_fresh) { _entry = k; _fresh = false; }
        else if (_entry == "0") _entry = k;
        else if (_entry == "-0") _entry = "-" + k;
        else _entry += k;
        _calcRefresh();
        return;
    }

    // Operators + - * /
    if (k == "+" || k == "-" || k == "*" || k == "/") {
        if (_op && !_fresh) {
            _acc = applyOp(_acc, atof(_entry.c_str()), _op);
        } else if (!_op) {
            _acc = atof(_entry.c_str());
        }
        _op = k[0];
        _lhs_disp = fmtNum(_acc);
        _entry = fmtNum(_acc);
        { std::string t; for (char ch : _entry) if (ch != ',') t += ch; _entry = t; }
        _fresh = true;
        _calcRefresh();
        return;
    }

    // Equals
    if (k == "=") {
        if (_op) {
            double rhs = atof(_entry.c_str());
            std::string rhs_disp = group(_entry);
            double res = applyOp(_acc, rhs, _op);
            // Show full "lhs op rhs" expression, result below.
            if (_calc_expr_lbl)
                lv_label_set_text(_calc_expr_lbl, (_lhs_disp + opSym(_op) + rhs_disp).c_str());
            _entry = fmtNum(res);
            { std::string t; for (char ch : _entry) if (ch != ',') t += ch; _entry = t; }
            if (_calc_res_lbl) lv_label_set_text(_calc_res_lbl, group(_entry).c_str());
            _acc = res;
            _op = 0;
            _fresh = true;
        }
        return;
    }
}

void AppSettings::_calcKey_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    lv_obj_t* b = (lv_obj_t*)lv_event_get_target(e);
    const char* code = (const char*)lv_obj_get_user_data(b);
    if (code) self->_calcInput(code);
}

// ════════════════════════════════════════════════════════════════════════════
//  Currency converter
// ════════════════════════════════════════════════════════════════════════════
void AppSettings::_openFx()
{
    if (_tools_page) lv_obj_add_flag(_tools_page, LV_OBJ_FLAG_HIDDEN);

    _fx_page = lv_obj_create(_base);
    lv_obj_set_size(_fx_page, 1280, 720);
    lv_obj_set_pos(_fx_page, 0, 0);
    lv_obj_set_style_bg_color(_fx_page, lv_color_hex(C_BG), 0);
    lv_obj_set_style_border_width(_fx_page, 0, 0);
    lv_obj_set_style_radius(_fx_page, 0, 0);
    lv_obj_set_style_pad_all(_fx_page, 0, 0);
    lv_obj_remove_flag(_fx_page, LV_OBJ_FLAG_SCROLLABLE);

    // Title.
    lv_obj_t* title = lv_label_create(_fx_page);
    lv_label_set_text(title, "Currency Converter");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);
    lv_obj_set_style_text_color(title, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_text_font(title, zh_font_30(), 0);

    // Currency code options for the dropdowns (ASCII → renders in montserrat).
    std::string opts;
    for (int i = 0; i < FX_N; i++) { if (i) opts += "\n"; opts += FX[i].code; }

    // ── Conversion panel (left) ──────────────────────────────────────────────
    const int CARD_X = 40, CARD_W = 600, CARD_H = 120;

    auto make_conv_card = [&](int y, bool is_from) -> lv_obj_t* {
        lv_obj_t* c = lv_obj_create(_fx_page);
        lv_obj_set_size(c, CARD_W, CARD_H);
        lv_obj_set_pos(c, CARD_X, y);
        lv_obj_set_style_bg_color(c, lv_color_hex(C_CARD), 0);
        lv_obj_set_style_radius(c, 20, 0);
        lv_obj_set_style_border_width(c, 0, 0);
        lv_obj_set_style_pad_all(c, 0, 0);
        lv_obj_set_style_shadow_width(c, 18, 0);
        lv_obj_set_style_shadow_color(c, lv_color_hex(0x5A7A9C), 0);
        lv_obj_set_style_shadow_opa(c, LV_OPA_30, 0);
        lv_obj_set_style_shadow_ofs_y(c, 4, 0);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* dd = lv_dropdown_create(c);
        lv_obj_set_size(dd, 130, 60);
        lv_obj_align(dd, LV_ALIGN_LEFT_MID, 16, 0);
        lv_dropdown_set_options(dd, opts.c_str());
        lv_obj_set_style_text_font(dd, &lv_font_montserrat_30, 0);
        lv_obj_t* dd_list = lv_dropdown_get_list(dd);
        lv_obj_set_style_text_font(dd_list, &lv_font_montserrat_30, 0);
        lv_obj_add_event_cb(dd, _fxDd_cb, LV_EVENT_VALUE_CHANGED, this);

        lv_obj_t* val = lv_label_create(c);
        lv_label_set_text(val, "0");
        lv_obj_set_style_text_font(val, &font_calc_op, 0);
        lv_obj_set_style_text_color(val,
            lv_color_hex(is_from ? CALC_TXT : C_ACCENT), 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, -20, 0);

        if (is_from) { _fx_from_dd = dd; _fx_amt_lbl = val; }
        else         { _fx_to_dd   = dd; _fx_res_lbl = val; }
        return c;
    };

    make_conv_card(120, true);
    make_conv_card(340, false);

    // Swap button centered in the gap between the two cards.
    // FROM card: 120..(120+CARD_H); TO card starts at 340 → gap centre = 290.
    lv_obj_t* swap = lv_obj_create(_fx_page);
    lv_obj_set_size(swap, 64, 64);
    lv_obj_set_pos(swap, CARD_X + CARD_W / 2 - 32, 290 - 32);
    lv_obj_set_style_bg_color(swap, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_radius(swap, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(swap, 3, 0);
    lv_obj_set_style_border_color(swap, lv_color_hex(C_BG), 0);
    lv_obj_remove_flag(swap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(swap, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(swap, _fxSwap_cb, LV_EVENT_CLICKED, this);
    lv_obj_t* swap_lbl = lv_label_create(swap);
    lv_label_set_text(swap_lbl, LV_SYMBOL_LOOP);
    lv_obj_center(swap_lbl);
    lv_obj_set_style_text_color(swap_lbl, lv_color_hex(0xFFFFFF), 0);

    // Rate line.
    _fx_rate_lbl = lv_label_create(_fx_page);
    lv_label_set_text(_fx_rate_lbl, "");
    lv_obj_set_pos(_fx_rate_lbl, CARD_X + 6, 480);
    lv_obj_set_style_text_color(_fx_rate_lbl, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_text_font(_fx_rate_lbl, zh_font_30(), 0);

    // ── Keypad (right) ───────────────────────────────────────────────────────
    const int KX = 700, KW = 540, GAP = 16, COLS = 3, ROWS = 4;

    // AC bar above the keypad.
    lv_obj_t* ac = lv_obj_create(_fx_page);
    lv_obj_set_size(ac, KW, 60);
    lv_obj_set_pos(ac, KX, 70);
    lv_obj_set_style_bg_color(ac, lv_color_hex(CALC_FUNC), 0);
    lv_obj_set_style_radius(ac, 16, 0);
    lv_obj_set_style_border_width(ac, 0, 0);
    lv_obj_set_style_bg_color(ac, lv_color_hex(0x6E6E70), LV_STATE_PRESSED);
    lv_obj_remove_flag(ac, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ac, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(ac, (void*)"AC");
    lv_obj_add_event_cb(ac, _fxKey_cb, LV_EVENT_CLICKED, this);
    lv_obj_t* ac_lbl = lv_label_create(ac);
    lv_label_set_text(ac_lbl, "AC");
    lv_obj_center(ac_lbl);
    lv_obj_set_style_text_color(ac_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ac_lbl, &font_calc_op, 0);

    struct FKey { const char* label; const char* code; bool sym; };
    static const FKey FKEYS[4][3] = {
        {{"7","7",false},{"8","8",false},{"9","9",false}},
        {{"4","4",false},{"5","5",false},{"6","6",false}},
        {{"1","1",false},{"2","2",false},{"3","3",false}},
        {{".",".",false},{"0","0",false},{LV_SYMBOL_BACKSPACE,"DEL",true}},
    };
    const int ktop = 150;
    const int bw = (KW - (COLS - 1) * GAP) / COLS;
    const int bh = (720 - ktop - 30 - (ROWS - 1) * GAP) / ROWS;
    for (int r = 0; r < ROWS; r++) {
        for (int col = 0; col < COLS; col++) {
            const FKey& k = FKEYS[r][col];
            lv_obj_t* b = lv_obj_create(_fx_page);
            lv_obj_set_size(b, bw, bh);
            lv_obj_set_pos(b, KX + col * (bw + GAP), ktop + r * (bh + GAP));
            lv_obj_set_style_bg_color(b, lv_color_hex(CALC_NUM), 0);
            lv_obj_set_style_radius(b, bh / 2, 0);
            lv_obj_set_style_border_width(b, 0, 0);
            lv_obj_set_style_bg_color(b, lv_color_hex(0x5A5A5C), LV_STATE_PRESSED);
            lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_user_data(b, (void*)k.code);
            lv_obj_add_event_cb(b, _fxKey_cb, LV_EVENT_CLICKED, this);
            lv_obj_t* l = lv_label_create(b);
            lv_label_set_text(l, k.label);
            lv_obj_center(l);
            lv_obj_set_style_text_color(l, lv_color_hex(CALC_TXT), 0);
            lv_obj_set_style_text_font(l, k.sym ? &lv_font_montserrat_28 : &font_calc_op, 0);
        }
    }

    // Defaults: 1 USD → CNY.
    _fx_entry = "1";
    lv_dropdown_set_selected(_fx_from_dd, 0);
    lv_dropdown_set_selected(_fx_to_dd, 1);
    _fxRecompute();

    // Pull live rates from the HA server in the background (falls back to the
    // static table if the server is unreachable).
    _fxFetch();
}

void AppSettings::_fxFetch()
{
    if (g_fx_fetching.exchange(true)) return;  // already in flight

    std::string host = GetHAL()->getConfig("svc_host", "");
    std::string url  = "http://" + host + ":8766/rate";

    bool spawned = GetHAL()->tryRunDetached([url]() {
        auto resp = GetHAL()->httpGet(url);
        if (resp.ok) {
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("ok", false) && j.contains("rates")) {
                    const auto& rates = j["rates"];
                    std::lock_guard<std::mutex> lk(g_fx_mutex);
                    for (int i = 0; i < FX_N; i++) {
                        auto it = rates.find(FX[i].code);
                        if (it != rates.end() && it->is_number()) {
                            double v = it->get<double>();
                            if (v > 0) g_fx_rate[i] = v;
                        }
                    }
                    g_fx_source = j.value("source", std::string("?"));
                    g_fx_updated.store(true);  // UI loop will recompute
                    mclog::tagInfo(_tag, "fx rates updated ({})", g_fx_source.c_str());
                }
            } catch (...) {
                mclog::tagWarn(_tag, "fx parse failed");
            }
        } else {
            mclog::tagWarn(_tag, "fx fetch failed (keeping static rates)");
        }
        g_fx_fetching.store(false);
    });
    if (!spawned) g_fx_fetching.store(false);
}

void AppSettings::_fxBack_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_closeFx();
}

void AppSettings::_closeFx()
{
    if (_fx_page) {
        lv_obj_delete(_fx_page);
        _fx_page = nullptr;
        _fx_from_dd = _fx_to_dd = _fx_amt_lbl = _fx_res_lbl = _fx_rate_lbl = nullptr;
    }
    if (_tools_page) lv_obj_remove_flag(_tools_page, LV_OBJ_FLAG_HIDDEN);
}

void AppSettings::_fxRecompute()
{
    if (!_fx_from_dd || !_fx_to_dd) return;
    int fi = lv_dropdown_get_selected(_fx_from_dd);
    int ti = lv_dropdown_get_selected(_fx_to_dd);
    if (fi < 0 || fi >= FX_N) fi = 0;
    if (ti < 0 || ti >= FX_N) ti = 0;

    double rf, rt;
    {
        std::lock_guard<std::mutex> lk(g_fx_mutex);
        rf = g_fx_rate[fi];
        rt = g_fx_rate[ti];
    }
    if (rf <= 0) rf = 1;
    double amt = atof(_fx_entry.c_str());
    double res = amt / rf * rt;

    if (_fx_amt_lbl) lv_label_set_text(_fx_amt_lbl, group(_fx_entry).c_str());
    if (_fx_res_lbl) lv_label_set_text(_fx_res_lbl, fxFmt(res).c_str());
    if (_fx_rate_lbl) {
        double one = rt / rf;  // 1 FROM = one TO
        char buf[96];
        snprintf(buf, sizeof(buf), "1 %s = %s %s", FX[fi].name, fxFmt(one).c_str(), FX[ti].name);
        lv_label_set_text(_fx_rate_lbl, buf);
    }
}

void AppSettings::_fxInput(const char* key)
{
    std::string k = key;
    if (k == "AC") {
        _fx_entry = "0";
    } else if (k == "DEL") {
        if (!_fx_entry.empty()) _fx_entry.pop_back();
        if (_fx_entry.empty()) _fx_entry = "0";
    } else if (k == ".") {
        if (_fx_entry.find('.') == std::string::npos) _fx_entry += ".";
    } else {  // digit
        if (_fx_entry == "0") _fx_entry = k;
        else if (_fx_entry.size() < 12) _fx_entry += k;
    }
    _fxRecompute();
}

void AppSettings::_fxKey_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    lv_obj_t* b = (lv_obj_t*)lv_event_get_target(e);
    const char* code = (const char*)lv_obj_get_user_data(b);
    if (code) self->_fxInput(code);
}

void AppSettings::_fxDd_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_fxRecompute();
}

void AppSettings::_fxSwap_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    if (!self->_fx_from_dd || !self->_fx_to_dd) return;
    int fi = lv_dropdown_get_selected(self->_fx_from_dd);
    int ti = lv_dropdown_get_selected(self->_fx_to_dd);
    lv_dropdown_set_selected(self->_fx_from_dd, ti);
    lv_dropdown_set_selected(self->_fx_to_dd, fi);
    self->_fxRecompute();
}

// ════════════════════════════════════════════════════════════════════════════
//  Unit converter
// ════════════════════════════════════════════════════════════════════════════

struct UnitCat {
    const char* name;
    struct Unit {
        const char* code;       // abbreviation shown in dropdown
        const char* label;      // full name for the equation line
        double to_base;         // multiply to get base-unit value
    };
    const Unit* units;
    int n_units;
};

// Conversion factors relative to the base unit of each category.
// Temperature is handled separately (not a linear factor).

static const UnitCat::Unit UNITS_LENGTH[] = {
    {"mm",  "millimeter",  0.001},
    {"cm",  "centimeter",  0.01},
    {"m",   "meter",       1.0},
    {"km",  "kilometer",   1000.0},
    {"in",  "inch",        0.0254},
    {"ft",  "foot",        0.3048},
    {"yd",  "yard",        0.9144},
    {"mi",  "mile",        1609.344},
};
static const UnitCat::Unit UNITS_WEIGHT[] = {
    {"mg",  "milligram", 0.001},
    {"g",   "gram",      1.0},
    {"kg",  "kilogram",  1000.0},
    {"t",   "metric ton",1e6},
    {"oz",  "ounce",     28.3495},
    {"lb",  "pound",     453.592},
};
static const UnitCat::Unit UNITS_TEMP[] = {
    {"°C",  "Celsius",    0},
    {"°F",  "Fahrenheit", 0},
    {"K",   "Kelvin",     0},
};
static const UnitCat::Unit UNITS_AREA[] = {
    {"mm²", "square millimeter", 1e-6},
    {"cm²", "square centimeter", 0.0001},
    {"m²",  "square meter",      1.0},
    {"km²", "square kilometer",  1e6},
    {"ha",  "hectare",            10000.0},
    {"mu",  "mu",                 666.6667},
    {"acre","acre",               4046.86},
    {"ft²", "square foot",        0.092903},
};
static const UnitCat::Unit UNITS_VOLUME[] = {
    {"mL",  "milliliter",    0.001},
    {"L",   "liter",         1.0},
    {"m³",  "cubic meter",   1000.0},
    {"gal", "gallon",        3.78541},
    {"cup", "cup",           0.236588},
};
static const UnitCat::Unit UNITS_SPEED[] = {
    {"m/s",  "meter/second",      1.0},
    {"km/h", "kilometer/hour",    0.277778},
    {"mph",  "mile/hour",         0.44704},
    {"kn",   "knot",              0.514444},
};
static const UnitCat::Unit UNITS_DATA[] = {
    {"B",   "byte",     1.0},
    {"KB",  "kilobyte", 1024.0},
    {"MB",  "megabyte", 1048576.0},
    {"GB",  "gigabyte", 1073741824.0},
    {"TB",  "terabyte", 1099511627776.0},
};

static const UnitCat UNIT_CATS[] = {
    {"Length",      UNITS_LENGTH, 8},
    {"Weight",      UNITS_WEIGHT, 6},
    {"Temperature", UNITS_TEMP,   3},
    {"Area",        UNITS_AREA,   8},
    {"Volume",      UNITS_VOLUME, 5},
    {"Speed",       UNITS_SPEED,  4},
    {"Data",        UNITS_DATA,   5},
};
static constexpr int UNIT_CAT_N = sizeof(UNIT_CATS) / sizeof(UNIT_CATS[0]);

static std::string unitFmt(double v)
{
    if (fabs(v) < 1e-12) return "0";
    char buf[32];
    if (fabs(v) >= 1e9)
        snprintf(buf, sizeof(buf), "%.8g", v);
    else if (fabs(v) < 0.001 && v != 0)
        snprintf(buf, sizeof(buf), "%.8g", v);
    else
        snprintf(buf, sizeof(buf), "%.3f", v);
    // Trim trailing zeros after decimal (but keep at least one decimal)
    std::string s = buf;
    if (s.find('.') != std::string::npos) {
        while (s.size() > 2 && s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

void AppSettings::_openUnit()
{
    if (_tools_page) lv_obj_add_flag(_tools_page, LV_OBJ_FLAG_HIDDEN);

    _unit_page = lv_obj_create(_base);
    lv_obj_set_size(_unit_page, 1280, 720);
    lv_obj_set_pos(_unit_page, 0, 0);
    lv_obj_set_style_bg_color(_unit_page, lv_color_hex(C_BG), 0);
    lv_obj_set_style_border_width(_unit_page, 0, 0);
    lv_obj_set_style_radius(_unit_page, 0, 0);
    lv_obj_set_style_pad_all(_unit_page, 0, 0);
    lv_obj_remove_flag(_unit_page, LV_OBJ_FLAG_SCROLLABLE);

    // Title.
    lv_obj_t* title = lv_label_create(_unit_page);
    lv_label_set_text(title, "Unit Converter");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);
    lv_obj_set_style_text_color(title, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_text_font(title, zh_font_30(), 0);

    // Category dropdown — left-aligned so it doesn't overlap the AC bar.
    std::string cat_opts;
    for (int i = 0; i < UNIT_CAT_N; i++) {
        if (i) cat_opts += "\n";
        cat_opts += UNIT_CATS[i].name;
    }
    _unit_cat_dd = lv_dropdown_create(_unit_page);
    lv_obj_set_size(_unit_cat_dd, 420, 60);
    lv_obj_set_pos(_unit_cat_dd, 40, 76);
    lv_dropdown_set_options(_unit_cat_dd, cat_opts.c_str());
    lv_obj_set_style_text_font(_unit_cat_dd, zh_font_30(), 0);
    lv_dropdown_set_selected(_unit_cat_dd, 0);
    _unit_cat_idx = 0;
    // The dropdown list is a separate object — must set its font too.
    lv_obj_t* cat_list = lv_dropdown_get_list(_unit_cat_dd);
    lv_obj_set_style_text_font(cat_list, zh_font_30(), 0);
    lv_obj_add_event_cb(_unit_cat_dd, _unitCatDd_cb, LV_EVENT_VALUE_CHANGED, this);

    // ── Conversion panel (left) ──────────────────────────────────────────────
    const int CARD_X = 40, CARD_W = 580, CARD_H = 120;
    const int KEY_X = 660, KEY_W = 580;

    _unit_from_dd = nullptr;
    _unit_to_dd   = nullptr;
    _unit_amt_lbl = nullptr;
    _unit_res_lbl = nullptr;

    auto make_card = [&](int y, bool is_from) -> lv_obj_t* {
        lv_obj_t* c = lv_obj_create(_unit_page);
        lv_obj_set_size(c, CARD_W, CARD_H);
        lv_obj_set_pos(c, CARD_X, y);
        lv_obj_set_style_bg_color(c, lv_color_hex(C_CARD), 0);
        lv_obj_set_style_radius(c, 20, 0);
        lv_obj_set_style_border_width(c, 0, 0);
        lv_obj_set_style_pad_all(c, 0, 0);
        lv_obj_set_style_shadow_width(c, 18, 0);
        lv_obj_set_style_shadow_color(c, lv_color_hex(0x5A7A9C), 0);
        lv_obj_set_style_shadow_opa(c, LV_OPA_30, 0);
        lv_obj_set_style_shadow_ofs_y(c, 4, 0);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* dd = lv_dropdown_create(c);
        lv_obj_set_size(dd, 140, 60);
        lv_obj_align(dd, LV_ALIGN_LEFT_MID, 16, 0);
        lv_obj_set_style_text_font(dd, &lv_font_montserrat_30, 0);
        // Dropdown list needs its own font (otherwise defaults to montserrat).
        lv_obj_t* dd_list = lv_dropdown_get_list(dd);
        lv_obj_set_style_text_font(dd_list, &lv_font_montserrat_30, 0);

        lv_obj_t* val = lv_label_create(c);
        lv_label_set_text(val, "0");
        lv_obj_set_style_text_font(val, &font_calc_op, 0);
        lv_obj_set_style_text_color(val,
            lv_color_hex(is_from ? CALC_TXT : C_ACCENT), 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, -20, 0);

        if (is_from) {
            _unit_from_dd = dd;
            _unit_amt_lbl = val;
            lv_obj_add_event_cb(dd, _unitFromDd_cb, LV_EVENT_VALUE_CHANGED, this);
        } else {
            _unit_to_dd = dd;
            _unit_res_lbl = val;
            lv_obj_add_event_cb(dd, _unitToDd_cb, LV_EVENT_VALUE_CHANGED, this);
        }
        return c;
    };

    make_card(180, true);
    make_card(400, false);

    // Swap button. FROM card bottom = 180+120=300, TO card top = 400.
    // Gap centre = (300+400)/2 = 350.  64 px button centred at y=318.
    lv_obj_t* swap = lv_obj_create(_unit_page);
    lv_obj_set_size(swap, 64, 64);
    lv_obj_set_pos(swap, CARD_X + CARD_W / 2 - 32, 318);
    lv_obj_set_style_bg_color(swap, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_radius(swap, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(swap, 3, 0);
    lv_obj_set_style_border_color(swap, lv_color_hex(C_BG), 0);
    lv_obj_remove_flag(swap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(swap, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(swap, _unitSwap_cb, LV_EVENT_CLICKED, this);
    lv_obj_t* swap_lbl = lv_label_create(swap);
    lv_label_set_text(swap_lbl, LV_SYMBOL_LOOP);
    lv_obj_center(swap_lbl);
    lv_obj_set_style_text_color(swap_lbl, lv_color_hex(0xFFFFFF), 0);

    // Equivalence line.
    _unit_eq_lbl = lv_label_create(_unit_page);
    lv_label_set_text(_unit_eq_lbl, "");
    lv_obj_set_pos(_unit_eq_lbl, CARD_X + 6, 555);
    lv_obj_set_style_text_color(_unit_eq_lbl, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_text_font(_unit_eq_lbl, zh_font_30(), 0);

    // ── Keypad (right) ───────────────────────────────────────────────────────
    const int GAP = 16, COLS = 3, ROWS = 4;

    lv_obj_t* ac = lv_obj_create(_unit_page);
    lv_obj_set_size(ac, KEY_W, 60);
    lv_obj_set_pos(ac, KEY_X, 76);
    lv_obj_set_style_bg_color(ac, lv_color_hex(CALC_FUNC), 0);
    lv_obj_set_style_radius(ac, 16, 0);
    lv_obj_set_style_border_width(ac, 0, 0);
    lv_obj_set_style_bg_color(ac, lv_color_hex(0x6E6E70), LV_STATE_PRESSED);
    lv_obj_remove_flag(ac, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ac, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(ac, (void*)"AC");
    lv_obj_add_event_cb(ac, _unitKey_cb, LV_EVENT_CLICKED, this);
    lv_obj_t* ac_lbl = lv_label_create(ac);
    lv_label_set_text(ac_lbl, "AC");
    lv_obj_center(ac_lbl);
    lv_obj_set_style_text_color(ac_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ac_lbl, &font_calc_op, 0);

    struct UKey { const char* label; const char* code; bool sym; };
    static const UKey UKEYS[4][3] = {
        {{"7","7",false},{"8","8",false},{"9","9",false}},
        {{"4","4",false},{"5","5",false},{"6","6",false}},
        {{"1","1",false},{"2","2",false},{"3","3",false}},
        {{".",".",false},{"0","0",false},{LV_SYMBOL_BACKSPACE,"DEL",true}},
    };
    const int ktop = 150;
    const int bw = (KEY_W - (COLS - 1) * GAP) / COLS;
    const int bh = (720 - ktop - 30 - (ROWS - 1) * GAP) / ROWS;
    for (int r = 0; r < ROWS; r++) {
        for (int col = 0; col < COLS; col++) {
            const UKey& k = UKEYS[r][col];
            lv_obj_t* b = lv_obj_create(_unit_page);
            lv_obj_set_size(b, bw, bh);
            lv_obj_set_pos(b, KEY_X + col * (bw + GAP), ktop + r * (bh + GAP));
            lv_obj_set_style_bg_color(b, lv_color_hex(CALC_NUM), 0);
            lv_obj_set_style_radius(b, bh / 2, 0);
            lv_obj_set_style_border_width(b, 0, 0);
            lv_obj_set_style_bg_color(b, lv_color_hex(0x5A5A5C), LV_STATE_PRESSED);
            lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_user_data(b, (void*)k.code);
            lv_obj_add_event_cb(b, _unitKey_cb, LV_EVENT_CLICKED, this);
            lv_obj_t* l = lv_label_create(b);
            lv_label_set_text(l, k.label);
            lv_obj_center(l);
            lv_obj_set_style_text_color(l, lv_color_hex(CALC_TXT), 0);
            lv_obj_set_style_text_font(l, k.sym ? &lv_font_montserrat_28 : &font_calc_op, 0);
        }
    }

    // Populate unit dropdowns for the initial category.
    _unitPopulateUnits();
    _unit_entry = "1";
    _unitRecompute();
}

void AppSettings::_unitPopulateUnits()
{
    int ci = _unit_cat_idx;
    if (ci < 0 || ci >= UNIT_CAT_N) ci = 0;
    const UnitCat& cat = UNIT_CATS[ci];

    std::string opts;
    for (int i = 0; i < cat.n_units; i++) {
        if (i) opts += "\n";
        opts += cat.units[i].code;
    }

    if (_unit_from_dd) {
        lv_dropdown_set_options(_unit_from_dd, opts.c_str());
        lv_dropdown_set_selected(_unit_from_dd, 0);
    }
    if (_unit_to_dd) {
        lv_dropdown_set_options(_unit_to_dd, opts.c_str());
        lv_dropdown_set_selected(_unit_to_dd, cat.n_units > 1 ? 1 : 0);
    }
}

void AppSettings::_unitRecompute()
{
    if (!_unit_from_dd || !_unit_to_dd) return;
    int ci = _unit_cat_idx;
    if (ci < 0 || ci >= UNIT_CAT_N) ci = 0;
    const UnitCat& cat = UNIT_CATS[ci];

    int fi = lv_dropdown_get_selected(_unit_from_dd);
    int ti = lv_dropdown_get_selected(_unit_to_dd);
    if (fi < 0 || fi >= cat.n_units) fi = 0;
    if (ti < 0 || ti >= cat.n_units) ti = 0;

    double amt = atof(_unit_entry.c_str());

    double res;
    if (ci == 2) {
        // Temperature — special handling
        // Convert from unit fi to °C, then to unit ti.
        double celsius;
        if (fi == 0)      celsius = amt;              // °C → °C
        else if (fi == 1) celsius = (amt - 32) / 1.8; // °F → °C
        else              celsius = amt - 273.15;       // K → °C

        if (ti == 0)      res = celsius;
        else if (ti == 1) res = celsius * 1.8 + 32;
        else              res = celsius + 273.15;
    } else {
        // Linear conversion: value × to_base / from_to_base
        double f_factor = cat.units[fi].to_base;
        double t_factor = cat.units[ti].to_base;
        if (f_factor <= 0) f_factor = 1;
        res = amt * f_factor / t_factor;
    }

    if (_unit_amt_lbl) lv_label_set_text(_unit_amt_lbl, group(_unit_entry).c_str());
    if (_unit_res_lbl) lv_label_set_text(_unit_res_lbl, unitFmt(res).c_str());
    if (_unit_eq_lbl) {
        if (ci == 2) {
            char buf[96];
            snprintf(buf, sizeof(buf), "%s %s = %s %s",
                group(_unit_entry).c_str(), cat.units[fi].label,
                unitFmt(res).c_str(), cat.units[ti].label);
            lv_label_set_text(_unit_eq_lbl, buf);
        } else {
            double f_factor = cat.units[fi].to_base;
            double t_factor = cat.units[ti].to_base;
            if (f_factor <= 0) f_factor = 1;
            double one = f_factor / t_factor;
            char buf[96];
            snprintf(buf, sizeof(buf), "1 %s = %s %s",
                cat.units[fi].label, unitFmt(one).c_str(), cat.units[ti].label);
            lv_label_set_text(_unit_eq_lbl, buf);
        }
    }
}

void AppSettings::_unitInput(const char* key)
{
    std::string k = key;
    if (k == "AC") {
        _unit_entry = "0";
    } else if (k == "DEL") {
        if (!_unit_entry.empty()) _unit_entry.pop_back();
        if (_unit_entry.empty()) _unit_entry = "0";
    } else if (k == ".") {
        if (_unit_entry.find('.') == std::string::npos) _unit_entry += ".";
    } else {  // digit
        if (_unit_entry == "0") _unit_entry = k;
        else if (_unit_entry.size() < 14) _unit_entry += k;
    }
    _unitRecompute();
}

void AppSettings::_closeUnit()
{
    if (_unit_page) {
        lv_obj_delete(_unit_page);
        _unit_page = nullptr;
        _unit_cat_dd = _unit_from_dd = _unit_to_dd = nullptr;
        _unit_amt_lbl = _unit_res_lbl = _unit_eq_lbl = nullptr;
    }
    if (_tools_page) lv_obj_remove_flag(_tools_page, LV_OBJ_FLAG_HIDDEN);
}

// ── Static callbacks ───────────────────────────────────────────────────────

void AppSettings::_unitKey_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    lv_obj_t* b = (lv_obj_t*)lv_event_get_target(e);
    const char* code = (const char*)lv_obj_get_user_data(b);
    if (code) self->_unitInput(code);
}

void AppSettings::_unitCatDd_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    self->_unit_cat_idx = lv_dropdown_get_selected(self->_unit_cat_dd);
    self->_unitPopulateUnits();
    self->_unit_entry = "1";
    self->_unitRecompute();
}

void AppSettings::_unitFromDd_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_unitRecompute();
}

void AppSettings::_unitToDd_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_unitRecompute();
}

void AppSettings::_unitSwap_cb(lv_event_t* e)
{
    auto* self = static_cast<AppSettings*>(lv_event_get_user_data(e));
    if (!self->_unit_from_dd || !self->_unit_to_dd) return;
    int fi = lv_dropdown_get_selected(self->_unit_from_dd);
    int ti = lv_dropdown_get_selected(self->_unit_to_dd);
    lv_dropdown_set_selected(self->_unit_from_dd, ti);
    lv_dropdown_set_selected(self->_unit_to_dd, fi);
    self->_unitRecompute();
}

void AppSettings::_unitBack_cb(lv_event_t* e)
{
    static_cast<AppSettings*>(lv_event_get_user_data(e))->_closeUnit();
}

// ════════════════════════════════════════════════════════════════════════════
//  Email — pulls unread emails from hermes :8766/unread_emails
// ════════════════════════════════════════════════════════════════════════════
void AppSettings::openEmailPage()
{
    // Public entry used by the home status-bar mail icon. Same as the user
    // tapping the Email tool tile — build the sub-page and start a fetch.
    // No-op if the app hasn't been opened yet (no _scr).
    if (!_base) return;
    _openEmail();
}

void AppSettings::_openEmail()
{
    mclog::tagInfo(_tag, "_openEmail: entered");
    if (_tools_page) lv_obj_add_flag(_tools_page, LV_OBJ_FLAG_HIDDEN);

    _email_page = lv_obj_create(_base);
    lv_obj_set_size(_email_page, 1280, 720);
    lv_obj_set_pos(_email_page, 0, 0);
    lv_obj_set_style_bg_color(_email_page, lv_color_hex(C_BG), 0);
    lv_obj_set_style_border_width(_email_page, 0, 0);
    lv_obj_set_style_radius(_email_page, 0, 0);
    lv_obj_set_style_pad_all(_email_page, 0, 0);
    lv_obj_remove_flag(_email_page, LV_OBJ_FLAG_SCROLLABLE);

    // Page title: "Email" or "Email (N unread)".
    _email_title = lv_label_create(_email_page);
    lv_label_set_text(_email_title, "Email");
    lv_obj_set_style_text_color(_email_title, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_text_font(_email_title, zh_font_30(), 0);
    lv_obj_align(_email_title, LV_ALIGN_TOP_MID, 0, 28);

    // Status line: loading, fetch failure, or empty-state message.
    _email_status = lv_label_create(_email_page);
    lv_label_set_text(_email_status, "Loading...");
    lv_obj_set_style_text_color(_email_status, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_text_font(_email_status, zh_font_30(), 0);
    lv_obj_align(_email_status, LV_ALIGN_TOP_MID, 0, 76);

    // Refresh button (top right); fetchEmail protects against concurrent requests.
    lv_obj_t* refresh = lv_btn_create(_email_page);
    lv_obj_set_size(refresh, 100, 50);
    lv_obj_align(refresh, LV_ALIGN_TOP_RIGHT, -30, 30);
    lv_obj_set_style_bg_color(refresh, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_radius(refresh, 12, 0);
    lv_obj_set_style_border_width(refresh, 0, 0);
    lv_obj_add_event_cb(refresh, _emailRefresh_cb, LV_EVENT_CLICKED, this);
    lv_obj_t* refresh_lbl = lv_label_create(refresh);
    lv_label_set_text(refresh_lbl, "Refresh");
    lv_obj_set_style_text_font(refresh_lbl, zh_font_30(), 0);
    lv_obj_set_style_text_color(refresh_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(refresh_lbl);

    // Use a custom lv_obj container instead of lv_list. lv_list scrolling/clipping
    // repeatedly allocates a 1200x16 RGBA8888 layer buffer (76.8 KB), which caused
    // an LVGL loop and blocked the LVGL lock on the target device.
    _email_list = lv_obj_create(_email_page);
    lv_obj_set_size(_email_list, 1200, 540);
    lv_obj_align(_email_list, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_style_bg_color(_email_list, lv_color_hex(C_CARD), 0);
    lv_obj_set_style_bg_opa(_email_list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_email_list, 0, 0);
    lv_obj_set_style_radius(_email_list, 16, 0);
    lv_obj_set_style_pad_all(_email_list, 12, 0);
    lv_obj_set_scroll_dir(_email_list, LV_DIR_VER);
    lv_obj_set_flex_flow(_email_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(_email_list, 8, 0);

    mclog::tagInfo(_tag, "_openEmail: dispatching fetchEmail");
    fetchEmail();
    mclog::tagInfo(_tag, "_openEmail: done");
}

void AppSettings::_closeEmail()
{
    if (_email_page) {
        lv_obj_delete(_email_page);
        _email_page = _email_title = _email_status = _email_list = nullptr;
    }
    if (_tools_page) lv_obj_remove_flag(_tools_page, LV_OBJ_FLAG_HIDDEN);
}

void AppSettings::fetchEmail()
{
    if (email_fetching.exchange(true)) {
        mclog::tagWarn(_tag, "fetchEmail: already in flight, skip");
        return;
    }
    mclog::tagInfo(_tag, "fetchEmail: entered");

    // email-status-server.py runs on port 8768 and reads the cached
    // /tmp/email-status.json file (periodically updated by the email-led.py cron
    // job). It responds in under 10 ms and does not sync over IMAP.
    // This is the same data source used by the desktop Übersicht unread-emails widget.
    // Use svc_host rather than ha_host: ha_host points to Home Assistant, while
    // svc_host points to the machine running hermes and email-status-server.
    std::string host = GetHAL()->getConfig("svc_host", "");
    std::string url  = "http://" + host + ":8768/api/email/status";
    mclog::tagInfo(_tag, "emailFetch: url={}", url);

    bool spawned = GetHAL()->tryRunDetached([url]() {
        mclog::tagInfo(_tag, "emailFetch: worker started");
        auto resp = GetHAL()->httpGet(url);
        mclog::tagInfo(_tag, "emailFetch: httpGet done, ok={}, status={}, body_len={}",
                       resp.ok, resp.status, resp.body.size());
        if (resp.ok) {
            try {
                auto j = nlohmann::json::parse(resp.body);
                std::lock_guard<std::mutex> lk(email_list_mutex);
                email_list.clear();
                // 8768 schema: {count, folders:[{name, unread, latest_subject,
                // latest_from, latest_preview}], error, account, time}
                if (j.contains("folders") && j["folders"].is_array()) {
                    // Drafts are unfinished messages, not unread email. Skip folders
                    // named "draft" or with the localized Chinese folder name, case-
                    // insensitively, and sum unread counts instead of using server count.
                    auto is_draft = [](std::string n) {
                        for (auto& c : n) c = (char)std::tolower((unsigned char)c);
                        return n.find("draft") != std::string::npos ||
                               n.find("\xE8\x8D\x89\xE7\xA8\xBF") != std::string::npos;
                    };
                    int total = 0;
                    for (const auto& f : j["folders"]) {
                        if (!f.is_object()) continue;
                        std::string name = f.value("name", std::string("?"));
                        if (is_draft(name)) continue;  // Skip the drafts folder.
                        total += f.value("unread", 0);
                        // The API returns one latest message per folder; use preview as the subject.
                        email_list.push_back(EmailItem{
                            f.value("latest_from", std::string("?")),
                            f.value("latest_subject", std::string("(No subject)")),
                            std::string(),  // The API does not expose a date.
                            name,
                        });
                    }
                    email_unread_total.store(total);
                    email_updated.store(true);
                    mclog::tagInfo(_tag, "email list updated ({} folders, {} total, drafts excluded)",
                                   email_list.size(), email_unread_total.load());
                } else {
                    email_error.store(true);
                    email_error_msg = j.value("error", std::string("Unknown error"));
                }
            } catch (const std::exception& ex) {
                mclog::tagWarn(_tag, "email parse failed: {}", ex.what());
                email_error.store(true);
                email_error_msg = std::string("JSON parsing failed: ") + ex.what();
            }
        } else {
            // A zero status usually indicates a timeout, connection, or DNS failure.
            mclog::tagWarn(_tag, "email fetch failed (status={}, body={})",
                           resp.status, resp.body);
            email_error.store(true);
            if (resp.status == 0) {
                email_error_msg = "Network/timeout (status=0) - check the port 8768 email-status-server";
            } else {
                email_error_msg = "HTTP " + std::to_string(resp.status);
            }
        }
        email_fetching.store(false);
    });
    if (!spawned) {
        mclog::tagWarn(_tag, "emailFetch: tryRunDetached failed (pthread_create?)");
        email_error.store(true);
        email_error_msg = "Failed to create a local thread (out of memory?)";
        // Do NOT reset email_fetching here — that would allow the next
        // onRunning frame (~16 ms later) to call fetchEmail() again, creating
        // a tight retry loop that burns CPU and prevents the idle task from
        // ever freeing memory. Keep the flag set; the worker (if ever spawned)
        // will clear it on completion, and the 60 s poll cycle will naturally
        // retry later.
    }
}

void AppSettings::_emailRefresh()
{
    if (!_email_list) return;
    // This LVGL version has no lv_list_clean; use lv_obj_clean to remove child objects.
    lv_obj_clean(_email_list);

    std::vector<EmailItem> copy;
    {
        std::lock_guard<std::mutex> lk(email_list_mutex);
        copy = email_list;
    }

    if (copy.empty()) {
        lv_label_set_text(_email_title, "Email");
        lv_label_set_text(_email_status, "No unread emails");
        return;
    }

    int total = email_unread_total.load();
    char buf[64];
    snprintf(buf, sizeof(buf), "Email (%d unread)", total);
    lv_label_set_text(_email_title, buf);
    lv_label_set_text(_email_status, "");

    for (const auto& m : copy) {
        // One row per folder: "[folder] from — subject", drawn manually instead of
        // using lv_list_add_button.
        char row_text[320];
        snprintf(row_text, sizeof(row_text), "[%s] %s — %s",
                 m.folder.c_str(), m.from.c_str(), m.subject.c_str());
        lv_obj_t* row = lv_obj_create(_email_list);
        lv_obj_set_size(row, LV_PCT(100), 56);
        lv_obj_set_style_bg_color(row, lv_color_hex(C_CARD_PR), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 10, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* lbl = lv_label_create(row);
        lv_label_set_text(lbl, row_text);
        lv_obj_set_style_text_font(lbl, zh_font_30(), 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(C_TEXT), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 16, 0);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
    }
}

void AppSettings::_emailShowError()
{
    if (!_email_status) return;
    lv_label_set_text(_email_status,
        ("Fetch failed: " + email_error_msg).c_str());
}
