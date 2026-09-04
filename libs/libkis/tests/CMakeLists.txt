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
    ${CMAKE_SOURCE_DIR}/libs/widgets
    ${CMAKE_BINARY_DIR}/libs/widgets
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
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
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(PaletteSchemaContractTest PRIVATE
    kritaglobal_EXPORTS kritaimage_EXPORTS kritalibkis_EXPORTS kritapigment_EXPORTS kritaresources_EXPORTS
    kritawidgets_EXPORTS kritawidgetutils_EXPORTS
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
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(SelectionSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibkis_EXPORTS
    kritaflake_EXPORTS
)

kis_add_test(
    ViewSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(ViewSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(ViewSchemaContractTest PRIVATE kritalibkis_EXPORTS)

kis_add_test(
    ResourcePresetSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(ResourcePresetSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/resources/ui
    ${CMAKE_BINARY_DIR}/libs/resources/ui
    ${CMAKE_SOURCE_DIR}/libs/ui/widgets
    ${CMAKE_BINARY_DIR}/libs/application
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(ResourcePresetSchemaContractTest PRIVATE
    kritalibkis_EXPORTS
    kritaimage_EXPORTS
    kritaglobal_EXPORTS
    kritaresources_EXPORTS
    kritaui_EXPORTS
)

kis_add_test(
    CanvasSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(CanvasSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(CanvasSchemaContractTest PRIVATE kritalibkis_EXPORTS)

kis_add_test(
    FileDialogSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(FileDialogSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(FileDialogSchemaContractTest PRIVATE kritalibkis_EXPORTS)

kis_add_test(
    ColorizeMaskSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test Boost::boost
)
target_include_directories(ColorizeMaskSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_BINARY_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting
    ${CMAKE_BINARY_DIR}/libs/painting
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(ColorizeMaskSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibkis_EXPORTS
)

kis_add_test(
    NodeSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test Boost::boost
)
target_include_directories(NodeSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_BINARY_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting
    ${CMAKE_BINARY_DIR}/libs/painting
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(NodeSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibkis_EXPORTS
)

kis_add_test(
    DocumentGeometrySchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test Boost::boost
)
target_include_directories(DocumentGeometrySchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_BINARY_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting
    ${CMAKE_BINARY_DIR}/libs/painting
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/ui/canvas
    ${CMAKE_BINARY_DIR}/libs/application
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(DocumentGeometrySchemaContractTest PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibkis_EXPORTS
    kritaui_EXPORTS
)

kis_add_test(
    DocumentLifecycleSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test Boost::boost
)
target_include_directories(DocumentLifecycleSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_BINARY_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting
    ${CMAKE_BINARY_DIR}/libs/painting
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/ui/canvas
    ${CMAKE_BINARY_DIR}/libs/application
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(DocumentLifecycleSchemaContractTest PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibkis_EXPORTS
    kritaui_EXPORTS
)

kis_add_test(
    DocumentNodeSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test Boost::boost
)
target_include_directories(DocumentNodeSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_BINARY_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting
    ${CMAKE_BINARY_DIR}/libs/painting
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/ui/canvas
    ${CMAKE_BINARY_DIR}/libs/application
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(DocumentNodeSchemaContractTest PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibkis_EXPORTS
    kritaui_EXPORTS
)
