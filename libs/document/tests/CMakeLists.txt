kis_add_tests(
    kis_document_autosave_state_test.cpp
    kis_document_identity_test.cpp
    kis_document_metadata_test.cpp
    kis_document_modification_state_test.cpp
    kis_document_recovery_autosave_state_test.cpp
    kis_document_recovery_status_test.cpp
    LINK_LIBRARIES kritadocument kritatestsdk Qt${QT_MAJOR_VERSION}::Xml
)
