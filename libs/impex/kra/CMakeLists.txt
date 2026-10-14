kis_add_library(kritaimpexkra SHARED
    KisKraRasterCodec.cpp
    KisKraRgbaTiles.cpp
)
generate_export_header(kritaimpexkra BASE_NAME kritaimpexkra
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/../kritaimpexkra_export.h)
target_include_directories(kritaimpexkra PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritaimpexkra
    PUBLIC kritaimagegpuoperations
    PRIVATE kritalzfcompression Qt6::Gui Qt6::Xml ${QUAZIP_LIBRARIES}
)
set_target_properties(kritaimpexkra PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION})
install(TARGETS kritaimpexkra ${INSTALL_TARGETS_DEFAULT_ARGS})
add_subdirectory(tests)
