# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: BSD-3-Clause

add_library(kritastore INTERFACE)

target_link_libraries(kritastore
    INTERFACE
        kritaresourcestorage
)

target_include_directories(kritastore
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
