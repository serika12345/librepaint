kis_add_tests(
    TestResourceUiContract.cpp
    LINK_LIBRARIES kritaresourceui Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    ResourceListViewModesContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(ResourceListViewModesContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisResourceUiDescriptorContractTest.cpp
    LINK_LIBRARIES
        kritaresourceuidescriptorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisResourceUiDescriptorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisResourceUiDescriptorContractTest PRIVATE kritaresourceui_EXPORTS)

kis_add_test(
    KisTagLabelContractTest.cpp
    LINK_LIBRARIES
        kritataglabelobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisTagLabelContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisTagLabelContractTest PRIVATE kritaresourceui_EXPORTS)
