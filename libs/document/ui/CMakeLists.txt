set(kritadocumentui_LIB_SRCS
    undo/kis_document_undo_store.cpp
    undo/kundo2model.cpp
    undo/kundo2view.cpp
)

kis_add_library(kritadocumentui SHARED ${kritadocumentui_LIB_SRCS})
generate_export_header(kritadocumentui BASE_NAME kritadocumentui)

target_link_libraries(kritadocumentui
    PUBLIC
        kritapaintingundo
        Qt${QT_MAJOR_VERSION}::Widgets
    PRIVATE
        KF${KF_MAJOR}::I18n
)

target_include_directories(kritadocumentui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(kritadocumentui PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritadocumentui ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
