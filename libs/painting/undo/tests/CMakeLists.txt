kis_add_tests(
    TestKUndo2Stack.cpp
    LINK_LIBRARIES kritapaintingundo kritatestsdk
)

kis_add_test(
    KisCommandIdsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisCommandIdsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
