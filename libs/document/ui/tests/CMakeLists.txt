kis_add_tests(
    kis_document_autosave_recovery_dialog_test.cpp
    LINK_LIBRARIES kritadocumentautosaverecoveryobjects Qt${QT_MAJOR_VERSION}::Test
)

kis_add_tests(
    kis_document_info_dialog_test.cpp
    LINK_LIBRARIES kritadocumentinfoobjects kritaimpexmetadataobjects Qt${QT_MAJOR_VERSION}::Test
)

kis_add_tests(
    kis_document_io_presentation_test.cpp
    LINK_LIBRARIES kritadocumentiopresentationobjects kritaimpexerrorobjects Qt${QT_MAJOR_VERSION}::Test
)

kis_add_tests(
    kis_document_named_autosave_dialog_test.cpp
    LINK_LIBRARIES kritadocumentnamedrecoveryobjects Qt${QT_MAJOR_VERSION}::Test
)

kis_add_tests(
    kis_document_undo_store_test.cpp
    kis_document_undo_ui_test.cpp
    LINK_LIBRARIES kritadocumentundoobjects kritapaintingundo Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisDocumentUiLifecycleSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDocumentUiLifecycleSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}/../../files
    ${CMAKE_SOURCE_DIR}/libs/widgets
    ${CMAKE_BINARY_DIR}/libs/widgets
    $<TARGET_PROPERTY:KF${KF_MAJOR}::ConfigCore,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::WidgetsAddons,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisDocumentUiLifecycleSchemaContractTest PRIVATE
    kritadocumentfiles_EXPORTS
    kritadocumentui_EXPORTS
    kritawidgets_EXPORTS
)

set(kritadocumentui_TEST_TARGETS
    kis_document_autosave_recovery_dialog_test
    kis_document_info_dialog_test
    kis_document_io_presentation_test
    kis_document_named_autosave_dialog_test
    kis_document_undo_store_test
    kis_document_undo_ui_test
)

foreach(target IN LISTS kritadocumentui_TEST_TARGETS)
    target_compile_definitions(${target} PRIVATE kritadocumentui_EXPORTS)
endforeach()
