kis_add_tests(
    KisActionsSnapshotTest.cpp
    TestKoProgressUpdater.cpp
    kis_parse_spin_boxes_test.cpp
    LINK_LIBRARIES kritawidgetutils kritaimage kritatestsdk
)

kis_add_test(
    KoUpdaterContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsprogressupdaterobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoUpdaterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoUpdaterContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KoFakeProgressProxyContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsfakeprogressproxyobjects
        kritawidgetutilsprogressupdaterobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoFakeProgressProxyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoFakeProgressProxyContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KoProgressBarContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsprogressbarobjects
        kritawidgetutilsprogressupdaterobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KoProgressBarContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoProgressBarContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KoPropertiesTest.cpp
    LINK_LIBRARIES
        kritawidgetutilspropertiesobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoPropertiesTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoPropertiesTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    TestSqueezedComboBox.cpp
    LINK_LIBRARIES
        kritasqueezedcomboboxobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(TestSqueezedComboBox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(TestSqueezedComboBox PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KoItemToolTipContractTest.cpp
    LINK_LIBRARIES
        kritaitemtooltipobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KoItemToolTipContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoItemToolTipContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisRepaintDebuggerContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsrepaintdebuggerobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisRepaintDebuggerContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisRepaintDebuggerContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisOptionButtonStripContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsoptionbuttonstripobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
target_include_directories(KisOptionButtonStripContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisOptionButtonStripContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisToolBarStateModelContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilstoolbarstatemodelobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Core
        lager
)
target_compile_definitions(KisToolBarStateModelContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisRootSurfaceTrackerBaseContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsrootsurfacetrackerobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisRootSurfaceTrackerBaseContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisRootSurfaceTrackerBaseContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisScreenMigrationTrackerContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsscreenmigrationtrackerobjects
        kritawidgetutilsrootsurfacetrackerobjects
        kritaglobalsignalcompressorobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisScreenMigrationTrackerContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisScreenMigrationTrackerContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    kis_simple_math_parser_test.cpp
    LINK_LIBRARIES kritawidgetutilsnumericparserobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_simple_math_parser_test PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisIntParseSpinBoxContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsintparsespinboxobjects
        kritawidgetutilsnumericparserobjects
        kritaglobalpaintingtweaksobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisIntParseSpinBoxContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisDoubleParseSpinBoxContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsdoubleparsespinboxobjects
        kritawidgetutilsnumericparserobjects
        kritaglobalpaintingtweaksobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisDoubleParseSpinBoxContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KStandardActionEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigGui
)
target_include_directories(KStandardActionEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KStandardActionEnumContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KColorSchemeEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::ConfigCore
)
target_include_directories(KColorSchemeEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KColorSchemeEnumContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KoCheckerBoardPainterContractTest.cpp
    LINK_LIBRARIES
        kritacheckerboardpainterobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
)
target_include_directories(KoCheckerBoardPainterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoCheckerBoardPainterContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisKineticScrollerContractTest.cpp
    LINK_LIBRARIES
        kritakineticscrollerobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisKineticScrollerContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisPopupButtonContractTest.cpp
    LINK_LIBRARIES
        kritapopupbuttonobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisPopupButtonContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisPopupButtonContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisHighlightedToolButtonContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisHighlightedToolButtonContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisHighlightedToolButtonContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisClickableLabelContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsclickablelabelobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisClickableLabelContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisClickableLabelContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisRecentFilesManagerContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsrecentfilesmanagerobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::ConfigCore
)
target_compile_definitions(KisRecentFilesManagerContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KRecentFilesActionContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsrecentfilesactionobjects
        kritawidgetutilsrecentfilesmanagerobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::WidgetsAddons
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ConfigCore
)
target_compile_definitions(KRecentFilesActionContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisOptionCollectionWidgetContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsoptioncollectionobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisOptionCollectionWidgetContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisOptionCollectionWidgetContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisWrappableHBoxLayoutContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilswrappablehboxlayoutobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisWrappableHBoxLayoutContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisWrappableHBoxLayoutContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisMenuStyleDontCloseOnAltContractTest.cpp
    LINK_LIBRARIES
        kritamenustyledontcloseonaltobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisMenuStyleDontCloseOnAltContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisMenuStyleDontCloseOnAltContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisPopupSelfActivatingLineEditContractTest.cpp
    LINK_LIBRARIES
        kritapopupselfactivatinglineeditobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisPopupSelfActivatingLineEditContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisPopupSelfActivatingLineEditContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisSpinBoxI18nHelperContractTest.cpp
    LINK_LIBRARIES
        kritaspinboxi18nhelperobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisSpinBoxI18nHelperContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisSpinBoxI18nHelperContractTest PRIVATE kritawidgetutils_EXPORTS)

ki18n_wrap_ui(DialogStateSaver_SRCS
    dialogsavertestwidget.ui
)

kis_add_test(
    KisDialogStateSaverTest.cpp
    ${DialogStateSaver_SRCS}
    TEST_NAME KisDialogStateSaverTest
    LINK_LIBRARIES kritawidgetutils kritatestsdk KF${KF_MAJOR}::ConfigCore
    )
