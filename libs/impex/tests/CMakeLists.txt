kis_add_tests(
    TestImportExportBoundary.cpp
    kis_document_metadata_test.cpp
    LINK_LIBRARIES kritaimpex Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Xml
)

kis_add_tests(
    TestImportExportUiBoundary.cpp
    LINK_LIBRARIES kritaapplicationui Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Widgets
)
