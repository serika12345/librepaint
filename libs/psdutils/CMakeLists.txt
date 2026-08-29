set(kritapsdutils_LIB_SRCS
    psd.cpp

    asl/kis_asl_reader.cpp
    asl/kis_asl_xml_parser.cpp
    asl/kis_asl_xml_writer.cpp
    asl/kis_asl_patterns_writer.cpp
    asl/kis_asl_writer.cpp

    cos/kis_cos_writer.cpp
    cos/kis_txt2_utls.cpp
    cos/psd_text_data_converter.cpp
)

add_library(kritapsdcosparserobjects OBJECT
    cos/kis_cos_parser.cpp
)
target_compile_definitions(kritapsdcosparserobjects PRIVATE kritapsdutils_EXPORTS)
target_include_directories(kritapsdcosparserobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapsdcosparserobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
)
if (TARGET Qt6::Core5Compat)
    target_link_libraries(kritapsdcosparserobjects PUBLIC Qt6::Core5Compat)
endif()
set_target_properties(kritapsdcosparserobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapsdcompressionobjects OBJECT
    compression.cpp
)
target_compile_definitions(kritapsdcompressionobjects PRIVATE kritapsdutils_EXPORTS)
target_include_directories(kritapsdcompressionobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapsdcompressionobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        ZLIB::ZLIB
)
set_target_properties(kritapsdcompressionobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaaslobjectcatcherobjects OBJECT
    asl/kis_asl_object_catcher.cpp
    asl/kis_asl_callback_object_catcher.cpp
)
target_compile_definitions(kritaaslobjectcatcherobjects PRIVATE kritapsdutils_EXPORTS)
target_include_directories(kritaaslobjectcatcherobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/asl>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
)
target_link_libraries(kritaaslobjectcatcherobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
        KF${KF_MAJOR}::I18n
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritaaslobjectcatcherobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

kis_add_library(kritapsdutils SHARED
    ${kritapsdutils_LIB_SRCS}
    $<TARGET_OBJECTS:kritapsdcosparserobjects>
    $<TARGET_OBJECTS:kritapsdcompressionobjects>
    $<TARGET_OBJECTS:kritaaslobjectcatcherobjects>
)
generate_export_header(kritapsdutils BASE_NAME kritapsdutils)

target_link_libraries(kritapsdutils
    PUBLIC
        kritapigment
        kritaglobal
        kritaflake
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE
        ZLIB::ZLIB
)

set_target_properties(kritapsdutils PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritapsdutils  ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
