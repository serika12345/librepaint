kis_add_tests(
    kis_coordinates_converter_test.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_test(
    KisInputActionGroupContractTest.cpp
    LINK_LIBRARIES
        kritacanvasinputactiongroupobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisInputActionGroupContractTest PRIVATE kritacanvas_EXPORTS)

kis_add_test(
    KisProjectionPixelFilterContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisProjectionPixelFilterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisProjectionPixelFilterContractTest PRIVATE kritacanvas_EXPORTS)

kis_add_test(
    KisCanvasUpdateInfoSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisCanvasUpdateInfoSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisCanvasUpdateInfoSchemaContractTest PRIVATE
    kritacanvas_EXPORTS
    kritaglobal_EXPORTS
)

kis_add_test(
    KisFrameDataSerializerSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisFrameDataSerializerSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisFrameDataSerializerSchemaContractTest PRIVATE kritacanvas_EXPORTS)

kis_add_test(
    KisFrameCacheStoreSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisFrameCacheStoreSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisFrameCacheStoreSchemaContractTest PRIVATE kritacanvas_EXPORTS)

kis_add_test(
    KisCoordinatesConverterSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisCoordinatesConverterSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgets
    ${CMAKE_BINARY_DIR}/libs/widgets
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisCoordinatesConverterSchemaContractTest PRIVATE
    kritacanvas_EXPORTS
    kritaglobal_EXPORTS
)

kis_add_test(
    KisToolCanvasSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisToolCanvasSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisToolCanvasSchemaContractTest PRIVATE
    kritacanvas_EXPORTS
    kritaimage_EXPORTS
    kritaglobal_EXPORTS
)

kis_add_test(
    KisDisplayColorFilterContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDisplayColorFilterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisDisplayColorFilterContractTest PRIVATE kritacanvas_EXPORTS)

kis_add_test(
    KisDisplayColorTransformSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDisplayColorTransformSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(KisDisplayColorTransformSchemaContractTest PRIVATE
    kritacanvas_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisOcioConfigurationContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisOcioConfigurationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisOcioConfigurationContractTest PRIVATE kritacanvas_EXPORTS)

kis_add_test(
    KisImagePatchContractTest.cpp
    LINK_LIBRARIES
        kritacanvasimagepatchobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisImagePatchContractTest PRIVATE
    kritacanvas_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisTileDataPoolContractTest.cpp
    LINK_LIBRARIES
        kritacanvastiledatapoolobjects
        kritaglobalsignalcompressorobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisTileDataPoolContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisTileDataPoolContractTest PRIVATE
    kritacanvas_EXPORTS
    kritaglobal_EXPORTS
)

kis_add_tests(
    kis_prescaled_projection_contract_test.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_tests(
    kis_display_color_transform_test.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_test(
    KisSurfaceColorSpaceWrapperTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KisSurfaceColorSpaceWrapperTest PRIVATE KisSurfaceColorSpaceWrapperTest.h)
target_include_directories(KisSurfaceColorSpaceWrapperTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisSurfaceColorSpaceWrapperTest PRIVATE kritacanvas_EXPORTS)

kis_add_tests(
    kis_frame_cache_store_test.cpp
    KisFrameSerializerTest.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_test(
    kis_animation_frame_cache_index_test.cpp
    LINK_LIBRARIES
        kritacanvasanimationframecacheindexobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(kis_animation_frame_cache_index_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
)
target_compile_definitions(kis_animation_frame_cache_index_test PRIVATE
    kritacanvas_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)
