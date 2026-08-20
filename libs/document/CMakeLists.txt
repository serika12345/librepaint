set(kritadocument_LIB_SRCS
    metadata/KoDocumentInfo.cpp
    session/kis_document_autosave_state.cpp
    session/kis_document_identity.cpp
    session/kis_document_modification_state.cpp
    session/kis_document_recovery_autosave_state.cpp
    session/kis_document_recovery_status.cpp
)

kis_add_library(kritadocument SHARED ${kritadocument_LIB_SRCS})
generate_export_header(kritadocument BASE_NAME kritadocument)

target_link_libraries(kritadocument
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        kritaresources
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
)

target_include_directories(kritadocument
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(kritadocument PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritadocument ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(ui)
add_subdirectory(tests)
