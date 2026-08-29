add_executable(CCSGraph CCSGraph.cpp)
target_link_libraries(CCSGraph kritapigment KF${KF_MAJOR}::I18n)
ecm_mark_as_test(CCSGraph)

kis_add_test(
    KoColorModelStandardIdsContractTest.cpp
    LINK_LIBRARIES
        kritapigmentstandardidsobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Test
        ${LINK_OPENEXR_LIB}
)
target_compile_definitions(KoColorModelStandardIdsContractTest PRIVATE
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    DebugPigmentContractTest.cpp
    LINK_LIBRARIES
        kritapigmentdebugobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(DebugPigmentContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(DebugPigmentContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoAlphaMaskApplicatorBaseContractTest.cpp
    LINK_LIBRARIES
        kritapigmentalphamaskapplicatorbaseobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoAlphaMaskApplicatorBaseContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoAlphaColorSpaceTypeContractTest.cpp
    LINK_LIBRARIES
        kritapigmentstandardidsobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        ${LINK_OPENEXR_LIB}
)
target_compile_definitions(KoAlphaColorSpaceTypeContractTest PRIVATE
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KoAlphaColorSpaceContractTest.cpp
    LINK_LIBRARIES
        kritapigment
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KoColorimetryUtilsContractTest.cpp
    LINK_LIBRARIES
        kritapigmentcolorimetryobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoColorimetryUtilsContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoColorProfileConstantsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoColorProfileConstantsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KoColorSpaceConstantsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoColorSpaceConstantsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KoCompositeOpIdsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoCompositeOpIdsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KoColorSpacePreserveLightnessUtilsContractTest.cpp
    LINK_LIBRARIES
        kritapigmentmathsobjects
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KoCmykColorSpaceMathsContractTest.cpp
    LINK_LIBRARIES
        kritapigmentcmykmathsobjects
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KoCmykColorSpaceTraitsContractTest.cpp
    LINK_LIBRARIES
        kritapigmentmathsobjects
        kritapigmentcmykmathsobjects
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KoLabColorSpaceMathsContractTest.cpp
    LINK_LIBRARIES
        kritapigmentlabmathsobjects
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KoGrayColorSpaceTraitsContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KoGrayColorSpaceTraitsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KoBgrColorSpaceTraitsContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KoBgrColorSpaceTraitsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    TestKoIntegerMaths.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestKoIntegerMaths PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_tests(
    TestKoColorSet.cpp
    TestKoColorSpaceRegistry.cpp
    TestKoColorSpaceAbstract.cpp
    TestColorConversionSystem.cpp
    TestKoColor.cpp
    TestConvolutionOpImpl.cpp
    KoRgbU8ColorSpaceTester.cpp
    TestKoColorSpaceSanity.cpp
    TestFallBackColorTransformation.cpp
    TestKoChannelInfo.cpp

    LINK_LIBRARIES kritapigment KF${KF_MAJOR}::I18n kritatestsdk
    )

kis_add_test(
    TestCompositeOpInversion.cpp TestCompositeOpInversion_p.cpp
    TEST_NAME TestCompositeOpInversion
    LINK_LIBRARIES kritapigment KF${KF_MAJOR}::I18n kritatestsdk
    )

kis_add_tests(
    TestColorConversion.cpp
    TestKoColorSpaceMaths.cpp
    TestKisSwatchGroup.cpp
    TestKoStopGradient.cpp

    LINK_LIBRARIES kritapigment kritatestsdk
    )


macos_test_fixrpath(CCSGraph)
