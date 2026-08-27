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
    TestImportExportPublicHeaders.cpp
    LINK_LIBRARIES kritaimage kritaresourcestorage Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestImportExportPublicHeaders PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

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
