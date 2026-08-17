kis_add_tests(
    TestImportExportBoundary.cpp
    LINK_LIBRARIES kritaimpex Qt${QT_MAJOR_VERSION}::Test
)

kis_add_tests(
    TestImportExportUiBoundary.cpp
    LINK_LIBRARIES kritaui Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Widgets
)
