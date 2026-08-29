kis_add_tests(
    KisActionsSnapshotTest.cpp
    KoPropertiesTest.cpp
    TestKoProgressUpdater.cpp
    TestSqueezedComboBox.cpp
    kis_parse_spin_boxes_test.cpp
    LINK_LIBRARIES kritawidgetutils kritaimage kritatestsdk
)

kis_add_test(
    kis_simple_math_parser_test.cpp
    LINK_LIBRARIES kritawidgetutilsnumericparserobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_simple_math_parser_test PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KStandardActionEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigGui
)
target_include_directories(KStandardActionEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KStandardActionEnumContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KColorSchemeEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::ConfigCore
)
target_include_directories(KColorSchemeEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KColorSchemeEnumContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisKineticScrollerContractTest.cpp
    LINK_LIBRARIES
        kritakineticscrollerobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisKineticScrollerContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisPopupButtonContractTest.cpp
    LINK_LIBRARIES
        kritapopupbuttonobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisPopupButtonContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisPopupButtonContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisHighlightedToolButtonContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisHighlightedToolButtonContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisHighlightedToolButtonContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisMenuStyleDontCloseOnAltContractTest.cpp
    LINK_LIBRARIES
        kritamenustyledontcloseonaltobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisMenuStyleDontCloseOnAltContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisMenuStyleDontCloseOnAltContractTest PRIVATE kritawidgetutils_EXPORTS)

kis_add_test(
    KisPopupSelfActivatingLineEditContractTest.cpp
    LINK_LIBRARIES
        kritapopupselfactivatinglineeditobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisPopupSelfActivatingLineEditContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisPopupSelfActivatingLineEditContractTest PRIVATE kritawidgetutils_EXPORTS)

ki18n_wrap_ui(DialogStateSaver_SRCS
    dialogsavertestwidget.ui
)

kis_add_test(
    KisDialogStateSaverTest.cpp
    ${DialogStateSaver_SRCS}
    TEST_NAME KisDialogStateSaverTest
    LINK_LIBRARIES kritawidgetutils kritatestsdk KF${KF_MAJOR}::ConfigCore
    )
