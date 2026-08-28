kis_add_tests(KisSignalCompressorTest.cpp
    KisForestTest.cpp
    KisRectsGridTest.cpp
    LINK_LIBRARIES kritaglobal kritatestsdk
    )

kis_add_test(
    KisMplTest.cpp
    LINK_LIBRARIES
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisMplTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisHistoryListTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisHistoryListTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KritaContainerUtilsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KritaContainerUtilsContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisGlobalTest.cpp
    LINK_LIBRARIES kritaglobalfileutilsobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisGlobalTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisFutureUtilsTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisFutureUtilsTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisAdaptedLockTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisAdaptedLockTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisValueCacheTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisValueCacheTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisSignalAutoConnectionTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSignalAutoConnectionTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisLazyStorageTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisLazyStorageTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisCumulativeUndoDataTest.cpp
    LINK_LIBRARIES
        kritaglobalcumulativeundodataobjects
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigCore
)
target_compile_definitions(KisCumulativeUndoDataTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisTemporaryFileConfigurationTest.cpp
    LINK_LIBRARIES
        kritaglobaltemporaryfileconfigurationobjects
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigCore
)
target_compile_definitions(KisTemporaryFileConfigurationTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisNodeAdditionFlagsTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeAdditionFlagsTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisSignalsBlockerTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSignalsBlockerTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisPortingUtilsTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisPortingUtilsTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisQPainterStateSaverTest.cpp
    LINK_LIBRARIES
        kritaglobalqpainterstatesaverobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
)
target_compile_definitions(KisQPainterStateSaverTest PRIVATE kritaglobal_EXPORTS)

if(OpenEXR_FOUND)
    kis_add_test(
        KisHalfTraitsTest.cpp
        LINK_LIBRARIES
            Qt${QT_MAJOR_VERSION}::Test
            ${LINK_OPENEXR_LIB}
    )
    target_include_directories(KisHalfTraitsTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()

kis_add_test(
    KisCppQuirksTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisCppQuirksTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisDeleteLaterWrapperTest.cpp
    LINK_LIBRARIES
        kritaglobaldeletelaterwrapperobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisDeleteLaterWrapperTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisBackupTest.cpp
    LINK_LIBRARIES
        kritaglobalbackupobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Core
)
target_compile_definitions(KisBackupTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisCursorOverrideLockTest.cpp
    LINK_LIBRARIES
        kritaglobalcursoroverridelockobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
)
target_compile_definitions(KisCursorOverrideLockTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisFilteredRollingMeanTest.cpp
    LINK_LIBRARIES
        kritaglobalfilteredrollingmeanobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
)
target_compile_definitions(KisFilteredRollingMeanTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KoIDContractTest.cpp
    LINK_LIBRARIES
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoIDContractTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisSharedTest.cpp
    LINK_LIBRARIES
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Core
)
target_compile_definitions(KisSharedTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisRollingSumAccumulatorWrapperTest.cpp
    LINK_LIBRARIES
        kritaglobalrollingsumobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
)
target_compile_definitions(KisRollingSumAccumulatorWrapperTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisRollingMeanAccumulatorWrapperTest.cpp
    LINK_LIBRARIES
        kritaglobalrollingmeanobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
)
target_compile_definitions(KisRollingMeanAccumulatorWrapperTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisPinnedSharedPtrTest.cpp
    LINK_LIBRARIES
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Core
)
target_compile_definitions(KisPinnedSharedPtrTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisLazyValueWrapperTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisLazyValueWrapperTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisReadWriteLockPolicyTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisReadWriteLockPolicyTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisLocklessStackContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Threads::Threads
)
target_include_directories(KisLocklessStackContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisLagerContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        lager
)
target_include_directories(KisLagerContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisSampleRectIteratorTest.cpp
    LINK_LIBRARIES
        kritaglobalsamplerectiteratorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSampleRectIteratorTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisRectsGridContractTest.cpp
    LINK_LIBRARIES
        kritaglobalrectsgridobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisRectsGridContractTest PRIVATE kritaglobal_EXPORTS)

kis_add_test(
    KisQStringListFwdTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisQStringListFwdTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisZugContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        zug
)
target_include_directories(KisZugContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KoGenericRegistryContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoGenericRegistryContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisPointerUtilsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPointerUtilsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisBezierPatchParamSpaceUtilsContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
)
target_include_directories(KisBezierPatchParamSpaceUtilsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisLodTransformBaseContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
)
target_include_directories(KisLodTransformBaseContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
