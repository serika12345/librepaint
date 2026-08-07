cmake_minimum_required(VERSION 3.24)
project(KritaIOSFrameworksProbe LANGUAGES CXX)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6GuiPrivate 6.11 REQUIRED)
find_package(KF6Config 6.28 REQUIRED)
find_package(KF6WidgetsAddons 6.28 REQUIRED)
find_package(KF6Codecs 6.28 REQUIRED)
find_package(KF6Completion 6.28 REQUIRED)
find_package(KF6CoreAddons 6.28 REQUIRED)
find_package(KF6GuiAddons 6.28 REQUIRED)
find_package(KF6I18n 6.28 REQUIRED)
find_package(KF6ItemViews 6.28 REQUIRED)
find_package(KF6ColorScheme 6.28 REQUIRED)
find_library(ICONV_LIBRARY iconv REQUIRED)

qt_standard_project_setup()
qt_add_executable(KritaIOSFrameworksProbe MACOSX_BUNDLE main.cpp)
kconfig_add_kcfg_files(KritaIOSFrameworksProbe probe.kcfgc)
target_include_directories(KritaIOSFrameworksProbe PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(
    KritaIOSFrameworksProbe
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        KF6::ConfigCore
        KF6::ConfigGui
        KF6::WidgetsAddons
        KF6::Codecs
        KF6::Completion
        KF6::CoreAddons
        KF6::GuiAddons
        KF6::I18n
        KF6::I18nLocaleData
        KF6::ItemViews
        KF6::ColorScheme
        ${ICONV_LIBRARY}
)
set_target_properties(
    KritaIOSFrameworksProbe
    PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER local.librepaint.ipad.frameworks-probe
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
)

install(TARGETS KritaIOSFrameworksProbe BUNDLE DESTINATION bin)
