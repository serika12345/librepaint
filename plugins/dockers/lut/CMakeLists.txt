add_subdirectory(tests)

set(kritalutdocker_static_SRCS
  lutdocker_dock.cpp
  black_white_point_chooser.cpp
)

ki18n_wrap_ui(kritalutdocker_static_SRCS
  wdglut.ui
)

set(kritalutdocker_static_SRCS
  ${kritalutdocker_static_SRCS}
  ocio_display_filter_vfx2021.cpp
)

kis_add_library(kritalutdocker_static STATIC ${kritalutdocker_static_SRCS})

target_link_libraries(kritalutdocker_static PUBLIC kritaapplication kritaui OpenColorIO::OpenColorIO)

if(IOS)
    # Krita's OCIO display filter uses this compatibility define to select its
    # existing OpenGL ES path. Qt 6 exposes the capability through QT_CONFIG
    # but no longer defines this legacy platform macro for consumers.
    target_compile_definitions(kritalutdocker_static PRIVATE QT_OPENGL_ES_2)
endif()

set(KRITA_LUTDOCKER_SOURCES
  lutdocker.cpp
)

kis_add_library(kritalutdocker MODULE ${KRITA_LUTDOCKER_SOURCES})

target_link_libraries(kritalutdocker kritaapplication kritalutdocker_static)
install(TARGETS kritalutdocker  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
