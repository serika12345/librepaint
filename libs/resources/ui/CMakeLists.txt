set(kritaresourceui_LIB_SRCS
    KisResourceUiDescriptor.cpp
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

kis_add_library(kritaresourceui SHARED ${kritaresourceui_LIB_SRCS})

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
