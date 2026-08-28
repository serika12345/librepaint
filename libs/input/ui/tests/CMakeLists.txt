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
