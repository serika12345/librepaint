set(kritapaintingmetadata_LIB_SRCS
    kis_meta_data_entry.cc
    kis_meta_data_filter.cc
    kis_meta_data_filter_p.cc
    kis_meta_data_filter_registry.cc
    kis_meta_data_filter_registry_model.cc
    kis_meta_data_merge_strategy.cc
    kis_meta_data_merge_strategy_p.cc
    kis_meta_data_merge_strategy_registry.cc
    kis_meta_data_schema_registry.cc
    kis_meta_data_store.cc
    kis_meta_data_validator.cc
    kis_meta_data_backend_registry.cpp
)

add_library(kritapaintingmetadataparserobjects OBJECT
    kis_meta_data_parser.cc
    kis_meta_data_value.cc
)
target_compile_definitions(kritapaintingmetadataparserobjects PRIVATE kritapaintingmetadata_EXPORTS)
target_include_directories(kritapaintingmetadataparserobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintingmetadataparserobjects
    PUBLIC
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
)
set_target_properties(kritapaintingmetadataparserobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintingmetadatatypeschemaobjects OBJECT
    kis_meta_data_schema.cc
    kis_meta_data_type_info.cc
)
target_compile_definitions(kritapaintingmetadatatypeschemaobjects PRIVATE kritapaintingmetadata_EXPORTS)
target_include_directories(kritapaintingmetadatatypeschemaobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintingmetadatatypeschemaobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritapaintingmetadatatypeschemaobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

kis_add_library(kritapaintingmetadata SHARED
    ${kritapaintingmetadata_LIB_SRCS}
    $<TARGET_OBJECTS:kritapaintingmetadataparserobjects>
    $<TARGET_OBJECTS:kritapaintingmetadatatypeschemaobjects>
)
generate_export_header(kritapaintingmetadata BASE_NAME kritapaintingmetadata)

target_link_libraries(kritapaintingmetadata
    PUBLIC
        kritaversion
        kritaglobal
        kritaplugin
        kritawidgetutils
)

target_include_directories(kritapaintingmetadata
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_target_properties(kritapaintingmetadata PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritapaintingmetadata ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
