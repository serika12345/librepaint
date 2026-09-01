add_executable(CCSGraph CCSGraph.cpp)
target_link_libraries(CCSGraph kritapigment KF${KF_MAJOR}::I18n)
ecm_mark_as_test(CCSGraph)

kis_add_test(
    KoMultipleColorConversionTransformationContractTest.cpp
    LINK_LIBRARIES
        kritapigmentmultiplecolorconversiontransformationobjects
        kritapigmentcolorconversiontransformationobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoMultipleColorConversionTransformationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KoMultipleColorConversionTransformationContractTest PRIVATE kritapigment_EXPORTS)

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
    KoOptimizedPixelDataScalerU8ToU16BaseContractTest.cpp
    LINK_LIBRARIES
        kritapigmentoptimizedpixelscalerbaseobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoOptimizedPixelDataScalerU8ToU16BaseContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KisDitherOpContractTest.cpp
    LINK_LIBRARIES
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDitherOpContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisDitherOpContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoConvolutionOpContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoConvolutionOpContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KoOptimizedPixelDataScalerU8ToU16FactoryContractTest.cpp
    LINK_LIBRARIES
        kritapigmentoptimizedpixelscalerfactoryobjects
        kritapigmentoptimizedpixelscalerbaseobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoOptimizedPixelDataScalerU8ToU16FactoryContractTest PRIVATE kritapigment_EXPORTS)
if(__per_arch_rgb_scaler_vector_objs)
    target_link_libraries(KoOptimizedPixelDataScalerU8ToU16FactoryContractTest
        kritapigmentoptimizedpixelscalervectorobjects
    )
endif()

kis_add_test(
    KoChannelInfoContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoChannelInfoContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KoMixColorsOpContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoMixColorsOpContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

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
    KoColorTransformationContractTest.cpp
    LINK_LIBRARIES
        kritapigmentcolortransformationobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoColorTransformationContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoColorTransformationFactoryContractTest.cpp
    LINK_LIBRARIES
        kritapigmentcolortransformationfactoryobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoColorTransformationFactoryContractTest PRIVATE
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KoColorConversionTransformationAbstractFactoryContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoColorConversionTransformationAbstractFactoryContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoColorConversionTransformationAbstractFactoryContractTest PRIVATE
    kritapigment_EXPORTS
)

kis_add_test(
    KoColorSpaceEngineContractTest.cpp
    LINK_LIBRARIES
        kritapigmentcolorspaceengineobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoColorSpaceEngineContractTest PRIVATE
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KoColorProfileQueryContractTest.cpp
    LINK_LIBRARIES
        kritapigmentcolorimetryobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoColorProfileQueryContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoColorProfileQueryContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoColorProfileConstantsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoColorProfileConstantsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KoColorProfileSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KoColorProfileSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoColorProfileSchemaContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoHistogramProducerContractTest.cpp
    LINK_LIBRARIES
        kritapigmenthistogramproducerobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoHistogramProducerContractTest PRIVATE
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KoColorTransferFunctionsContractTest.cpp
    LINK_LIBRARIES
        kritamultiarch
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KoColorTransferFunctionsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../compositeops
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KoColorSpaceConstantsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoColorSpaceConstantsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KoColorSpaceSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KoColorSpaceSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KoColorSpaceSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KoCompositeOpIdsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoCompositeOpIdsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KoCompositeOpFunctionsContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Imath::ImathConfig
)
target_sources(KoCompositeOpFunctionsContractTest PRIVATE
    $<TARGET_OBJECTS:kritapigmentmathsobjects>
)
target_include_directories(KoCompositeOpFunctionsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../compositeops
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KoCompositeOpFunctionsContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoStreamedMathContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Imath::ImathConfig
)
target_include_directories(KoStreamedMathContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../compositeops
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/multiarch
)
target_compile_definitions(KoStreamedMathContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    TestKoColorSpaceMaths.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Imath::ImathConfig
)
target_sources(TestKoColorSpaceMaths PRIVATE
    $<TARGET_OBJECTS:kritapigmentmathsobjects>
)
target_include_directories(TestKoColorSpaceMaths PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(TestKoColorSpaceMaths PRIVATE
    IMATH_HALF_NO_LOOKUP_TABLE
    kritapigment_EXPORTS
)

kis_add_test(
    KoColorSpaceBlendingPolicyContractTest.cpp
    LINK_LIBRARIES
        kritapigmentcolorspaceblendingpolicyobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoColorSpaceBlendingPolicyContractTest PRIVATE kritapigment_EXPORTS)

kis_add_test(
    KoAlphaDarkenParamsWrapperContractTest.cpp
    LINK_LIBRARIES
        kritapigmentalphadarkenparamswrapperobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigCore
        Imath::ImathConfig
        Boost::boost
)
target_compile_definitions(KoAlphaDarkenParamsWrapperContractTest PRIVATE kritapigment_EXPORTS)

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

kis_add_test(
    TestColorConversion.cpp
    LINK_LIBRARIES
        kritapigmentcolorconversionsobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestColorConversion PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(TestColorConversion PRIVATE kritapigment_EXPORTS)

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
    TestKisSwatchGroup.cpp
    TestKoStopGradient.cpp

    LINK_LIBRARIES kritapigment kritatestsdk
    )


macos_test_fixrpath(CCSGraph)
