# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuDeviceBudgetTest.cpp LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuDeviceBudgetTest PRIVATE KisGpuTestDevice.cpp)
set_tests_properties(libs-image-gpu-KisGpuDeviceBudgetTest PROPERTIES TIMEOUT 30 LABELS gpu)

kis_add_test(KisGpuRecoveryCompactionTest.cpp LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuRecoveryCompactionTest PRIVATE KisGpuTestDevice.cpp)
set_tests_properties(libs-image-gpu-KisGpuRecoveryCompactionTest PROPERTIES TIMEOUT 30 LABELS gpu)

kis_add_test(KisGpuBrushTextureTest.cpp
    LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuBrushTextureTest PRIVATE KisGpuTestDevice.cpp)
target_compile_definitions(KisGpuBrushTextureTest PRIVATE
    TEXTURE_FIXTURE="${CMAKE_SOURCE_DIR}/libs/painting/tests/data/brush_texture_contract.json")
set_tests_properties(libs-image-gpu-KisGpuBrushTextureTest PROPERTIES TIMEOUT 30 LABELS gpu)

kis_add_test(KisGpuSelectionSessionTest.cpp
    LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuSelectionSessionTest PRIVATE KisGpuTestDevice.cpp)
set_tests_properties(libs-image-gpu-KisGpuSelectionSessionTest PROPERTIES TIMEOUT 30 LABELS gpu)

kis_add_test(KisGpuDabSelectionTest.cpp
    LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuDabSelectionTest PRIVATE KisGpuTestDevice.cpp)
set_tests_properties(libs-image-gpu-KisGpuDabSelectionTest PROPERTIES TIMEOUT 30 LABELS gpu)

kis_add_test(KisGpuTileTextureTest.cpp
    LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuTileTextureTest PRIVATE KisGpuTestDevice.cpp)
set_tests_properties(libs-image-gpu-KisGpuTileTextureTest PROPERTIES TIMEOUT 30 LABELS gpu)

kis_add_test(KisGpuTileStoreTest.cpp
    LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuTileStoreTest PRIVATE KisGpuTestDevice.cpp)
set_tests_properties(libs-image-gpu-KisGpuTileStoreTest PROPERTIES TIMEOUT 30 LABELS gpu)
target_compile_definitions(KisGpuTileStoreTest PRIVATE
    RASTER_EDIT_FIXTURE="${CMAKE_SOURCE_DIR}/libs/image/tests/data/raster_edit_contract.json")

kis_add_test(KisGpuEditSessionTest.cpp
    LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuEditSessionTest PRIVATE KisGpuTestDevice.cpp)
set_tests_properties(libs-image-gpu-KisGpuEditSessionTest PROPERTIES TIMEOUT 30 LABELS gpu)

kis_add_test(KisGpuNativeFailureTest.cpp
    LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
set_tests_properties(libs-image-gpu-KisGpuNativeFailureTest PROPERTIES TIMEOUT 60 LABELS gpu)

add_executable(KisGpuPaintBenchmark EXCLUDE_FROM_ALL KisGpuPaintBenchmark.cpp KisGpuTestDevice.cpp)
target_link_libraries(KisGpuPaintBenchmark PRIVATE kritaimagegpudocument)
set_target_properties(KisGpuPaintBenchmark PROPERTIES AUTOMOC OFF MACOSX_BUNDLE OFF)
