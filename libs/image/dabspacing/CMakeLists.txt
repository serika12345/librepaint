# SPDX-License-Identifier: GPL-2.0-or-later
add_library(kritaimagedabspacing STATIC KisDabSpacing.cpp)
target_include_directories(kritaimagedabspacing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritaimagedabspacing PUBLIC Qt${QT_MAJOR_VERSION}::Core PRIVATE Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaimagedabspacing PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
