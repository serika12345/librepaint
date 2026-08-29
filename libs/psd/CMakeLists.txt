add_library(kritapsdheaderobjects OBJECT
    psd_header.cpp
)
target_compile_definitions(kritapsdheaderobjects PRIVATE kritapsd_EXPORTS)
target_include_directories(kritapsdheaderobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/psdutils>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapsdheaderobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritapsdheaderobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritapsd_SRCS
    psd_additional_layer_info_block.cpp
    psd_layer_record.cpp
    psd_layer_section.cpp
    psd_pixel_utils.cpp
    psd_resource_block.cpp
    psd_resource_section.cpp
)

kis_add_library(kritapsd SHARED
    ${kritapsd_SRCS}
    $<TARGET_OBJECTS:kritapsdheaderobjects>
)
generate_export_header(kritapsd)

target_link_libraries(kritapsd
    PUBLIC
        kritaimage
        kritapsdutils
        kritaapplicationui
)

set_target_properties(kritapsd PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritapsd ${INSTALL_TARGETS_DEFAULT_ARGS})
