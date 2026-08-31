########### next target ###############

kis_add_tests(
    kis_auto_brush_test.cpp
    kis_auto_brush_factory_test.cpp
    kis_gbr_brush_test.cpp
    kis_png_brush_test.cpp
    kis_boundary_test.cpp
    kis_imagepipe_brush_test.cpp
    TestAbrStorage.cpp
    KisBrushModelTest.cpp
    LINK_LIBRARIES kritaimage kritalibbrush kritatestsdk
    )

kis_add_test(
    KisDabShapeContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDabShapeContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KisBrushModelValuesContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
        lager
)
target_include_directories(KisBrushModelValuesContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/brushengine
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisBrushModelValuesContractTest PRIVATE
    kritalibbrush_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisBrushFactoryContractTest.cpp
    LINK_LIBRARIES
        kritaresourceloadvalueobjects
        kritaembeddedresourcevalueobjects
        kritaresourcestypesobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
        lager
)
target_include_directories(KisBrushFactoryContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/brushengine
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
)
target_compile_definitions(KisBrushFactoryContractTest PRIVATE
    kritalibbrush_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisQImagePyramidContractTest.cpp
    LINK_LIBRARIES
        kritabrushqimagepyramidobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisQImagePyramidContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisQImagePyramidContractTest PRIVATE kritalibbrush_EXPORTS)

kis_add_test(
    KisPipeBrushParasiteContractTest.cpp
    LINK_LIBRARIES
        kritabrushpipeparasiteobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::I18n
        Imath::Imath
        Boost::boost
)
target_include_directories(KisPipeBrushParasiteContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/brushengine
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/psdutils
    ${CMAKE_BINARY_DIR}/libs/psdutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
)
target_compile_definitions(KisPipeBrushParasiteContractTest PRIVATE kritalibbrush_EXPORTS)
