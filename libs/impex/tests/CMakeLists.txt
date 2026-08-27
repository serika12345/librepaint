kis_add_tests(
    TestImportExportBoundary.cpp
    LINK_LIBRARIES kritaimpex Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Xml
)

kis_add_tests(
    kis_document_metadata_test.cpp
    LINK_LIBRARIES kritaimpexmetadataobjects Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Xml
)
target_compile_definitions(kis_document_metadata_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    TestImportExportUiBoundary.cpp
    LINK_LIBRARIES kritaapplicationui Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Widgets
)
