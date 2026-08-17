########### manual test for file contents ###############

kis_add_tests(
    TestResourceStorageArchiveContract.cpp
    LINK_LIBRARIES kritaresourcestorage Qt${QT_MAJOR_VERSION}::Test
)

add_executable(storedroptest storedroptest.cpp)
target_link_libraries(storedroptest kritaresourcestorage Qt${QT_MAJOR_VERSION}::Widgets)
ecm_mark_as_test(storedroptest)

macos_test_fixrpath( storedroptest )
