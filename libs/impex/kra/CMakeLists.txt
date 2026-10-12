add_library(kritaimpexkra OBJECT
    KisKraRasterCodec.cpp
    KisKraRgbaTiles.cpp
)
target_compile_definitions(kritaimpexkra PRIVATE kritaimpex_EXPORTS)
target_include_directories(kritaimpexkra PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritaimpexkra
    PUBLIC kritaimagegpuoperations
    PRIVATE kritaimagecompressionobjects Qt6::Gui Qt6::Xml ${QUAZIP_LIBRARIES}
)
set_target_properties(kritaimpexkra PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
add_subdirectory(tests)
