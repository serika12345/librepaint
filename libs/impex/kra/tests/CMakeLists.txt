kis_add_test(TestKraRasterCodec.cpp
    LINK_LIBRARIES kritaimpexkra kritaimagegpuoperations Qt6::Test Qt6::Gui Qt6::Xml ${QUAZIP_LIBRARIES}
)
target_compile_definitions(TestKraRasterCodec PRIVATE
    KRA_COMPATIBILITY_DATA_DIR="${CMAKE_SOURCE_DIR}/plugins/impex/libkra/tests/data")
