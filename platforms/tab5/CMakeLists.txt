# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)


# And optionally
set(CMAKE_COLOR_MAKEFILE ON)
# set(CMAKE_VERBOSE_MAKEFILE ON)
# Add other common application components

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(PROJECT_VER_MAJOR 0)
set(PROJECT_VER_MINOR 1)
set(PROJECT_VER_PATCH 0)
set(PROJECT_VER "${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}.${PROJECT_VER_PATCH}")

set(EXTRA_COMPONENT_DIRS 
    "../../dependencies"
)
 
# FreeType include path add
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/managed_components/espressif__freetype/freetype/include)

# LittleFS inlucd path add 
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/managed_components/joltwallet__littlefs/src/littlefs)

project(m5stack_tab5)

# Display Compile Time Information
message(STATUS "--------------Compile Info------------")
message(STATUS "IDF_PATH = ${IDF_PATH}")
message(STATUS "IDF_TARGET = ${IDF_TARGET}")
message(STATUS "PROJECT_NAME = ${PROJECT_NAME}")
#message(STATUS "PROJECTS_PATH = ${PROJECTS_PATH}")
#message(STATUS "PROJECT_DIR = ${PROJECT_DIR}")
message(STATUS "BUILD_DIR = ${BUILD_DIR}")
message(STATUS "SDKCONFIG = ${SDKCONFIG}")
message(STATUS "SDKCONFIG_DEFAULTS = ${SDKCONFIG_DEFAULTS}")
message(STATUS "COMPILE_OPTIONS = ${COMPILE_OPTIONS}")

