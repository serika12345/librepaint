# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-2.0-or-later

include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/LibrePaintSDKProfile.cmake")
if(NOT CMAKE_SYSTEM_NAME STREQUAL LibrePaintSDK_SYSTEM OR
   NOT CMAKE_SYSTEM_PROCESSOR STREQUAL LibrePaintSDK_PROCESSOR)
    message(FATAL_ERROR "This LibrePaint SDK targets ${LibrePaintSDK_SYSTEM}/${LibrePaintSDK_PROCESSOR}. Rebuild the plugin with the matching SDK toolchain.")
endif()
file(REAL_PATH "${CMAKE_CXX_COMPILER}" _librepaint_compiler)
file(REAL_PATH "${LibrePaintSDK_CXX_COMPILER}" _librepaint_sdk_compiler)
if(NOT _librepaint_compiler STREQUAL _librepaint_sdk_compiler)
    message(FATAL_ERROR "The plugin compiler differs from the LibrePaint SDK. Configure a new build directory with -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/LibrePaintSDKToolchain.cmake in the pinned development environment.")
endif()

find_package(Qt6 ${LibrePaintSDK_QT_VERSION} EXACT REQUIRED COMPONENTS Core Gui Widgets Xml Test)
find_package(KF6I18n REQUIRED)
find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}" ${ECM_MODULE_PATH})
include(KritaAddBrokenUnitTest)
include("${CMAKE_CURRENT_LIST_DIR}/LibrePaintSDKTargets.cmake")
add_library(kritatestsdk INTERFACE IMPORTED)
set_target_properties(kritatestsdk PROPERTIES
    INTERFACE_LINK_LIBRARIES Qt6::Test
    INTERFACE_INCLUDE_DIRECTORIES "${_librepaint_sdk_root}/include/source/sdk/tests"
    INTERFACE_COMPILE_DEFINITIONS "FILES_DEFAULT_DATA_DIR=\"${_librepaint_sdk_root}/share/sdk/tests/data/\";SYSTEM_RESOURCES_DATA_DIR=\"${_librepaint_sdk_root}/share/krita/data/\";LIBREPAINT_SDK_TEST_SOURCE_DIR=\"${_librepaint_sdk_root}/include/source\";LIBREPAINT_SDK_TEST_RESOURCE_DIR=\"${_librepaint_sdk_root}/share/krita/data\";LIBREPAINT_SDK_TEST_PLUGIN_DIR=\"${CMAKE_BINARY_DIR}/plugin\"")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_SHARED_MODULE_PREFIX "")
list(APPEND CMAKE_AUTOMOC_MACRO_NAMES K_PLUGIN_FACTORY_WITH_JSON K_PLUGIN_CLASS_WITH_JSON)
set(QT_MAJOR_VERSION 6)
set(KF_MAJOR 6)
set(KRITA_PLUGIN_INSTALL_DIR "lib/librepaint/plugins")
set(KDE_INSTALL_DATADIR "share")
set(KDE_INSTALL_APPDIR "share/applications")
set(LibrePaintSDK_FOUND TRUE)

# External plugins retain the same library declaration and optional PCH argument.
# Precompiled headers belong to each consumer's own build tree.
function(kis_add_library)
    cmake_parse_arguments(PARSE_ARGV 0 _library "" "PCH_FILE" "")
    add_library(${_library_UNPARSED_ARGUMENTS})
endfunction()
