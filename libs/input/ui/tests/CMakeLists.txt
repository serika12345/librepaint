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
    KisToolInvocationActionSchemaContractTest.cpp
    TEST_NAME KisToolInvocationActionSchemaContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisToolInvocationActionSchemaContractTest
    PRIVATE
        kritacanvas_EXPORTS
        kritainputui_EXPORTS
)
target_include_directories(KisToolInvocationActionSchemaContractTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}/..
        ${CMAKE_SOURCE_DIR}/libs/canvas
        ${CMAKE_BINARY_DIR}/libs/canvas
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

kis_add_test(
    KisInputProfileManagerSchemaContractTest.cpp
    TEST_NAME KisInputProfileManagerSchemaContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisInputProfileManagerSchemaContractTest
    PRIVATE kritainputui_EXPORTS
)
target_include_directories(KisInputProfileManagerSchemaContractTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}/..
        ${CMAKE_SOURCE_DIR}/libs/widgetutils
        ${CMAKE_BINARY_DIR}/libs/widgetutils
        $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)

kis_add_test(
    KisInputManagerSchemaContractTest.cpp
    TEST_NAME KisInputManagerSchemaContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisInputManagerSchemaContractTest
    PRIVATE
        kritacanvas_EXPORTS
        kritainputui_EXPORTS
)
target_include_directories(KisInputManagerSchemaContractTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}/..
        ${CMAKE_SOURCE_DIR}/libs/canvas
        ${CMAKE_BINARY_DIR}/libs/canvas
)

kis_add_test(
    KisToolProxySchemaContractTest.cpp
    TEST_NAME KisToolProxySchemaContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisToolProxySchemaContractTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}/..
        ${CMAKE_SOURCE_DIR}/libs/canvas
        ${CMAKE_BINARY_DIR}/libs/canvas
        ${CMAKE_SOURCE_DIR}/libs/tools
        ${CMAKE_BINARY_DIR}/libs/tools
        ${CMAKE_SOURCE_DIR}/libs/flake
        ${CMAKE_BINARY_DIR}/libs/flake
        ${CMAKE_SOURCE_DIR}/libs/flake/resources
        ${CMAKE_BINARY_DIR}/libs/flake/resources
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_SOURCE_DIR}/libs/pigment
        ${CMAKE_BINARY_DIR}/libs/pigment
        ${CMAKE_SOURCE_DIR}/libs/pigment/compositeops
        ${CMAKE_BINARY_DIR}/libs/pigment/compositeops
        ${CMAKE_SOURCE_DIR}/libs/pigment/resources
        ${CMAKE_BINARY_DIR}/libs/pigment/resources
        ${CMAKE_SOURCE_DIR}/libs/resources
        ${CMAKE_BINARY_DIR}/libs/resources
        $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisToolProxySchemaContractTest
    PRIVATE
        kritaflake_EXPORTS
        kritaglobal_EXPORTS
        kritaimage_EXPORTS
        kritainputui_EXPORTS
        kritapigment_EXPORTS
        kritaresources_EXPORTS
        kritatools_EXPORTS
)
