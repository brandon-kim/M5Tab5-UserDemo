/**
 * @file app_test.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2025-12-18
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
class AppTest : public mooncake::AppAbility {
public:
    AppTest();
	~AppTest();

    //  Override lifecycle callbacks
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    std::unique_ptr<test_view::TestView> _view;
};
