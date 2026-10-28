#include "page_base_container.h"

namespace ui {

PageBaseContainer::PageBaseContainer(lv_obj_t* screen)
{
    if (!screen) {
        screen = lv_screen_active();
    }
    _container = screen;
    lv_obj_remove_style_all(_container);
    lv_obj_set_size(_container, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);
}

PageBaseContainer::~PageBaseContainer()
{
    removeSwipeGesture();
    _container = nullptr;
}

void PageBaseContainer::installSwipeGesture(std::function<void()> callback, lv_dir_t direction)
{
    removeSwipeGesture();
    _gesture_callback = std::move(callback);
    _gesture_direction = direction;

    lv_indev_t* indev = lv_indev_get_next(nullptr);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            lv_indev_add_event_cb(indev, _gestureEventCb, LV_EVENT_GESTURE, this);
            _gesture_indev = indev;
            return;
        }
        indev = lv_indev_get_next(indev);
    }
}

void PageBaseContainer::removeSwipeGesture()
{
    if (_gesture_indev) {
        lv_indev_remove_event_cb_with_user_data(_gesture_indev, nullptr, this);
        _gesture_indev = nullptr;
    }
    _gesture_callback = {};
}

void PageBaseContainer::_gestureEventCb(lv_event_t* event)
{
    auto* container = static_cast<PageBaseContainer*>(lv_event_get_user_data(event));
    if (!container || !container->_gesture_callback) return;

    lv_indev_t* indev = static_cast<lv_indev_t*>(lv_event_get_target(event));
    if (lv_indev_get_gesture_dir(indev) == container->_gesture_direction) {
        container->_gesture_callback();
    }
}

}  // namespace ui
