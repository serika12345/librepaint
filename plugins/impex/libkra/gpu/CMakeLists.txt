# SPDX-License-Identifier: GPL-2.0-or-later
add_library(kritagpukrasave STATIC KisGpuKraSaveJob.cpp)
target_link_libraries(kritagpukrasave PUBLIC kritaimagegpudocument PRIVATE kritaapplicationui)
target_include_directories(kritagpukrasave PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(kritagpukrasave PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
