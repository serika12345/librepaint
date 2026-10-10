# SPDX-License-Identifier: GPL-2.0-or-later
add_library(kritapaintinggpubrushstroke STATIC KisGpuBrushStroke.cpp)
target_include_directories(kritapaintinggpubrushstroke PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritapaintinggpubrushstroke PUBLIC kritaimagegpudocument PRIVATE kritaimagedabspacing)
set_target_properties(kritapaintinggpubrushstroke PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
