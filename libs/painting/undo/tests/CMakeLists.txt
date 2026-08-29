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

kis_add_test(
    KisUndoStoresContractTest.cpp
    LINK_LIBRARIES
        kritapaintingundokundo2coreobjects
        kritapaintingundostoreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisUndoStoresContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisUndoStoresContractTest PRIVATE kritapaintingundo_EXPORTS)
