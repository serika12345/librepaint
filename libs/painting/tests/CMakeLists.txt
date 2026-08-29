kis_add_tests(
    TestPaintingBoundary.cpp
    TestPublicImageHeaders.cpp
    LINK_LIBRARIES kritapainting kritatestsdk
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
