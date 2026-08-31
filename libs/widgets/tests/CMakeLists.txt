#add_subdirectory(filedialogtester)

kis_add_test(
    KoSliderComboContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsslidercomboobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KoSliderComboContractTest PRIVATE kritawidgets_EXPORTS)

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
    KoPageWidgetItemContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KoPageWidgetItemContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoPageWidgetItemContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoDialogEnumContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsdialogobjects
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
    KoAspectButtonContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsaspectbuttonobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KoAspectButtonContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoAnchorSelectionWidgetTest.cpp
    LINK_LIBRARIES
        kritawidgetsanchorselectionwidgetobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KoAnchorSelectionWidgetTest PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
    kritawidgets_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    KisPaletteDelegateContractTest.cpp
    LINK_LIBRARIES
        kritawidgetspalettedelegateobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisPaletteDelegateContractTest PRIVATE kritawidgets_EXPORTS)

kis_add_test(
    KoLineStyleSelectorContractTest.cpp
    LINK_LIBRARIES
        kritawidgetslinestyleselectorobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KoLineStyleSelectorContractTest PRIVATE kritawidgets_EXPORTS)

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
    KoDockWidgetTitleBarContractTest.cpp
    LINK_LIBRARIES
        kritawidgetsdocktitlebarobjects
        kritawidgetsdocktitlebarbuttonobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::WidgetsAddons
)
target_include_directories(KoDockWidgetTitleBarContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_compile_definitions(KoDockWidgetTitleBarContractTest PRIVATE kritawidgets_EXPORTS)

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
    TestKoZoomAction.cpp
    LINK_LIBRARIES
        kritawidgetszoomactionstateobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
        KF${KF_MAJOR}::I18n
)
target_compile_definitions(TestKoZoomAction PRIVATE
    kritaflake_EXPORTS
    kritawidgets_EXPORTS
)

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
    TestKisTagSelectionWidget.cpp
    TestKisPaletteModel.cpp
    LINK_LIBRARIES kritawidgets kritatestsdk
    )
