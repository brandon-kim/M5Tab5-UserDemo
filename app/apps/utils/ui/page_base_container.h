#pragma once

#include <lvgl.h>

#include <functional>

namespace ui {

class PageBaseContainer {
public:
    explicit PageBaseContainer(lv_obj_t* screen = nullptr);
    ~PageBaseContainer();

    PageBaseContainer(const PageBaseContainer&) = delete;
    PageBaseContainer& operator=(const PageBaseContainer&) = delete;

    lv_obj_t* get() const { return _container; }
    operator lv_obj_t*() const { return _container; }

    void installSwipeGesture(std::function<void()> callback, lv_dir_t direction = LV_DIR_TOP,
                            bool oneShot = true);
    void removeSwipeGesture();

private:
    static void _gestureEventCb(lv_event_t* event);

    lv_obj_t* _container = nullptr;
    lv_indev_t* _gesture_indev = nullptr;
    lv_dir_t _gesture_direction = LV_DIR_TOP;
    std::function<void()> _gesture_callback;
    bool _gesture_one_shot = true;
};

}  // namespace ui
