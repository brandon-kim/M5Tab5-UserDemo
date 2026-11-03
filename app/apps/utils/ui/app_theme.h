/* theme constants and resources */
#pragma once
#include <cstdint>
#include <lvgl.h>

/* font resources */
extern "C" {
    extern const lv_font_t   lv_font_montserrat_20;
    extern const lv_font_t   lv_font_montserrat_30;
    extern const lv_font_t   lv_font_montserrat_40;
}

namespace app_theme {
    /* color constants */
    constexpr uint32_t COLOR_PRIMARY = 0xFF0000; // Red
    constexpr uint32_t COLOR_SECONDARY = 0x00FF00; // Green
    constexpr uint32_t COLOR_TERTIARY = 0x0000FF; // Blue
    constexpr uint32_t COLOR_BACKGROUND = 0xFFFFFF; // White    
    constexpr uint32_t COLOR_TEXT = 0x000000; // Black
    constexpr uint32_t COLOR_ACCENT = 0xFFFF00; // Yellow

    /* offset constants */
    constexpr int16_t OFFSET_X = 10;
    constexpr int16_t OFFSET_Y = 10;

    /* screen constants */
    constexpr int16_t SCREEN_WIDTH = 1280;
    constexpr int16_t SCREEN_HEIGHT = 720;

    inline const lv_font_t* font_small() {
        return &lv_font_montserrat_20;
    }

    inline const lv_font_t* font_medium() {
        return &lv_font_montserrat_30;
    }

    inline const lv_font_t* font_large() {
        return &lv_font_montserrat_40;
    }


    inline void make_read_only(lv_obj_t* o)
    {
        lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICK_FOCUSABLE); // Prevent focus (such as an input cursor) from being created when clicked
        lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);          // Preserve click/touch events
        lv_obj_remove_state(o, LV_STATE_FOCUSED);            // Clear the focused state
        lv_group_remove_obj(o);                             // Exclude from the physical keyboard Tab focus group
    }

}