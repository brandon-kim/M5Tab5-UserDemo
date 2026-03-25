/**
 * @file app_no_smooth.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-03-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#include "view/view.h"
#include <mooncake.h>
#include <stdint.h>

/**
 * @brief derived App
 *
 */
class AppNoSmooth : public mooncake::AppAbility {
public:
    AppNoSmooth();
	~AppNoSmooth();

    // Override lifecycle callbacks
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    std::unique_ptr<view::NoSmoothView> _view;
};
