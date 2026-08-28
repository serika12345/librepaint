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
