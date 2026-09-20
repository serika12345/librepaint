set(kritadocumentinfo_OBJECT_SRCS
    info/KoDocumentInfoDlg.cpp
)

ki18n_wrap_ui(kritadocumentinfo_OBJECT_SRCS
    info/koDocumentInfoAboutWidget.ui
    info/koDocumentInfoAuthorWidget.ui
)

set(kritadocumentiopresentation_OBJECT_SRCS
    io/kis_document_io_presentation.cpp
)

set(kritadocumentautosaverecovery_OBJECT_SRCS
    recovery/KisAutoSaveRecoveryDialog.cpp
)

set(kritadocumentnamedrecovery_OBJECT_SRCS
    recovery/KisRecoverNamedAutosaveDialog.cpp
)

ki18n_wrap_ui(kritadocumentnamedrecovery_OBJECT_SRCS
    recovery/KisRecoverNamedAutosaveDialog.ui
)

set(kritadocumentundo_OBJECT_SRCS
    undo/kis_document_undo_store.cpp
    undo/kundo2model.cpp
    undo/kundo2view.cpp
)

add_library(kritadocumentinfoobjects OBJECT ${kritadocumentinfo_OBJECT_SRCS})
add_library(kritadocumentiopresentationobjects OBJECT ${kritadocumentiopresentation_OBJECT_SRCS})
add_library(kritadocumentautosaverecoveryobjects OBJECT ${kritadocumentautosaverecovery_OBJECT_SRCS})
add_library(kritadocumentnamedrecoveryobjects OBJECT ${kritadocumentnamedrecovery_OBJECT_SRCS})
add_library(kritadocumentundoobjects OBJECT ${kritadocumentundo_OBJECT_SRCS})

set(kritadocumentui_OBJECT_TARGETS
    kritadocumentinfoobjects
    kritadocumentiopresentationobjects
    kritadocumentautosaverecoveryobjects
    kritadocumentnamedrecoveryobjects
    kritadocumentundoobjects
)

foreach(target IN LISTS kritadocumentui_OBJECT_TARGETS)
    target_compile_definitions(${target} PRIVATE kritadocumentui_EXPORTS)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endforeach()

target_link_libraries(kritadocumentinfoobjects
    PUBLIC
        kritaimpexmetadataobjects
        kritaplugin
        kritawidgets
        kritawidgetutils
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)

target_link_libraries(kritadocumentiopresentationobjects
    PUBLIC
        kritaimpexerrorobjects
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)

target_link_libraries(kritadocumentautosaverecoveryobjects
    PUBLIC
        kritadocumentfiles
        kritawidgets
        kritawidgetutils
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ItemViews
)

target_link_libraries(kritadocumentnamedrecoveryobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)

target_link_libraries(kritadocumentundoobjects
    PUBLIC
        kritapaintingundo
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)

kis_add_library(kritadocumentui SHARED
    $<TARGET_OBJECTS:kritadocumentinfoobjects>
    $<TARGET_OBJECTS:kritadocumentiopresentationobjects>
    $<TARGET_OBJECTS:kritadocumentautosaverecoveryobjects>
    $<TARGET_OBJECTS:kritadocumentnamedrecoveryobjects>
    $<TARGET_OBJECTS:kritadocumentundoobjects>
)
generate_export_header(kritadocumentui BASE_NAME kritadocumentui)

target_link_libraries(kritadocumentui
    PUBLIC
        kritapaintingundo
        kritawidgets
        kritawidgetutils
        kritadocumentfiles
        Qt${QT_MAJOR_VERSION}::Widgets
    PRIVATE
        kritadocument
        kritaimpex
        kritaplugin
        kritaresourcestorage
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ItemViews
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
