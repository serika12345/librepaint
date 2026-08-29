kis_add_tests(
    kis_tiled_data_manager_test.cpp
    kis_low_memory_tests.cpp
    kis_lockless_stack_test.cpp
    kis_chunk_allocator_test.cpp
    kis_memory_window_test.cpp
    kis_store_limits_test.cpp
    kis_swapped_data_store_test.cpp
    kis_tile_data_store_test.cpp
    kis_tile_data_pooler_test.cpp
    LINK_LIBRARIES kritaimage kritatestsdk
    )

set_tests_properties(libs-image-tiles3-kis_low_memory_tests PROPERTIES TIMEOUT 180)

kis_add_test(
    ImageLzfCompressionContractTest.cpp
    LINK_LIBRARIES
        kritaimagecompressionobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(ImageLzfCompressionContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
)
target_compile_definitions(ImageLzfCompressionContractTest PRIVATE kritaimage_EXPORTS)
