kis_add_tests(
    TestToolCoreContract.cpp
    LINK_LIBRARIES kritatools Qt${QT_MAJOR_VERSION}::Test
)

kis_add_tests(
    kis_stabilized_events_sampler_test.cpp
    LINK_LIBRARIES kritatools kritatestsdk
)
