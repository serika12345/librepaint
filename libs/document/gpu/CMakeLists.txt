# SPDX-License-Identifier: GPL-2.0-or-later
add_library(kritadocumentgpudevice STATIC KisGpuDocumentDevice.cpp KisGpuDocumentDevice.h)
target_include_directories(kritadocumentgpudevice PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritadocumentgpudevice PUBLIC kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritadocumentgpudevice PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
