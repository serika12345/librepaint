kis_add_tests(
    kis_meta_data_test.cpp
    LINK_LIBRARIES kritapaintingmetadata kritatestsdk
)

kis_add_test(
    KisMetaDataAnonymizerCompatibilityTest.cpp
    LINK_LIBRARIES
        kritapaintingmetadata
        kritatestsdk
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataAnonymizerCompatibilityTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisMetaDataTagsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataTagsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)





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
    KisMetaDataValueContractTest.cpp
    LINK_LIBRARIES
        kritapaintingmetadata
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataValueContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisMetaDataTypeInfoContractTest.cpp
    LINK_LIBRARIES
        kritapaintingmetadata
        kritatestsdk
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMetaDataTypeInfoContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
