add_library(kritaimagegpuoperations OBJECT
    KisGpuOperationDocument.cpp
    KisGpuOperationRecovery.cpp
)
target_compile_definitions(kritaimagegpuoperations PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagegpuoperations PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritaimagegpuoperations PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagegpuoperations PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_subdirectory(tests)
