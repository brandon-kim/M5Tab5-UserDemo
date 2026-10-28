/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <lvgl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <apps/utils/ui/page_base_container.h>

namespace view {

/**
 * @brief  App No Smooth Test View
 */
class NoSmoothView {
public:
    ~NoSmoothView();
    void init();
    void update();
    void setOnQuit(std::function<void()> cb) { _container->installSwipeGesture(std::move(cb)); }

private:
    std::unique_ptr<ui::PageBaseContainer> _container;
    
};

}  // namespace view
