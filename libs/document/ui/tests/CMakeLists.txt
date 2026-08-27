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
