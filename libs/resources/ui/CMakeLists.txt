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

add_library(kritataglabelobjects OBJECT
    KisTagLabel.cpp
)
target_compile_definitions(kritataglabelobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritataglabelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritataglabelobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritataglabelobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritaicontooltipobjects OBJECT
    KisIconToolTip.cpp
)
target_compile_definitions(kritaicontooltipobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaicontooltipobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaicontooltipobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaicontooltipobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcethumbnailpainterobjects OBJECT
    KisResourceThumbnailPainter.cpp
)
target_compile_definitions(kritaresourcethumbnailpainterobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourcethumbnailpainterobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourcethumbnailpainterobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourcethumbnailpainterobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritaresourceitemdelegateobjects OBJECT
    KisResourceItemDelegate.cpp
)
target_compile_definitions(kritaresourceitemdelegateobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceitemdelegateobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourceitemdelegateobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceitemdelegateobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritaresourceitemchoosersyncobjects OBJECT
    KisResourceItemChooserSync.cpp
)
target_compile_definitions(kritaresourceitemchoosersyncobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceitemchoosersyncobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourceitemchoosersyncobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaresourceitemchoosersyncobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritaresourceitemviewobjects OBJECT
    KisResourceItemView.cpp
)
target_compile_definitions(kritaresourceitemviewobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceitemviewobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritaresourceitemviewobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritaresourceitemviewobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritaresourceitemlistviewobjects OBJECT
    KisResourceItemListView.cpp
)
target_compile_definitions(kritaresourceitemlistviewobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceitemlistviewobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritaresourceitemlistviewobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritaresourceitemlistviewobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritaresourceitemlistwidgetobjects OBJECT
    KisResourceItemListWidget.cpp
)
target_compile_definitions(kritaresourceitemlistwidgetobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceitemlistwidgetobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritaresourceitemlistwidgetobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritaresourceitemlistwidgetobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritastoragechooserdelegateobjects OBJECT
    KisStorageChooserDelegate.cpp
)
target_compile_definitions(kritastoragechooserdelegateobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritastoragechooserdelegateobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritastoragechooserdelegateobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritastoragechooserdelegateobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritastoragechooserwidgetobjects OBJECT
    KisStorageChooserWidget.cpp
)
target_compile_definitions(kritastoragechooserwidgetobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritastoragechooserwidgetobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritastoragechooserwidgetobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritastoragechooserwidgetobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritastoragechooserwidgetactivationobjects OBJECT
    KisStorageChooserWidgetActivation.cpp
)
target_compile_definitions(kritastoragechooserwidgetactivationobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritastoragechooserwidgetactivationobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritastoragechooserwidgetactivationobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritastoragechooserwidgetactivationobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritastoragechooserwidgetmodelsourceobjects OBJECT
    KisStorageChooserWidgetModelSource.cpp
)
target_compile_definitions(kritastoragechooserwidgetmodelsourceobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritastoragechooserwidgetmodelsourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritastoragechooserwidgetmodelsourceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritastoragechooserwidgetmodelsourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsaddobjects OBJECT
    KisResourceUserOperationsAdd.cpp
)
target_compile_definitions(kritaresourceuseroperationsaddobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsaddobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourceuseroperationsaddobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsaddobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsaddsourceobjects OBJECT
    KisResourceUserOperationsAddSource.cpp
)
target_compile_definitions(kritaresourceuseroperationsaddsourceobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsaddsourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritaresourceuseroperationsaddsourceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsaddsourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsupdateobjects OBJECT
    KisResourceUserOperationsUpdate.cpp
)
target_compile_definitions(kritaresourceuseroperationsupdateobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsupdateobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourceuseroperationsupdateobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsupdateobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsupdatesourceobjects OBJECT
    KisResourceUserOperationsUpdateSource.cpp
)
target_compile_definitions(kritaresourceuseroperationsupdatesourceobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsupdatesourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritaresourceuseroperationsupdatesourceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsupdatesourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsoverwriteobjects OBJECT
    KisResourceUserOperationsOverwrite.cpp
)
target_compile_definitions(kritaresourceuseroperationsoverwriteobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsoverwriteobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritaresourceuseroperationsoverwriteobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsoverwriteobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsnameusageobjects OBJECT
    KisResourceUserOperationsNameUsage.cpp
)
target_compile_definitions(kritaresourceuseroperationsnameusageobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsnameusageobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritaresourceuseroperationsnameusageobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsnameusageobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsnamesourceobjects OBJECT
    KisResourceUserOperationsNameSource.cpp
)
target_compile_definitions(kritaresourceuseroperationsnamesourceobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsnamesourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritaresourceuseroperationsnamesourceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsnamesourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsimportobjects OBJECT
    KisResourceUserOperationsImport.cpp
)
target_compile_definitions(kritaresourceuseroperationsimportobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsimportobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourceuseroperationsimportobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsimportobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsimportsourceobjects OBJECT
    KisResourceUserOperationsImportSource.cpp
)
target_compile_definitions(kritaresourceuseroperationsimportsourceobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsimportsourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritaresourceuseroperationsimportsourceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsimportsourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsrenameobjects OBJECT
    KisResourceUserOperationsRename.cpp
)
target_compile_definitions(kritaresourceuseroperationsrenameobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsrenameobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourceuseroperationsrenameobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsrenameobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceuseroperationsrenamesourceobjects OBJECT
    KisResourceUserOperationsRenameSource.cpp
)
target_compile_definitions(kritaresourceuseroperationsrenamesourceobjects PRIVATE kritaresourceui_EXPORTS)
target_include_directories(kritaresourceuseroperationsrenamesourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritaresourceuseroperationsrenamesourceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceuseroperationsrenamesourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritaresourceui_LIB_SRCS
    KisResourceItemChooserContextMenu.cpp
    KisResourceItemChooser.cpp
    KisTagChooserWidget.cpp
    KisTagFilterWidget.cpp
    KisTagToolButton.cpp
    KisResourceTaggingManager.cpp
    TagActions.cpp
)

kis_add_library(kritaresourceui SHARED
    ${kritaresourceui_LIB_SRCS}
    $<TARGET_OBJECTS:kritaresourceuidescriptorobjects>
    $<TARGET_OBJECTS:kritataglabelobjects>
    $<TARGET_OBJECTS:kritaicontooltipobjects>
    $<TARGET_OBJECTS:kritaresourcethumbnailpainterobjects>
    $<TARGET_OBJECTS:kritaresourceitemdelegateobjects>
    $<TARGET_OBJECTS:kritaresourceitemchoosersyncobjects>
    $<TARGET_OBJECTS:kritaresourceitemviewobjects>
    $<TARGET_OBJECTS:kritaresourceitemlistviewobjects>
    $<TARGET_OBJECTS:kritaresourceitemlistwidgetobjects>
    $<TARGET_OBJECTS:kritastoragechooserdelegateobjects>
    $<TARGET_OBJECTS:kritastoragechooserwidgetobjects>
    $<TARGET_OBJECTS:kritastoragechooserwidgetactivationobjects>
    $<TARGET_OBJECTS:kritastoragechooserwidgetmodelsourceobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsaddobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsaddsourceobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsupdateobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsupdatesourceobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsoverwriteobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsnameusageobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsnamesourceobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsimportobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsimportsourceobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsrenameobjects>
    $<TARGET_OBJECTS:kritaresourceuseroperationsrenamesourceobjects>
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
