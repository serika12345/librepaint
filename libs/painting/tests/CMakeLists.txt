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
