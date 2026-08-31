########### manual test for file contents ###############

kis_add_tests(
    TestResourceStorageArchiveContract.cpp
    LINK_LIBRARIES kritaresourcestorage Qt${QT_MAJOR_VERSION}::Test
)

add_executable(storedroptest storedroptest.cpp)
target_link_libraries(storedroptest kritaresourcestorage Qt${QT_MAJOR_VERSION}::Widgets)
ecm_mark_as_test(storedroptest)

macos_test_fixrpath( storedroptest )

kis_add_test(
    StoreDebugContractTest.cpp
    LINK_LIBRARIES
        kritaresourcestoragelogobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(StoreDebugContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(StoreDebugContractTest PRIVATE kritaresourcestorage_EXPORTS)

kis_add_test(
    KoStoreDeviceContractTest.cpp
    LINK_LIBRARIES
        kritaresourcestoragedeviceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KoStoreDeviceContractTest PRIVATE ../KoStoreDevice.h)
target_include_directories(KoStoreDeviceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoStoreDeviceContractTest PRIVATE kritaresourcestorage_EXPORTS)
