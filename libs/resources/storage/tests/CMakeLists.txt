########### manual test for file contents ###############

kis_add_tests(
    TestResourceStorageArchiveContract.cpp
    LINK_LIBRARIES kritaresourcestorage Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Xml
)

if(ANDROID)
    add_executable(storedroptest EXCLUDE_FROM_ALL storedroptest.cpp)
else()
    add_executable(storedroptest storedroptest.cpp)
endif()
target_link_libraries(storedroptest kritaresourcestorage Qt${QT_MAJOR_VERSION}::Widgets)
ecm_mark_as_test(storedroptest)

macos_test_fixrpath( storedroptest )

kis_add_test(
    StoreDebugCompatibilityTest.cpp
    LINK_LIBRARIES
        kritaresourcestoragelogobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(StoreDebugCompatibilityTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(StoreDebugCompatibilityTest PRIVATE kritaresourcestorage_EXPORTS)
