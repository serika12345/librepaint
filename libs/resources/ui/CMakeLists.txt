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

set(kritaresourceui_LIB_SRCS
    KisResourceItemChooserContextMenu.cpp
    KisResourceItemChooser.cpp
    KisResourceItemChooserSync.cpp
    KisResourceItemDelegate.cpp
    KisResourceItemListView.cpp
    KisResourceItemListWidget.cpp
    KisResourceItemView.cpp
    KisTagChooserWidget.cpp
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
    $<TARGET_OBJECTS:kritataglabelobjects>
    $<TARGET_OBJECTS:kritaicontooltipobjects>
    $<TARGET_OBJECTS:kritaresourcethumbnailpainterobjects>
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
