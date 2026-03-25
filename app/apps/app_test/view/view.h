/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <cstdint>
#include <smooth_lvgl.hpp>
#include <uitk/short_namespace.hpp>
#include <string_view>
#include <memory>
#include <vector>
#include <functional>

namespace view {

/**
 * @brief  App Test View
 */
class TestView {
public:
    void init();
    void update();
    void setOnQuit(std::function<void()> cb) { _on_quit = std::move(cb); }

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _container;
    std::function<void()>      _on_quit;
    std::unique_ptr<uitk::lvgl_cpp::Label>     _label;
    std::unique_ptr<uitk::lvgl_cpp::Button>    _button_quit;
};

}  // namespace view
