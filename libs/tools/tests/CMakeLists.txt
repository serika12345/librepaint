kis_add_tests(
    TestToolCoreContract.cpp
    LINK_LIBRARIES kritatools Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisSpeedSmootherContractTest.cpp
    LINK_LIBRARIES
        kritatoolsspeedsmootherobjects
        kritaglobalfilteredrollingmeanobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSpeedSmootherContractTest PRIVATE kritatools_EXPORTS)

kis_add_test(
    KisRectangleInteractionContractTest.cpp
    LINK_LIBRARIES
        kritatoolsrectangleinteractionobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisRectangleInteractionContractTest PRIVATE kritatools_EXPORTS)

kis_add_tests(
    kis_stabilized_events_sampler_test.cpp
    LINK_LIBRARIES kritatools kritatestsdk
)
