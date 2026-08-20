set(kritadocumentfiles_LIB_SRCS
    kis_document_autosave_files.cpp
    kis_document_backup_file.cpp
    kis_document_save_target.cpp
)

kis_add_library(kritadocumentfiles SHARED ${kritadocumentfiles_LIB_SRCS})
generate_export_header(kritadocumentfiles BASE_NAME kritadocumentfiles)

target_link_libraries(kritadocumentfiles
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        kritaglobal
        kritaresourcestorage
)

target_include_directories(kritadocumentfiles
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(kritadocumentfiles PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritadocumentfiles ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
