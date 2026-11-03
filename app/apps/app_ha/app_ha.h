#pragma once
#include <mooncake.h>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>
#include "ha_client.h"
#include "view/view.h"

class AppHA : public mooncake::AppAbility {
public:
    AppHA();
    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    std::shared_ptr<HaClient> _ha;
    std::unique_ptr<ha_view::HaView> _view;
    uint32_t _last_ui_update   = 0;
    uint32_t _last_weather_ms  = 0;

    ha_view::WeatherInfo _weather;
    std::mutex           _weather_mutex;

    bool _fetch_weather_async();  // true once HA weather data is available

    std::atomic<bool> _closing{false};

    // ─── Worker pool (detached + counter) ────────────────────────────────────
    // Previously a std::vector<std::thread> was appended to and only joined in
    // onClose — every switch/fan-adjust leaked one 16 KB pthread stack in
    // internal SRAM until the app closed. After ~50 toggles the device OOM'd.
    // Workers now run detached and bump a counter; onClose waits on a CV.
    std::atomic<int>         _active_workers{0};
    std::mutex               _worker_mutex;
    std::condition_variable  _worker_cv;
    void _start_worker(std::function<void()> fn);
    void _join_workers();
};
