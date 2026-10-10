# SPDX-License-Identifier: GPL-2.0-or-later
qt_add_resources(gpu_canvas_resources gpu_canvas.qrc)
add_library(kritacanvasgpurenderer STATIC GpuRenderer.cpp ${gpu_canvas_resources})
target_include_directories(kritacanvasgpurenderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritacanvasgpurenderer PUBLIC kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritacanvasgpurenderer PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
