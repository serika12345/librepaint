kis_add_test(
    kis_input_manager_test.cpp
    TEST_NAME KisInputManagerTest
    LINK_LIBRARIES kritainputui kritatestsdk
)

kis_add_test(
    KisToolProxyContractTest.cpp
    TEST_NAME KisToolProxyContractTest
    LINK_LIBRARIES kritainputui kritatestsdk KF${KF_MAJOR}::ConfigCore
)

target_include_directories(KisToolProxyContractTest PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})

set(tablet_test_dialog_sources
    ../wintab/drawpile_tablettester/tablettester.cpp
    ../wintab/drawpile_tablettester/tablettest.cpp
    ../kis_input_config.cpp
    ${CMAKE_SOURCE_DIR}/libs/flake/KoPointerEvent.cpp
    ${CMAKE_SOURCE_DIR}/libs/widgetutils/config/khelpclient.cpp
)
ki18n_wrap_ui(tablet_test_dialog_sources ../wintab/drawpile_tablettester/tablettest.ui)
kis_add_test(
    TabletTestDialogTest.cpp
    ${tablet_test_dialog_sources}
    TEST_NAME TabletTestDialogTest
    LINK_LIBRARIES
        kritawidgetsdialogobjects
        kritaglobalconfignotifierobjects
        kritaglobalsignalcompressorobjects
        kritaglobaldebugobjects
        kritatoolsspeedsmootherobjects
        kritaglobalfilteredrollingmeanobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(TabletTestDialogTest PRIVATE
    kritainputui_EXPORTS
    kritaflake_EXPORTS
    kritawidgetutils_EXPORTS
    kritawidgets_EXPORTS
    kritaglobal_EXPORTS
    kritatools_EXPORTS
)
target_include_directories(TabletTestDialogTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
)

kis_add_test(
    KisToolInvocationActionCompatibilityTest.cpp
    TEST_NAME KisToolInvocationActionCompatibilityTest
    LINK_LIBRARIES
        kritainputui
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisExtendedModifiersMapperContractTest.cpp
    TEST_NAME KisExtendedModifiersMapperContractTest
    LINK_LIBRARIES
        kritainputuiextendedmodifiersinterfaceobjects
        kritainputuiextendedmodifiersmapperobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisExtendedModifiersMapperContractTest
    PRIVATE kritainputui_EXPORTS
)
