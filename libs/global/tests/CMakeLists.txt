kis_add_tests(KisSignalAutoConnectionTest.cpp
    KisSignalCompressorTest.cpp
    KisForestTest.cpp
    KisMplTest.cpp
    KisRectsGridTest.cpp
    KisLazyStorageTest.cpp
    KisValueCacheTest.cpp
    KisHistoryListTest.cpp
    KisAdaptedLockTest.cpp
    KisConfigurationValueTypesTest.cpp
    LINK_LIBRARIES kritaglobal kritatestsdk
    )

kis_add_test(
    KisGlobalTest.cpp
    LINK_LIBRARIES kritaglobalfileutilsobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisGlobalTest PRIVATE kritaglobal_EXPORTS)
