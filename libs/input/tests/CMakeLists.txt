kis_add_tests(
    TestInputEventSuppressor.cpp
    TestInputProfile.cpp
    TestInputShortcutMatcher.cpp
    LINK_LIBRARIES kritainput Qt${QT_MAJOR_VERSION}::Gui Qt${QT_MAJOR_VERSION}::Test
)
