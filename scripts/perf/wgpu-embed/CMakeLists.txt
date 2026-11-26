# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.21)
project(wgpu_embed_check LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Gui GuiPrivate Widgets OpenGL OpenGLWidgets)

if(NOT DEFINED ENV{WGPU_NATIVE_INCLUDE} OR NOT DEFINED ENV{WGPU_NATIVE_LIB})
    message(FATAL_ERROR "run inside scripts/perf/wgpu-embed/env.nix (build.sh)")
endif()

add_executable(wgpu_embed_check wgpu_embed_check.cpp)
target_include_directories(wgpu_embed_check PRIVATE $ENV{WGPU_NATIVE_INCLUDE})
target_link_libraries(wgpu_embed_check PRIVATE
    Qt6::Gui Qt6::GuiPrivate Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
    $ENV{WGPU_NATIVE_LIB})

if(APPLE)
    enable_language(OBJCXX)
    target_sources(wgpu_embed_check PRIVATE metal_layer.mm)
    set_source_files_properties(metal_layer.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_link_libraries(wgpu_embed_check PRIVATE "-framework AppKit" "-framework QuartzCore" "-framework Metal")
endif()
