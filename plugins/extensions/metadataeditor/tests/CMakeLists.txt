kis_add_test(
    KisEntryEditorContractTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kis_entry_editor.cc
    TEST_NAME KisEntryEditorContractTest
    LINK_LIBRARIES
        kritapaintingmetadata
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisEntryEditorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
