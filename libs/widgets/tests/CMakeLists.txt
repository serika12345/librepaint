#add_subdirectory(filedialogtester)

kis_add_test(
    KisColorSelectorConfigurationContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisColorSelectorConfigurationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisColorSelectorConfigurationContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoDialogEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::WidgetsAddons
)
target_include_directories(KoDialogEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoDialogEnumContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoTitledTabWidgetContractTest.cpp
    LINK_LIBRARIES
        kritawidgetstitledtabobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KoTitledTabWidgetContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoColorPopupButtonContractTest.cpp
    LINK_LIBRARIES
        kritawidgetscolorpopupbuttonobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KoColorPopupButtonContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KisStackedWidgetContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsstackedwidgetobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisStackedWidgetContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoDockWidgetTitleBarButtonContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsdocktitlebarbuttonobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KoDockWidgetTitleBarButtonContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoVBoxContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsvboxobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KoVBoxContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KisWarningBlockContractTest.cpp
    LINK_LIBRARIES
        kritawidgetswarningblockobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisWarningBlockContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KisAngleGaugeContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsanglegaugeobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisAngleGaugeContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KisColorLabelButtonContractTest.cpp
    LINK_LIBRARIES
        kritawidgetscolorlabelbuttonobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisColorLabelButtonContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KisStretchedIconButtonContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsstretchediconbuttonobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisStretchedIconButtonContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KisColorLabelSelectorWidgetContractTest.cpp
    LINK_LIBRARIES
        kritawidgetscolorlabelselectorobjects
        kritawidgetscolorlabelbuttonobjects
        kritawidgetutilswrappablehboxlayoutobjects
        kritaglobalpaintingtweaksobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisColorLabelSelectorWidgetContractTest PRIVATE
    kritaglobal_EXPORTS
    kritawidgets_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    KisLevelsSliderContractTest.cpp
    LINK_LIBRARIES
        kritaglobalpaintingtweaksobjects
        kritawidgetslevelssliderobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisLevelsSliderContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    WidgetsDebugContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsdebugobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(WidgetsDebugContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoZoomHandlerContractTest.cpp
    LINK_LIBRARIES
        kritawidgetszoomhandlerobjects
        kritaflakeviewconverterobjects
        kritaflakeviewtransformstillpointobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoZoomHandlerContractTest PRIVATE
    kritaflake_EXPORTS
    kritawidgets_EXPORTS
)

kis_add_tests(
    zoomhandler_test.cpp
    KoAnchorSelectionWidgetTest.cpp
    TestKoZoomAction.cpp
    TestKisTagSelectionWidget.cpp
    TestKisPaletteModel.cpp
    LINK_LIBRARIES kritawidgets kritatestsdk
    )
