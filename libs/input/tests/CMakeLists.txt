kis_add_tests(
    TestInputEventSuppressor.cpp
    TestInputProfile.cpp
    TestInputShortcutMatcher.cpp
    LINK_LIBRARIES kritainput Qt${QT_MAJOR_VERSION}::Gui Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisShortcutConfigurationContractTest.cpp
    LINK_LIBRARIES
        kritainputshortcutconfigurationobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisShortcutConfigurationContractTest PRIVATE kritainput_EXPORTS)
