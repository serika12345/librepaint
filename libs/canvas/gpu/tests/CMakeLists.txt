kis_add_test(KisGpuTilePresentationTest.cpp
    TEST_NAME_VAR gpu_tile_presentation_test
    LINK_LIBRARIES kritacanvasgputilepresentation kritaimagegputiles kritaimagegpuoperations Qt6::GuiPrivate Qt6::Test
)
target_compile_definitions(KisGpuTilePresentationTest PRIVATE kritacanvas_EXPORTS kritaimage_EXPORTS)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_tests_properties(${gpu_tile_presentation_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
