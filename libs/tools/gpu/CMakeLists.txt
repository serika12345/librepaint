# SPDX-License-Identifier: GPL-2.0-or-later
add_library(kritatoolsgpustrokeinput STATIC KisGpuStrokeInput.cpp)
target_include_directories(kritatoolsgpustrokeinput PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritatoolsgpustrokeinput PUBLIC kritapaintinggpubrushstroke)
set_target_properties(kritatoolsgpustrokeinput PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
