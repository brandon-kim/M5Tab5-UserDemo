/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <cstdint>
#include <memory>
#include <lvgl.h>
#include <vector>
#include <functional>

namespace view {

/**
 * @brief  App No Smooth Test View
 */
class NoSmoothView {
public:
    ~NoSmoothView();
    void init();
    void update();
    void setOnQuit(std::function<void()> cb) { _on_quit = std::move(cb); }

private:
    lv_obj_t* _container = nullptr;

    std::function<void()> _on_quit;
    
};

}  // namespace view
