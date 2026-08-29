add_subdirectory(tests)

configure_file(xmlgui/config-xmlgui.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-xmlgui.h )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/config)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/xmlgui)

add_library(kritawidgetutilsnumericparserobjects OBJECT
    kis_num_parser.cpp
)
target_compile_definitions(kritawidgetutilsnumericparserobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritawidgetutilsnumericparserobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetutilsnumericparserobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritawidgetutilsnumericparserobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritawidgetutilspropertiesobjects OBJECT
    KoProperties.cpp
)
target_compile_definitions(kritawidgetutilspropertiesobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritawidgetutilspropertiesobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetutilspropertiesobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritawidgetutilspropertiesobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritasqueezedcomboboxobjects OBJECT
    KisSqueezedComboBox.cpp
)
target_compile_definitions(kritasqueezedcomboboxobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritasqueezedcomboboxobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritasqueezedcomboboxobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritasqueezedcomboboxobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritacheckerboardpainterobjects OBJECT
    KoCheckerBoardPainter.cpp
)
target_compile_definitions(kritacheckerboardpainterobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritacheckerboardpainterobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritacheckerboardpainterobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritacheckerboardpainterobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaitemtooltipobjects OBJECT
    KoItemToolTip.cpp
)
target_compile_definitions(kritaitemtooltipobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritaitemtooltipobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaitemtooltipobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritaitemtooltipobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritakineticscrollerobjects OBJECT
    KisKineticScroller.cpp
)
target_compile_definitions(kritakineticscrollerobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritakineticscrollerobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritakineticscrollerobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::ConfigCore
)
set_target_properties(kritakineticscrollerobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritapopupbuttonobjects OBJECT
    KisPopupButton.cpp
)
target_compile_definitions(kritapopupbuttonobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritapopupbuttonobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapopupbuttonobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritapopupbuttonobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritamenustyledontcloseonaltobjects OBJECT
    KisMenuStyleDontCloseOnAlt.cpp
)
target_compile_definitions(kritamenustyledontcloseonaltobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritamenustyledontcloseonaltobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritamenustyledontcloseonaltobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritamenustyledontcloseonaltobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapopupselfactivatinglineeditobjects OBJECT
    KisPopupSelfActivatingLineEdit.cpp
)
target_compile_definitions(kritapopupselfactivatinglineeditobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritapopupselfactivatinglineeditobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapopupselfactivatinglineeditobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritapopupselfactivatinglineeditobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritaspinboxi18nhelperobjects OBJECT
    KisSpinBoxI18nHelper.cpp
)
target_compile_definitions(kritaspinboxi18nhelperobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritaspinboxi18nhelperobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaspinboxi18nhelperobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritaspinboxi18nhelperobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritawidgetutilsprogressupdaterobjects OBJECT
    KoProgressProxy.cpp
    KoProgressProxy.h
    KoUpdater.cpp
    KoUpdater.h
    KoUpdaterPrivate_p.cpp
    KoUpdaterPrivate_p.h
)
target_compile_definitions(kritawidgetutilsprogressupdaterobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritawidgetutilsprogressupdaterobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetutilsprogressupdaterobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritawidgetutilsprogressupdaterobjects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritawidgetutilsfakeprogressproxyobjects OBJECT
    KoFakeProgressProxy.cpp
)
target_compile_definitions(kritawidgetutilsfakeprogressproxyobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritawidgetutilsfakeprogressproxyobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetutilsfakeprogressproxyobjects
    PUBLIC
        kritawidgetutilsprogressupdaterobjects
        Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritawidgetutilsfakeprogressproxyobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritawidgetutilsoptioncollectionobjects OBJECT
    KisOptionCollectionWidget.cpp
)
target_compile_definitions(kritawidgetutilsoptioncollectionobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritawidgetutilsoptioncollectionobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritawidgetutilsoptioncollectionobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritawidgetutilsoptioncollectionobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritawidgetutilswrappablehboxlayoutobjects OBJECT
    KisWrappableHBoxLayout.cpp
)
target_compile_definitions(kritawidgetutilswrappablehboxlayoutobjects PRIVATE kritawidgetutils_EXPORTS)
target_include_directories(kritawidgetutilswrappablehboxlayoutobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetutilswrappablehboxlayoutobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritawidgetutilswrappablehboxlayoutobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(kritawidgetutils_LIB_SRCS

    $<TARGET_OBJECTS:kritawidgetutilsnumericparserobjects>
    $<TARGET_OBJECTS:kritawidgetutilspropertiesobjects>
    $<TARGET_OBJECTS:kritasqueezedcomboboxobjects>
    $<TARGET_OBJECTS:kritacheckerboardpainterobjects>
    $<TARGET_OBJECTS:kritaitemtooltipobjects>
    $<TARGET_OBJECTS:kritakineticscrollerobjects>
    $<TARGET_OBJECTS:kritapopupbuttonobjects>
    $<TARGET_OBJECTS:kritamenustyledontcloseonaltobjects>
    $<TARGET_OBJECTS:kritapopupselfactivatinglineeditobjects>
    $<TARGET_OBJECTS:kritaspinboxi18nhelperobjects>
    $<TARGET_OBJECTS:kritawidgetutilsprogressupdaterobjects>
    $<TARGET_OBJECTS:kritawidgetutilsfakeprogressproxyobjects>
    $<TARGET_OBJECTS:kritawidgetutilsoptioncollectionobjects>
    $<TARGET_OBJECTS:kritawidgetutilswrappablehboxlayoutobjects>

    WidgetUtilsDebug.cpp

    kis_icon_utils.cpp
    kis_action_registry.cpp
    KisActionsSnapshot.cpp
    KoGroupButton.cpp
    KoProgressBar.cpp
    KoProgressUpdater.cpp
    KoFileDialog.cpp
    KisPreviewFileDialog.cpp
    KisDialogStateSaver.cpp
    kis_cursor.cc
    kis_cursor_cache.cpp

    kis_double_parse_spin_box.cpp
    kis_double_parse_unit_spin_box.cpp
    kis_int_parse_spin_box.cpp
    kis_slider_spin_box.cpp
    kis_multipliers_double_slider_spinbox.cpp
    kis_spin_box_unit_manager.cpp
    kis_font_family_combo_box.cpp
    KisFontFamilyValidator.cpp
    KisRecentFilesManager.cpp
    KisOptionButtonStrip.cpp
    KisRepaintDebugger.cpp
    KisScreenMigrationTracker.cpp
    KisRootSurfaceTrackerBase.cpp

    config/khelpclient.cpp
    config/klanguagebutton.cpp
    config/krecentfilesaction.cpp
    config/kstandardaction.cpp

    xmlgui/KisShortcutsEditorItem.cpp
    xmlgui/KisShortcutEditWidget.cpp
    xmlgui/KisShortcutsEditorDelegate.cpp
    xmlgui/KisShortcutsDialog.cpp
    xmlgui/KisShortcutsDialog_p.cpp
    xmlgui/KisShortcutsEditor.cpp
    xmlgui/KisShortcutsEditor_p.cpp

    xmlgui/kshortcutschemeseditor.cpp
    xmlgui/kshortcutschemeshelper.cpp

    xmlgui/kactioncategory.cpp
    xmlgui/kactioncollection.cpp
    xmlgui/kcheckaccelerators.cpp
    xmlgui/kedittoolbar.cpp
    xmlgui/khelpmenu.cpp
    xmlgui/kkeysequencewidget.cpp
    xmlgui/kmainwindow.cpp
    xmlgui/kshortcutwidget.cpp
    xmlgui/kswitchlanguagedialog_p.cpp
    xmlgui/ktoggletoolbaraction.cpp
    xmlgui/ktoolbar.cpp
    xmlgui/ktoolbarhandler.cpp
    xmlgui/KisToolBarStateModel.cpp
    xmlgui/kundoactions.cpp
    xmlgui/kxmlguibuilder.cpp
    xmlgui/kxmlguiclient.cpp
    xmlgui/kxmlguifactory.cpp
    xmlgui/kxmlguifactory_p.cpp
    xmlgui/kxmlguiversionhandler.cpp
    xmlgui/kxmlguiwindow.cpp

    katecommandbar.cpp
    commandmodel.cpp

    KisClickableLabel.cpp
)

if (QT_MAJOR_VERSION STREQUAL "5")
    set(kritawidgetutils_LIB_SRCS ${kritawidgetutils_LIB_SRCS}
        config/kcolorscheme.cpp
    )
endif()

# Add QtGui's private headers to KKeySequenceWidget, which are
# required for the shortcuts translation
set_source_files_properties(xmlgui/kkeysequencewidget.cpp
    PROPERTIES
    INCLUDE_DIRECTORIES "${Qt${QT_MAJOR_VERSION}Gui_PRIVATE_INCLUDE_DIRS}"
    )

if (HAVE_DBUS)
    set(kritawidgetutils_LIB_SRCS ${kritawidgetutils_LIB_SRCS}
        xmlgui/kmainwindowiface.cpp
    )
endif()

ki18n_wrap_ui(kritawidgetutils_LIB_SRCS
    xmlgui/KisShortcutsDialog.ui
    xmlgui/kshortcutwidget.ui
)

qt_add_resources(kritawidgetutils_LIB_SRCS xmlgui/kxmlgui.qrc)
krita_ios_add_static_resource(kxmlgui)

kis_add_library(kritawidgetutils SHARED ${kritawidgetutils_LIB_SRCS})

target_include_directories(kritawidgetutils
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/xmlgui>
)

generate_export_header(kritawidgetutils BASE_NAME kritawidgetutils)

if (HAVE_DBUS)
   set (KRITA_WIDGET_UTILS_EXTRA_LIBS ${KRITA_WIDGET_UTILS_EXTRA_LIBS} Qt${QT_MAJOR_VERSION}::DBus)
endif ()

if (APPLE AND NOT IOS)
    find_library(FOUNDATION_LIBRARY Foundation)
    set(KRITA_WIDGET_UTILS_EXTRA_LIBS ${KRITA_WIDGET_UTILS_EXTRA_LIBS} ${FOUNDATION_LIBRARY})
    target_include_directories(kritawidgetutils PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../macosutils>
    )
    target_link_libraries(kritawidgetutils PRIVATE kritamacosutils)
endif ()

if (ANDROID)
    target_link_libraries(kritawidgetutils PRIVATE Qt${QT_MAJOR_VERSION}::AndroidExtras)
endif()

target_link_libraries(kritawidgetutils
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Core
        KF${KF_MAJOR}::ItemViews
        kritaglobal
        kritaresources
    PRIVATE
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::ConfigGui
        KF${KF_MAJOR}::GuiAddons
        KF${KF_MAJOR}::WidgetsAddons
        kritaplugin
        
        ${KRITA_WIDGET_UTILS_EXTRA_LIBS}
)

if(NOT IOS)
    target_link_libraries(kritawidgetutils PRIVATE Qt${QT_MAJOR_VERSION}::PrintSupport)
endif()

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(kritawidgetutils PRIVATE KF6::ColorScheme)
endif()

set_target_properties(kritawidgetutils
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritawidgetutils ${INSTALL_TARGETS_DEFAULT_ARGS})
