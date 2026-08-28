kis_add_tests(
    TestResourceUiContract.cpp
    LINK_LIBRARIES kritaresourceui Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    ResourceListViewModesContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(ResourceListViewModesContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
