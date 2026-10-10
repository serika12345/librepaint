# SPDX-License-Identifier: GPL-2.0-or-later
add_library(kritadocumentgpurecovery STATIC KisGpuDocumentRecoveryFile.cpp)
target_link_libraries(kritadocumentgpurecovery PUBLIC kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Core)
target_include_directories(kritadocumentgpurecovery PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(kritadocumentgpurecovery PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
