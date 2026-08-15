cmake_minimum_required(VERSION 3.25)

project(KritaIOSSmoke LANGUAGES OBJCXX)

add_executable(KritaIOSSmoke MACOSX_BUNDLE main.mm)
target_compile_features(KritaIOSSmoke PRIVATE cxx_std_17)
target_link_libraries(KritaIOSSmoke PRIVATE "-framework Foundation" "-framework UIKit")

set_target_properties(KritaIOSSmoke PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "LibrePaint iOS Smoke"
    MACOSX_BUNDLE_GUI_IDENTIFIER "local.librepaint.ipad.smoke"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    MACOSX_BUNDLE_BUNDLE_VERSION "1"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "local.librepaint.ipad.smoke"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
)
