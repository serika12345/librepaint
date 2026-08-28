kis_add_tests(
    TestInputProfile.cpp
    TestInputShortcutMatcher.cpp
    LINK_LIBRARIES kritainput Qt${QT_MAJOR_VERSION}::Gui Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    TestInputEventSuppressor.cpp
    LINK_LIBRARIES
        kritainputeventsuppressorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(TestInputEventSuppressor PRIVATE kritainput_EXPORTS)

kis_add_test(
    KisShortcutConfigurationContractTest.cpp
    LINK_LIBRARIES
        kritainputshortcutconfigurationobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisShortcutConfigurationContractTest PRIVATE kritainput_EXPORTS)

kis_add_test(
    KisInputProfileContractTest.cpp
    LINK_LIBRARIES
        kritainputprofileobjects
        kritainputshortcutconfigurationobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisInputProfileContractTest PRIVATE kritainput_EXPORTS)

kis_add_test(
    KisAbstractShortcutContractTest.cpp
    LINK_LIBRARIES
        kritainputabstractshortcutobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisAbstractShortcutContractTest PRIVATE
    kritacanvas_EXPORTS
    kritainput_EXPORTS
)

kis_add_test(
    KisTimedSignalThresholdContractTest.cpp
    LINK_LIBRARIES
        kritainputtimedsignalthresholdobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisTimedSignalThresholdContractTest PRIVATE kritainput_EXPORTS)
