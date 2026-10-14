# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.25)
project(QtAndroidNativeSurface VERSION 6.11.1 LANGUAGES C CXX)
set(QT_SKIP_AUTO_PLUGIN_INCLUSION ON)
set(QT_GENERATE_SBOM OFF)
find_package(Qt6 6.11.1 EXACT REQUIRED COMPONENTS BuildInternals Core CorePrivate Gui GuiPrivate)
qt_build_internals_set_up_private_api()
get_filename_component(QtBase_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${QtBase_SOURCE_DIR}/cmake")
add_subdirectory("${QtBase_SOURCE_DIR}/src/plugins/platforms/android" platform)
