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
