kis_add_tests(
    TestPaintingBoundary.cpp
    TestPublicImageHeaders.cpp
    LINK_LIBRARIES kritapainting kritatestsdk
)

kis_add_test(
    KisAsynchronousStrokeUpdateHelperContractTest.cpp
    LINK_LIBRARIES
        kritapaintingasyncstrokeupdatehelperobjects
        kritaimagejobstrategyobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KisAsynchronousStrokeUpdateHelperContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../KisAsynchronousStrokeUpdateHelper.h
)
target_compile_definitions(KisAsynchronousStrokeUpdateHelperContractTest PRIVATE
    kritaimage_EXPORTS
    kritapainting_EXPORTS
)

kis_add_test(
    KisFigurePaintingOptionsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisFigurePaintingOptionsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    FreehandStrokeSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(FreehandStrokeSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../strokes
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/brushengine
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(FreehandStrokeSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapainting_EXPORTS
    kritapaintingundo_EXPORTS
    kritapigment_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisFilterStrokeStrategySchemaContractTest.cpp
    TEST_NAME KisFilterStrokeStrategySchemaContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisFilterStrokeStrategySchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../strokes
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/image/brushengine
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisFilterStrokeStrategySchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapainting_EXPORTS
    kritapaintingundo_EXPORTS
    kritapigment_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisMaskingBrushCompositeOpBaseContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMaskingBrushCompositeOpBaseContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KUndo2CommandExtraDataContractTest.cpp
    ../undo/kundo2commandextradata.cpp
    TEST_NAME KUndo2CommandExtraDataContractTest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KUndo2CommandExtraDataContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KUndo2CommandExtraDataContractTest PRIVATE kritapaintingundo_EXPORTS)

kis_add_test(
    KUndo2MagicStringContractTest.cpp
    ../undo/kundo2magicstring.cpp
    TEST_NAME KUndo2MagicStringContractTest
    LINK_LIBRARIES
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KUndo2MagicStringContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KUndo2MagicStringContractTest PRIVATE kritapaintingundo_EXPORTS)
