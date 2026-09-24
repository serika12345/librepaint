cmake_minimum_required(VERSION 3.19)
project(LibrePaintAndroidTestPackage LANGUAGES CXX)

if(NOT ANDROID_TEST_LIBRARY)
    message(FATAL_ERROR "ANDROID_TEST_LIBRARY is required")
endif()
if(NOT EXISTS "${ANDROID_TEST_LIBRARY}")
    message(FATAL_ERROR "Android test library does not exist: ${ANDROID_TEST_LIBRARY}")
endif()
if(NOT ANDROID_TEST_APK_DIR)
    message(FATAL_ERROR "ANDROID_TEST_APK_DIR is required")
endif()

find_package(ECM REQUIRED NO_MODULE)
find_package(Qt5Core REQUIRED)

add_library(librepaint_android_test SHARED IMPORTED GLOBAL)
set_target_properties(librepaint_android_test PROPERTIES
    IMPORTED_LOCATION "${ANDROID_TEST_LIBRARY}"
)

set(ANDROID_SDK_COMPILE_API "35")
set(ANDROID_SYSROOT_PREFIX "${ANDROID_TOOLCHAIN_ROOT}/sysroot/usr" CACHE STRING "" FORCE)
string(REGEX REPLACE "-(clang)?([0-9].[0-9])?$" "" ECM_ANDROID_STL_ARCH "${ANDROID_TOOLCHAIN_NAME}")

if(NOT CMAKE_CXX_STANDARD_LIBRARIES MATCHES "[^ ]*c\\+\\+[^ ]*\\.so")
    set(KRITA_ANDROID_STL_PATH
        "${ANDROID_SYSROOT_PREFIX}/lib/${ECM_ANDROID_STL_ARCH}/lib${ANDROID_STL}.so")
    if(NOT EXISTS "${KRITA_ANDROID_STL_PATH}")
        message(FATAL_ERROR "Android shared STL was not found")
    endif()
    set(CMAKE_CXX_STANDARD_LIBRARIES
        "${CMAKE_CXX_STANDARD_LIBRARIES} \"${KRITA_ANDROID_STL_PATH}\"" CACHE STRING "" FORCE)
endif()

set(_CMAKE_ANDROID_DIR "${ECM_DIR}/../toolchain")
include("${_CMAKE_ANDROID_DIR}/ECMAndroidDeployQt.cmake")
ecm_androiddeployqt(librepaint_android_test "${ECM_ADDITIONAL_FIND_ROOT_PATH}")
set_target_properties(create-apk-librepaint_android_test PROPERTIES
    ANDROID_APK_DIR "${ANDROID_TEST_APK_DIR}"
)

file(WRITE "${CMAKE_BINARY_DIR}/stl" "\"${KRITA_ANDROID_STL_PATH}\"")
file(WRITE "${CMAKE_BINARY_DIR}/ranlib" "${CMAKE_RANLIB}")
