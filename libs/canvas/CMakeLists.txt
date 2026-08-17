set(kritacanvas_LIB_SRCS
    kis_coordinates_converter.cpp
    KisCanvasState.cpp
)

kis_add_library(kritacanvas SHARED ${kritacanvas_LIB_SRCS})
generate_export_header(kritacanvas BASE_NAME kritacanvas)

target_link_libraries(kritacanvas
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        kritaflake
        kritaimage
)

target_include_directories(kritacanvas
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
)

set_target_properties(kritacanvas
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritacanvas ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
