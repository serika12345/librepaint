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

add_library(kritapsdresourcevaluerecordobjects OBJECT
    psd_resource_value_records.cpp
)
target_compile_definitions(kritapsdresourcevaluerecordobjects PRIVATE kritapsd_EXPORTS)
target_include_directories(kritapsdresourcevaluerecordobjects
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libs/psdutils
        ${CMAKE_BINARY_DIR}/libs/psdutils
        ${CMAKE_SOURCE_DIR}/libs/pigment
        ${CMAKE_BINARY_DIR}/libs/pigment
        ${CMAKE_SOURCE_DIR}/libs/pigment/resources
        ${CMAKE_SOURCE_DIR}/libs/resources
        ${CMAKE_BINARY_DIR}/libs/resources
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
        ${CMAKE_SOURCE_DIR}/libs/flake
        ${CMAKE_BINARY_DIR}/libs/flake
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_SOURCE_DIR}/libs/image/generator
        ${CMAKE_SOURCE_DIR}/libs/image/filter
        $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(kritapsdresourcevaluerecordobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritapsdresourcevaluerecordobjects PROPERTIES
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
    $<TARGET_OBJECTS:kritapsdresourcevaluerecordobjects>
)
generate_export_header(kritapsd)

if (TARGET PsdFormatValuesContractTest)
    target_sources(PsdFormatValuesContractTest PRIVATE
        $<TARGET_OBJECTS:kritapsdresourcevaluerecordobjects>
    )
endif()

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
