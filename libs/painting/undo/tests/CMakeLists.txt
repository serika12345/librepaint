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

kis_add_test(
    KisCommandUtilsContractTest.cpp
    LINK_LIBRARIES
        kritapaintingundocommandutilsobjects
        kritapaintingundokundo2coreobjects
        kritapaintingundostoreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisCommandUtilsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisCommandUtilsContractTest PRIVATE kritapaintingundo_EXPORTS)

kis_add_test(
    KUndo2GroupContractTest.cpp
    LINK_LIBRARIES
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KUndo2GroupContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KUndo2GroupContractTest PRIVATE kritapaintingundo_EXPORTS)

kis_add_test(
    KUndo2StackContractTest.cpp
    LINK_LIBRARIES
        kritapaintingundokundo2coreobjects
        kritapaintingundostackactionobjects
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ConfigGui
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KUndo2StackContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/widgetutils/config
)
target_compile_definitions(KUndo2StackContractTest PRIVATE kritapaintingundo_EXPORTS)
