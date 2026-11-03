
#include "clock.h"

#include <time.h>

void Clock::_clock_container_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) {
        return;
    }

    auto* self = static_cast<Clock*>(lv_event_get_user_data(e));

    if (!self) {
        return;
    }

    self->_container = nullptr;
    self->_stop_timer();
}


static const char *weekday_strs[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

void Clock::_clock_timer_cb(lv_timer_t* timer)
{
    auto* self = static_cast<Clock*>(lv_timer_get_user_data(timer));
    if (!self) return;

    time_t t = time(nullptr);
    struct tm lt = {};
#if defined(_WIN32)
    if (localtime_s(&lt, &t) != 0) return;
#else
    localtime_r(&t, &lt);
#endif

    lv_subject_set_int(&self->_sec_subject, lt.tm_sec);
    lv_subject_set_int(&self->_hour_subject, lt.tm_hour);
    lv_subject_set_int(&self->_min_subject, lt.tm_min);

    if( self->last_day != t/(60*60*24) ) {
        self->last_day = t/(60*60*24);

        char date_buf[32];
        lv_snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d", 
                 lt.tm_year + 1900, 
                 lt.tm_mon + 1, 
                 lt.tm_mday);
        
        if(self->_date_label != NULL) {
            lv_label_set_text(self->_date_label, date_buf);
        }
        //lv_label_set_text(weekday_label, weekday_strs[time_info->tm_wday]);
    }

}


lv_obj_t* Clock::_create(lv_obj_t* parent)
{
    // Initialize subjects for hours, minutes, and seconds
    lv_subject_init_int(&_hour_subject, 0);
    lv_subject_init_int(&_min_subject, 0);
    lv_subject_init_int(&_sec_subject, 0);

    // Create a main container for the clock
    _container = lv_obj_create(parent);
    //remve all style
    lv_obj_remove_style_all(_container);
    lv_obj_set_style_pad_all(_container, 0, 0);
    lv_obj_set_size(_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_ROW);  // date container on top, time container below
    lv_obj_set_flex_align(_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // add gap between date and time containers
    lv_obj_set_style_pad_column(_container, 20, 0); 

    // Add event callback to handle deletion of the clock container
    lv_obj_add_event_cb(_container, _clock_container_event_cb, LV_EVENT_DELETE, this);

    // time area 
    _time_container = lv_obj_create(_container);
    //transparent
    lv_obj_remove_style_all(_time_container);
    lv_obj_set_style_pad_all(_time_container, 0, 0);
    lv_obj_set_style_pad_column(_time_container, 10, 0);
    //lv_obj_set_style_bg_opa(_time_container, LV_OPA_TRANSP, 0);
    //lv_obj_set_style_border_width(_time_container, 0, 0);
    //lv_obj_set_style_pad_hor(_time_container, 0, 0);
    lv_obj_set_size(_time_container,LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(_time_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _hour_label = lv_label_create(_time_container);
    lv_obj_set_style_text_font(_hour_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_hour_label, lv_color_hex(0x4FA3FF), 0);
    lv_label_bind_text(_hour_label, &_hour_subject, "%02d:");
    lv_obj_align(_hour_label, LV_ALIGN_LEFT_MID, 0, 0);

    _min_label = lv_label_create(_time_container);
    lv_obj_set_style_text_font(_min_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_min_label, lv_color_hex(0x4FA3FF), 0);
    lv_label_bind_text(_min_label, &_min_subject, "%02d");
    lv_obj_align_to(_min_label, _hour_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    _sec_label = lv_label_create(_time_container);
    lv_obj_set_style_text_font(_sec_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_sec_label, lv_color_hex(0x4FA3FF), 0);
    lv_label_bind_text(_sec_label, &_sec_subject, ":%02d");
    lv_obj_align_to(_sec_label, _min_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0); 

    //date area
    _date_label = lv_label_create(_container);
    lv_obj_set_style_text_font(_date_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(_date_label, "----.--.--"); // 초기 텍스트


    // Create a timer to update the clock every second
    _clock_timer = lv_timer_create(_clock_timer_cb, 1000, this);
    lv_timer_set_user_data(_clock_timer, this);
    return _container;
}


//constructor
Clock::Clock(lv_obj_t* parent)
{
    _create(parent);
}

void Clock::_stop_timer()
{
    if (_clock_timer != nullptr) {
        lv_timer_del(_clock_timer);
        _clock_timer = nullptr;
    }
}

Clock::~Clock()
{
    if (_container != nullptr) {
        lv_obj_remove_event_cb_with_user_data(
            _container,
            _clock_container_event_cb,
            this);
        lv_obj_delete(_container);
        _container = nullptr;
    }

    _stop_timer();
    lv_subject_deinit(&_sec_subject);
    lv_subject_deinit(&_hour_subject);
    lv_subject_deinit(&_min_subject);
}