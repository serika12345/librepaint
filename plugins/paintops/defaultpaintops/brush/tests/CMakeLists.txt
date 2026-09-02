# on macOS cannot link to a MH_LIBRARY, see bug 417391
kis_add_test(KisDabRenderingQueueTest.cpp
    TEST_NAME KisDabRenderingQueueTest
    LINK_LIBRARIES kritadefaultpaintops_static kritalibpaintop kritaimage kritatestsdk
    )

kis_add_test(
    KisDabRenderingJobSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisDabRenderingJobSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/../..
    ${CMAKE_SOURCE_DIR}/libs/brush
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/brushengine
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/plugins/paintops/libpaintop
    ${CMAKE_BINARY_DIR}/plugins/paintops/libpaintop
)
target_include_directories(KisDabRenderingJobSchemaContractTest SYSTEM PRIVATE
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::ImathConfig,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisDabRenderingJobSchemaContractTest PRIVATE
    kritadefaultpaintops_static_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)


krita_add_broken_unit_test(kis_brushop_test.cpp ../../../../../sdk/tests/stroke_testing_utils.cpp
    TEST_NAME KisBrushOpTest
    LINK_LIBRARIES kritaapplicationui kritalibpaintop kritatestsdk
    )
