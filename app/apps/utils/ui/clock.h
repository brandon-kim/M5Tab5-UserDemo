#pragma once

#include <lvgl.h>

class Clock {
public:
    explicit Clock(lv_obj_t* parent);
    ~Clock();

    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;

    lv_obj_t* _container = nullptr;
    lv_obj_t* _time_container = nullptr;
    lv_obj_t* _hour_label;
    lv_obj_t* _min_label;
    lv_obj_t* _sec_label;

    lv_obj_t* _date_label = nullptr;
    
private:
    lv_subject_t _sec_subject;
    lv_subject_t _min_subject;
    lv_subject_t _hour_subject;

    lv_timer_t* _clock_timer = nullptr;

    uint64_t last_day = -1; // Last processed day (Day) for date change detection

    lv_obj_t* _create(lv_obj_t* parent);

    static void _clock_container_event_cb(lv_event_t *e);
    static void _clock_timer_cb(lv_timer_t* timer);
    void _stop_timer();
};