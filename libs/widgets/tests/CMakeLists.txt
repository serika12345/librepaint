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

kis_add_tests(
    zoomhandler_test.cpp
    KoAnchorSelectionWidgetTest.cpp
    TestKoZoomAction.cpp
    TestKisTagSelectionWidget.cpp
    TestKisPaletteModel.cpp
    LINK_LIBRARIES kritawidgets kritatestsdk
    )
