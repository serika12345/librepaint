# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuTileStoreTest.cpp
    LINK_LIBRARIES kritaimagegpudocument Qt${QT_MAJOR_VERSION}::Test)
set_tests_properties(libs-image-gpu-KisGpuTileStoreTest PROPERTIES TIMEOUT 30 LABELS gpu)
