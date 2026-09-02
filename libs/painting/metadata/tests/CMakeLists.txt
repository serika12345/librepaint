kis_add_tests(
    kis_meta_data_test.cpp
    LINK_LIBRARIES kritapaintingmetadata kritatestsdk
)

kis_add_test(
    KisMetaDataTagsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataTagsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KisMetaDataStoreSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataStoreSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisMetaDataStoreSchemaContractTest PRIVATE kritapaintingmetadata_EXPORTS)

kis_add_test(
    KisMetaDataIOBackendContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataIOBackendContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisMetaDataIOBackendContractTest PRIVATE kritapaintingmetadata_EXPORTS)

kis_add_test(
    KisMetaDataMergeStrategyContractTest.cpp
    LINK_LIBRARIES
        kritapaintingmetadatamergestrategyobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisMetaDataMergeStrategyContractTest PRIVATE kritapaintingmetadata_EXPORTS)

kis_add_test(
    KisMetaDataParserContractTest.cpp
    LINK_LIBRARIES
        kritapaintingmetadataparserobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataParserContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisMetaDataParserContractTest PRIVATE kritapaintingmetadata_EXPORTS)

kis_add_test(
    KisMetaDataValueContractTest.cpp
    LINK_LIBRARIES
        kritapaintingmetadataparserobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataValueContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisMetaDataValueContractTest PRIVATE kritapaintingmetadata_EXPORTS)

kis_add_test(
    KisMetaDataTypeInfoContractTest.cpp
    LINK_LIBRARIES
        kritapaintingmetadataparserobjects
        kritapaintingmetadatatypeschemaobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataTypeInfoContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisMetaDataTypeInfoContractTest PRIVATE kritapaintingmetadata_EXPORTS)
