kis_add_tests(
    #TestKrita.cpp
    TestChannel.cpp
    TestDocument.cpp
    TestNode.cpp
    TestVectorLayer.cpp
    TestShape.cpp
    TestFilter.cpp
    TestManagedColor.cpp
    TestNotifier.cpp
    LINK_LIBRARIES kritalibkis kritatestsdk
    )

kis_add_test(
    GridConfigSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(GridConfigSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/ui/canvas
    ${CMAKE_BINARY_DIR}/libs/application
)
target_compile_definitions(GridConfigSchemaContractTest PRIVATE
    kritalibkis_EXPORTS
    kritaui_EXPORTS
)

kis_add_test(
    PaletteSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Gui Qt${QT_MAJOR_VERSION}::Test Boost::boost
)
target_include_directories(PaletteSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(PaletteSchemaContractTest PRIVATE
    kritaglobal_EXPORTS kritaimage_EXPORTS kritalibkis_EXPORTS kritapigment_EXPORTS kritaresources_EXPORTS
)

kis_add_test(
    AngleSelectorSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(AngleSelectorSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgets
    ${CMAKE_BINARY_DIR}/libs/widgets
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_compile_definitions(AngleSelectorSchemaContractTest PRIVATE
    kritalibkis_EXPORTS
    kritawidgets_EXPORTS
)

kis_add_test(
    SliderSpinBoxSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(SliderSpinBoxSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_compile_definitions(SliderSpinBoxSchemaContractTest PRIVATE
    kritalibkis_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    SelectionSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(SelectionSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(SelectionSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibkis_EXPORTS
)
