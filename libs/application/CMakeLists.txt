set(kritaapplication_LIB_SRCS
    kis_config.cc
    kis_snap_config.cpp
    platform-adapters/KisAndroidFileProxy.cpp
)

kis_add_library(kritaapplication SHARED ${kritaapplication_LIB_SRCS})
generate_export_header(kritaapplication BASE_NAME kritaapplication)

target_link_libraries(kritaapplication
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::ConfigCore
        kritaglobal
        kritapigment
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Xml
        kritacanvas
        kritaflake
        kritaresources
)

target_include_directories(kritaapplication
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

if(WIN32)
    set_source_files_properties(
        kis_config.cc
        PROPERTIES
        INCLUDE_DIRECTORIES "${Qt${QT_MAJOR_VERSION}Gui_PRIVATE_INCLUDE_DIRS}"
    )
endif()

set_target_properties(kritaapplication
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritaapplication ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
