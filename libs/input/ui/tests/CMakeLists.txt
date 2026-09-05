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
