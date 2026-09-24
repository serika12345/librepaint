cmake_minimum_required(VERSION 3.19)
project(LibrePaintAndroidPackage LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS
  Concurrent
  Core
  Core5Compat
  Gui
  Network
  OpenGL
  OpenGLWidgets
  PrintSupport
  Qml
  Quick
  QuickControls2
  QuickWidgets
  Sql
  Svg
  SvgWidgets
  Widgets
  Xml
)

add_library(krita MODULE IMPORTED GLOBAL)
set_target_properties(krita PROPERTIES
  IMPORTED_LOCATION "${NATIVE_PREFIX}/lib/libkrita_@androidAbi@.so"
  QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/apk"
)
target_link_libraries(krita INTERFACE
  Qt6::Concurrent
  Qt6::Core
  Qt6::Core5Compat
  Qt6::Gui
  Qt6::Network
  Qt6::OpenGL
  Qt6::OpenGLWidgets
  Qt6::PrintSupport
  Qt6::Qml
  Qt6::Quick
  Qt6::QuickControls2
  Qt6::QuickWidgets
  Qt6::Sql
  Qt6::Svg
  Qt6::SvgWidgets
  Qt6::Widgets
  Qt6::Xml
)

if(ANDROID_EXTRA_LIBS)
  set_target_properties(krita PROPERTIES
    QT_ANDROID_EXTRA_LIBS "${ANDROID_EXTRA_LIBS}"
  )
endif()

qt_android_generate_deployment_settings(krita)
qt_android_add_apk_target(krita)
add_custom_target(create-apk-krita
  DEPENDS krita_make_apk
)
add_custom_target(create-aab-krita
  DEPENDS krita_make_aab
)
