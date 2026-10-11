# SPDX-License-Identifier: GPL-2.0-or-later
add_library(kritatoolsgpustrokeinput STATIC KisGpuStrokeInput.cpp)
target_include_directories(kritatoolsgpustrokeinput PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritatoolsgpustrokeinput PUBLIC kritapaintinggpubrushstroke)
set_target_properties(kritatoolsgpustrokeinput PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
add_library(kritatoolsgpupointerinput STATIC KisGpuPointerInput.cpp KisGpuPointerInput.h)
target_include_directories(kritatoolsgpupointerinput PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritatoolsgpupointerinput PUBLIC kritatoolsgpustrokeinput Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE kritatoolsquickshape)
set_target_properties(kritatoolsgpupointerinput PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
