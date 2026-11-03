/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <smooth_ui_toolkit.hpp>
#include <uitk/short_namespace.hpp>
#include <smooth_lvgl.hpp>
#include <cstdint>
#include <string_view>
#include <memory>
#include <vector>
#include <functional>
#include <apps/utils/ui/page_base_container.h>

#define  USE_APP_LOADING_ANIMATION  0

namespace view {

/**
 * @brief  App Test View
 */
class TestView {
public:
    void init();
    void update();
    void setOnQuit(std::function<void()> cb) { _container->installSwipeGesture(std::move(cb)); }

private:
    std::unique_ptr<ui::PageBaseContainer> _container;
    std::unique_ptr<uitk::lvgl_cpp::Label> _label;

};

}  // namespace view
