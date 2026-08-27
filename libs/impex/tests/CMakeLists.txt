kis_add_tests(
    TestImportExportBoundary.cpp
    LINK_LIBRARIES kritaimpexadditionalchecksobjects kritaimpexfilterregistryobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(TestImportExportBoundary PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_import_export_filter_factory_test.cpp
    LINK_LIBRARIES kritaimpex Qt${QT_MAJOR_VERSION}::Test
)

kis_add_tests(
    kis_import_export_filter_test.cpp
    LINK_LIBRARIES
        kritaimpexfilterstateobjects
        kritaimpexfilterprogressobjects
        kritaimpexfilterconfigurationobjects
        kritaimpexfiltercapabilityobjects
        kritaimpexfilterverificationobjects
        kritaimpexexportcheckbaseobjects
        kritaimpexerrorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_import_export_filter_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_import_export_filter_saved_configuration_test.cpp
    LINK_LIBRARIES
        kritaimpexfilterstateobjects
        kritaimpexfilterprogressobjects
        kritaimpexfilterconfigurationobjects
        kritaimpexfiltersavedconfigurationobjects
        kritaimpexfiltercapabilityobjects
        kritaimpexfilterverificationobjects
        kritaimpexexportcheckbaseobjects
        kritaimpexerrorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_import_export_filter_saved_configuration_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_import_user_feedback_test.cpp
    TEST_NAMES_VAR import_user_feedback_test_names
    LINK_LIBRARIES
        kritaimpexuserfeedbackobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(kis_import_user_feedback_test PRIVATE kritaapplicationui_EXPORTS)
if(UNIX AND NOT APPLE)
    set_tests_properties(
        ${import_user_feedback_test_names}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

kis_add_tests(
    kis_animation_rendering_options_test.cpp
    LINK_LIBRARIES
        kritaimpexanimationoptionsstateobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_animation_rendering_options_test PRIVATE kritaapplicationui_EXPORTS)

kis_add_tests(
    kis_animation_rendering_options_persistence_test.cpp
    LINK_LIBRARIES
        kritaimpexanimationoptionsstateobjects
        kritaimpexanimationoptionspersistenceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_animation_rendering_options_persistence_test PRIVATE kritaapplicationui_EXPORTS)

kis_add_tests(
    kis_remote_file_fetcher_test.cpp
    TEST_NAMES_VAR remote_file_fetcher_test_names
    LINK_LIBRARIES
        kritaimpexremotefilefetcherobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(kis_remote_file_fetcher_test PRIVATE kritaapplicationui_EXPORTS)
if(UNIX AND NOT APPLE)
    set_tests_properties(
        ${remote_file_fetcher_test_names}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

kis_add_tests(
    TestImportExportPublicHeaders.cpp
    LINK_LIBRARIES kritaresourcestorage Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestImportExportPublicHeaders
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_tests(
    kis_store_paintdevice_writer_test.cpp
    LINK_LIBRARIES kritaresourcestorage Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(kis_store_paintdevice_writer_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_tests(
    kis_import_export_error_code_test.cpp
    LINK_LIBRARIES kritaimpexerrorobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_import_export_error_code_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_document_metadata_test.cpp
    LINK_LIBRARIES kritaimpexmetadataobjects Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Xml
)
target_compile_definitions(kis_document_metadata_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_export_check_base_test.cpp
    LINK_LIBRARIES kritaimpexexportcheckbaseobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_export_check_base_test PRIVATE kritaimpex_EXPORTS)
target_include_directories(kis_export_check_base_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_tests(
    kis_export_checks_test.cpp
    LINK_LIBRARIES
        kritaimpexexportcheckbaseobjects
        kritaimpexexportcheckregistryobjects
        kritaimpexpreexportcheckerobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_export_checks_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    TestImportExportUiBoundary.cpp
    LINK_LIBRARIES kritaapplicationui Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Widgets
)
