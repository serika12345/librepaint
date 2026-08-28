add_library(kritaresourceuidescriptorobjects OBJECT
    KisResourceUiDescriptor.cpp
)
target_compile_definitions(kritaresourceuidescriptorobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuidescriptorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourceuidescriptorobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaresourceuidescriptorobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritaresourceui_LIB_SRCS
    KisIconToolTip.cpp
    KisResourceItemChooserContextMenu.cpp
    KisResourceItemChooser.cpp
    KisResourceItemChooserSync.cpp
    KisResourceItemDelegate.cpp
    KisResourceItemListView.cpp
    KisResourceItemListWidget.cpp
    KisResourceItemView.cpp
    KisResourceThumbnailPainter.cpp
    KisTagChooserWidget.cpp
    KisTagLabel.cpp
    KisTagFilterWidget.cpp
    KisTagToolButton.cpp
    KisResourceTaggingManager.cpp
    KisStorageChooserWidget.cpp
    TagActions.cpp
    KisResourceUserOperations.cpp
)

kis_add_library(kritaresourceui SHARED
    ${kritaresourceui_LIB_SRCS}
    $<TARGET_OBJECTS:kritaresourceuidescriptorobjects>
)

generate_export_header(kritaresourceui BASE_NAME kritaresourceui)

target_link_libraries(kritaresourceui
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        kritaresources
        kritawidgetutils
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Sql
        kritaversion
        kritaglobal
        kritaplugin
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::WidgetsAddons
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::I18n
)

set_target_properties(kritaresourceui PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)
install(TARGETS kritaresourceui ${INSTALL_TARGETS_DEFAULT_ARGS} )

add_subdirectory(tests)
