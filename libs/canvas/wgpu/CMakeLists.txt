# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-2.0-or-later

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.19)
    project(LibrePaintWgpuCanvasProbe LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/modules)
    find_package(Qt6 REQUIRED COMPONENTS Gui Widgets Test)
    include(CTest)
endif()

find_package(WgpuNative REQUIRED)
add_library(kritacanvaswgpuimage STATIC WgpuImageRenderer.cpp)
target_include_directories(kritacanvaswgpuimage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritacanvaswgpuimage PUBLIC Qt6::Gui WgpuNative::WgpuNative)
set_target_properties(kritacanvaswgpuimage PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(APPLE)
        enable_language(OBJCXX)
    endif()
    add_subdirectory(../../ui/canvas/wgpu canvas-window)
endif()

if(BUILD_TESTING)
    add_executable(WgpuImageRendererTest WgpuImageRendererTest.cpp)
    target_link_libraries(WgpuImageRendererTest PRIVATE kritacanvaswgpuimage Qt6::Test)
    set_target_properties(WgpuImageRendererTest PROPERTIES AUTOMOC ON MACOSX_BUNDLE OFF)
    add_test(NAME libs-canvas-wgpu-WgpuImageRendererTest COMMAND WgpuImageRendererTest)
    set_tests_properties(libs-canvas-wgpu-WgpuImageRendererTest PROPERTIES TIMEOUT 30)
endif()
