
**Warning: This code is intended for testing purchased hardware and may not function correctly in all environments. Use at your own risk.**

# M5Tab5 User Demo ( Modified for Windows 11 Emulation ) 

User demo source code of [M5Tab5](https://docs.m5stack.com/en/products/sku/k145).

## Updates

- Updated LVGL component to v9.4.0
- Updated dependency components and modified related code:
  - **LVGL:** v9.4.0
  - **Mooncake:** v2.3.3
  - **Mooncake Log:** v1.4.0
  - **Smooth UI Toolkit:** v2.11.0



## Build


### Install SDL and the build tools [https://github.com/lvgl/lv_port_pc_vscode](https://github.com/lvgl/lv_port_pc_vscode)

- **Windows (vcpkg):** `vcpkg install sdl2`  (`vcpkg` can be installed from [https://github.com/microsoft/vcpkg](https://github.com/microsoft/vcpkg)) Also install either MinGW or another compiler and `cmake`.
- **macOS (Homebrew):** `brew install sdl2 cmake make`  
- **Linux:**  
  - **Debian/Ubuntu:** `sudo apt install build-essential cmake libsdl2-dev`  
  - **Arch:** `sudo pacman -S base-devel cmake sdl2`  
  - **Fedora:** `sudo dnf install @development-tools cmake SDL2-devel`  
- **Manual Installation of SDL:** Download from [SDL’s website](https://github.com/libsdl-org/SDL/releases) and place headers/libraries in your project.
- **Verify Installation:** `sdl2-config --version`, `cmake --version`, `gcc --version`, `g++ --version` (should return the installed version).  

### Fetch Dependencies

```bash
python ./fetch_repos.py
```

## Desktop Build

#### Tool Chains

```bash
sudo apt install build-essential cmake libsdl2-dev
```

#### Build

```bash
mkdir build && cd build
```

```bash
cmake .. && make -j8
```

#### Run

```bash
./desktop/app_desktop_build
```

## IDF Build

#### Tool Chains

[ESP-IDF v5.4.2](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32s3/index.html)

#### Build

```bash
cd platforms/tab5
```

```bash
idf.py build
```

#### Flash

```bash
idf.py flash
```

## Acknowledgments

This project references the following open-source libraries and resources:

- https://github.com/lvgl/lvgl
- https://www.heroui.com
- https://github.com/Forairaaaaa/smooth_ui_toolkit
- https://github.com/Forairaaaaa/mooncake
- https://github.com/Forairaaaaa/mooncake_log
- https://github.com/alexreinert/piVCCU/blob/master/kernel/rtc-rx8130.c
- https://components.espressif.com/components/espressif/esp_cam_sensor
- https://components.espressif.com/components/espressif/esp_ipa
- https://components.espressif.com/components/espressif/esp_sccb_intf
- https://components.espressif.com/components/espressif/esp_video
- https://components.espressif.com/components/espressif/esp_lvgl_port
- https://github.com/jarzebski/Arduino-INA226
- https://github.com/boschsensortec/BMI270_SensorAPI
