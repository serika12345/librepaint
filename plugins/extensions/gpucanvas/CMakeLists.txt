# SPDX-License-Identifier: GPL-2.0-or-later
add_library(kritagpucanvaswindow STATIC KisGpuDocumentWindow.cpp KisGpuDocumentWindow.h)
target_include_directories(kritagpucanvaswindow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritagpucanvaswindow PUBLIC kritaapplicationgpucanvas Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritagpucanvaswindow PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
