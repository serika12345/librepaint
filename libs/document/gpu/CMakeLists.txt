add_library(kritadocumentgpupaint OBJECT
    KisGpuPaintDocument.cpp
    KisGpuPaintDocument.h
    ../files/gpu/KisGpuKraDocumentFile.cpp
)
target_compile_definitions(kritadocumentgpupaint PRIVATE kritadocumentpainting_EXPORTS)
target_include_directories(kritadocumentgpupaint PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritadocumentgpupaint PUBLIC kritagpuimage Qt6::Core
    PRIVATE kritaimpexkra Qt6::Gui)
set_target_properties(kritadocumentgpupaint PROPERTIES POSITION_INDEPENDENT_CODE ON)
kis_add_library(kritadocumentpainting SHARED $<TARGET_OBJECTS:kritadocumentgpupaint>)
generate_export_header(kritadocumentpainting BASE_NAME kritadocumentpainting
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/../kritadocumentpainting_export.h)
target_include_directories(kritadocumentpainting PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>)
target_link_libraries(kritadocumentpainting
    PUBLIC kritagpuimage kritaimpexkra Qt6::Core)
set_target_properties(kritadocumentpainting PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION})
install(TARGETS kritadocumentpainting ${INSTALL_TARGETS_DEFAULT_ARGS})
add_subdirectory(tests)
