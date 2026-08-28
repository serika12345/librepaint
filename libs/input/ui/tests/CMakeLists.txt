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

kis_add_test(
    KisExtendedModifiersMapperPluginInterfaceContractTest.cpp
    TEST_NAME KisExtendedModifiersMapperPluginInterfaceContractTest
    LINK_LIBRARIES
        kritainputuiextendedmodifiersinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisExtendedModifiersMapperPluginInterfaceContractTest
    PRIVATE kritainputui_EXPORTS
)

kis_add_test(
    KisPopupWidgetInterfaceContractTest.cpp
    TEST_NAME KisPopupWidgetInterfaceContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisPopupWidgetInterfaceContractTest
    PRIVATE kritainputui_EXPORTS
)
target_include_directories(KisPopupWidgetInterfaceContractTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}/..
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
