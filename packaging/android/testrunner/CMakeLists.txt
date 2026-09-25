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

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_policy(SET QTP0002 NEW)

add_library(librepaint_android_test_case SHARED IMPORTED GLOBAL)
set_target_properties(librepaint_android_test_case PROPERTIES
    IMPORTED_LOCATION "${ANDROID_TEST_LIBRARY}"
)

qt_add_executable(librepaint_android_test MANUAL_FINALIZATION
    "${CMAKE_CURRENT_LIST_DIR}/../../../cmake/modules/KritaAndroidTestMain.cpp"
)
target_link_libraries(librepaint_android_test PRIVATE
    librepaint_android_test_case
    Qt6::Core
)
set_target_properties(librepaint_android_test PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_TEST_APK_DIR}"
)

qt_finalize_executable(librepaint_android_test)
add_custom_target(create-apk-librepaint_android_test
    DEPENDS librepaint_android_test_make_apk
)
