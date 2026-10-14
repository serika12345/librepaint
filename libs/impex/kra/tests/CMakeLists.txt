kis_add_test(TestKraRasterCodec.cpp
    LINK_LIBRARIES kritaimpexkra kritaimagegpuoperations Qt6::Test Qt6::Gui Qt6::Xml ${QUAZIP_LIBRARIES}
)
target_compile_definitions(TestKraRasterCodec PRIVATE
    KRA_COMPATIBILITY_DATA_DIR=":/kra-compatibility")
qt6_add_resources(TestKraRasterCodec kra_compatibility_fixtures
    PREFIX "/kra-compatibility"
    BASE "${CMAKE_SOURCE_DIR}/plugins/impex/libkra/tests/data"
    FILES
        "${CMAKE_SOURCE_DIR}/plugins/impex/libkra/tests/data/single_layer_no_channel_flags_transp_def_pixel.kra"
        "${CMAKE_SOURCE_DIR}/plugins/impex/libkra/tests/data/single_layer_no_channel_flags_nontransp_def_pixel.kra"
)
