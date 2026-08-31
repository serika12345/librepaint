/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoCompositeOpFunctions.h"

#include <QTest>

#include <array>
#include <cmath>

namespace
{
using ChannelFunction = float (*)(float, float);
using IntegerChannelFunction = quint8 (*)(quint8, quint8);
using RgbFunction = void (*)(float, float, float, float &, float &, float &);

struct NamedChannelFunction {
    const char *name;
    ChannelFunction function;
    float expected;
};

struct NamedIntegerChannelFunction {
    const char *name;
    IntegerChannelFunction function;
    quint8 expected;
};

struct NamedRgbFunction {
    const char *name;
    RgbFunction function;
    std::array<float, 3> expected;
};

constexpr float sourceChannel = 0.25f;
constexpr float destinationChannel = 0.75f;

const std::array<NamedChannelFunction, 37> freeChannelFunctions{{
    {"cfAddition", cfAddition<float>, 1.0f},
    {"cfAllanon", cfAllanon<float>, 0.5f},
    {"cfArcTangent", cfArcTangent<float>, 0.204832762f},
    {"cfDarkenOnly", cfDarkenOnly<float>, 0.25f},
    {"cfDifference", cfDifference<float>, 0.5f},
    {"cfDivisiveModulo", cfDivisiveModulo<float>, 1.0f},
    {"cfDivisiveModuloContinuous", cfDivisiveModuloContinuous<float>, 1.0f},
    {"cfEasyBurn", cfEasyBurn<float>, 0.200998142f},
    {"cfEasyDodge", cfEasyDodge<float>, 0.799001873f},
    {"cfFhyrd", cfFhyrd<float>, 0.5f},
    {"cfFogDarkenIFSIllusions", cfFogDarkenIFSIllusions<float>, 0.375f},
    {"cfFogLightenIFSIllusions", cfFogLightenIFSIllusions<float>, 0.625f},
    {"cfFreeze", cfFreeze<float>, 0.75f},
    {"cfGlow", cfGlow<float>, 0.25f},
    {"cfHeat", cfHeat<float>, 0.25f},
    {"cfInterpolation", cfInterpolation<float>, 0.5f},
    {"cfInterpolationB", cfInterpolationB<float>, 0.5f},
    {"cfLightenOnly", cfLightenOnly<float>, 0.75f},
    {"cfLinearLight", cfLinearLight<float>, 0.25f},
    {"cfModulo", cfModulo<float>, 0.249999762f},
    {"cfModuloContinuous", cfModuloContinuous<float>, 0.25f},
    {"cfModuloShift", cfModuloShift<float>, 1.0f},
    {"cfModuloShiftContinuous", cfModuloShiftContinuous<float>, 1.0f},
    {"cfMultiply", cfMultiply<float>, 0.1875f},
    {"cfOver", cfOver<float>, 0.25f},
    {"cfPNormA", cfPNormA<float>, 0.774238825f},
    {"cfPNormB", cfPNormB<float>, 0.752304196f},
    {"cfPenumbraC", cfPenumbraC<float>, 0.5f},
    {"cfPenumbraD", cfPenumbraD<float>, 0.5f},
    {"cfReflect", cfReflect<float>, 0.75f},
    {"cfScreen", cfScreen<float>, 0.8125f},
    {"cfShadeIFSIllusions", cfShadeIFSIllusions<float>, 0.0714745969f},
    {"cfSoftLightIFSIllusions", cfSoftLightIFSIllusions<float>, 0.665748119f},
    {"cfSoftLightPegtopDelphi", cfSoftLightPegtopDelphi<float>, 0.65625f},
    {"cfSubtract", cfSubtract<float>, 0.5f},
    {"cfSuperLight", cfSuperLight<float>, 0.477274477f},
    {"cfTintIFSIllusions", cfTintIFSIllusions<float>, 0.928525388f},
}};

const std::array<NamedIntegerChannelFunction, 10> logicalChannelFunctions{{
    {"cfAnd", cfAnd<quint8>, quint8(0)},
    {"cfConverse", cfConverse<quint8>, quint8(204)},
    {"cfImplies", cfImplies<quint8>, quint8(51)},
    {"cfNand", cfNand<quint8>, quint8(255)},
    {"cfNor", cfNor<quint8>, quint8(0)},
    {"cfNotConverse", cfNotConverse<quint8>, quint8(51)},
    {"cfNotImplies", cfNotImplies<quint8>, quint8(204)},
    {"cfOr", cfOr<quint8>, quint8(255)},
    {"cfXnor", cfXnor<quint8>, quint8(0)},
    {"cfXor", cfXor<quint8>, quint8(255)},
}};

const std::array<NamedChannelFunction, 33> singleChannelFunctors{{
    {"CFAdditiveSubtractive", CFAdditiveSubtractive<float>::composeChannel, 0.366025418f},
    {"CFColorBurn", CFColorBurn<float>::composeChannel, 0.0f},
    {"CFColorDodge", CFColorDodge<float, KoCompositeOpClampPolicy::ClampAsFloatSDR>::composeChannel, 1.0f},
    {"CFDivide", CFDivide<float>::composeChannel, 1.0f},
    {"CFEquivalence", CFEquivalence<float>::composeChannel, 0.5f},
    {"CFExclusion", CFExclusion<float>::composeChannel, 0.625f},
    {"CFFlatLight", CFFlatLight<float>::composeChannel, 0.5f},
    {"CFFrect", CFFrect<float>::composeChannel, 0.75f},
    {"CFGammaDark", CFGammaDark<float>::composeChannel, 0.31640625f},
    {"CFGammaIllumination", CFGammaIllumination<float>::composeChannel, 0.842509866f},
    {"CFGammaLight", CFGammaLight<float>::composeChannel, 0.930604875f},
    {"CFGeometricMean", CFGeometricMean<float>::composeChannel, 0.433012694f},
    {"CFGleat", CFGleat<float>::composeChannel, 0.25f},
    {"CFGrainExtract", CFGrainExtract<float>::composeChannel, 1.0f},
    {"CFGrainMerge", CFGrainMerge<float>::composeChannel, 0.5f},
    {"CFHardLight", CFHardLight<float>::composeChannel, 0.375f},
    {"CFHardMix", CFHardMix<float, KoCompositeOpClampPolicy::ClampAsFloatSDR>::composeChannel, 1.0f},
    {"CFHardMixPhotoshop", CFHardMixPhotoshop<float>::composeChannel, 0.0f},
    {"CFHardMixSofterPhotoshop", CFHardMixSofterPhotoshop<float>::composeChannel, 0.75f},
    {"CFHardOverlay", CFHardOverlay<float, KoCompositeOpClampPolicy::ClampAsFloatSDR>::composeChannel, 0.375f},
    {"CFHelow", CFHelow<float>::composeChannel, 0.25f},
    {"CFInverseSubtract", CFInverseSubtract<float>::composeChannel, 0.0f},
    {"CFLinearBurn", CFLinearBurn<float, KoCompositeOpClampPolicy::ClampAsFloatSDR>::composeChannel, 0.0f},
    {"CFNegation", CFNegation<float>::composeChannel, 1.0f},
    {"CFOverlay", CFOverlay<float>::composeChannel, 0.625f},
    {"CFParallel", CFParallel<float>::composeChannel, 0.375f},
    {"CFPenumbraA", CFPenumbraA<float>::composeChannel, 0.5f},
    {"CFPenumbraB", CFPenumbraB<float>::composeChannel, 0.5f},
    {"CFPinLight", CFPinLight<float>::composeChannel, 0.5f},
    {"CFReeze", CFReeze<float>::composeChannel, 0.75f},
    {"CFSoftLight", CFSoftLight<float>::composeChannel, 0.65625f},
    {"CFSoftLightSvg", CFSoftLightSvg<float>::composeChannel, 0.65625f},
    {"CFVividLight", CFVividLight<float, KoCompositeOpClampPolicy::ClampAsFloatSDR>::composeChannel, 0.5f},
}};

const std::array<NamedRgbFunction, 10> hsyRgbFunctors{{
    {"CFColor", CFColor<HSYType, float>::composeChannels, {0.351199985f, 0.551200032f, 0.951200008f}},
    {"CFDarkerColor", CFDarkerColor<HSYType, float>::composeChannels, {0.2f, 0.4f, 0.8f}},
    {"CFDecreaseLightness", CFDecreaseLightness<HSYType, float>::composeChannels, {0.0f, 0.0f, 0.0f}},
    {"CFDecreaseSaturation",
     CFDecreaseSaturation<HSYType, float>::composeChannels,
     {0.634800017f, 0.514800012f, 0.394800007f}},
    {"CFHue", CFHue<HSYType, float>::composeChannels, {0.413133323f, 0.546466649f, 0.813133299f}},
    {"CFIncreaseLightness", CFIncreaseLightness<HSYType, float>::composeChannels, {1.0f, 0.90527606f, 0.81055212f}},
    {"CFIncreaseSaturation",
     CFIncreaseSaturation<HSYType, float>::composeChannels,
     {0.846699953f, 0.466700017f, 0.0866999924f}},
    {"CFLighterColor", CFLighterColor<HSYType, float>::composeChannels, {0.7f, 0.5f, 0.3f}},
    {"CFLightness", CFLightness<HSYType, float>::composeChannels, {0.548799992f, 0.348800004f, 0.148800015f}},
    {"CFSaturation", CFSaturation<HSYType, float>::composeChannels, {0.781499982f, 0.4815f, 0.181499988f}},
}};

const std::array<NamedRgbFunction, 5> remainingRgbFunctors{{
    {"CFReorientedNormalMapCombine",
     CFReorientedNormalMapCombine<float>::composeChannels,
     {0.99794656f, 0.54526788f, 0.5f}},
    {"CFLambertLighting", CFLambertLighting<HSYType, float>::composeChannels, {0.649428189f, 0.927150071f, 1.0f}},
    {"CFLambertLightingGamma2_2",
     CFLambertLightingGamma2_2<HSYType, float>::composeChannels,
     {0.280000001f, 0.400000006f, 0.480000019f}},
    {"CFTangentNormalmap", CFTangentNormalmap<float>::composeChannels, {0.399999976f, 0.400000006f, 0.100000024f}},
    {"CFTint", CFTint<HSYType, float>::composeChannels, {0.629599988f, 0.722199976f, 0.907400012f}},
}};

bool closeTo(float actual, float expected)
{
    return qAbs(actual - expected) <= 1.0e-6f * qMax(1.0f, qAbs(expected));
}
} // namespace

class KoCompositeOpFunctionsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void channelRangeHelpersPreserveTypePolicy();
    void freeChannelFunctionsPreserveObservedValues();
    void alphaAwareAdditionPreservesObservedValues();
    void singleChannelFunctorsPreserveObservedValues();
    void hsyRgbFunctorsPreserveObservedValues();
    void normalLightingAndTintFunctorsPreserveObservedValues();
};

void KoCompositeOpFunctionsContractTest::channelRangeHelpersPreserveTypePolicy()
{
    float red = -0.1f;
    float green = 0.4f;
    float blue = 1.2f;
    possiblyFixNegativeValuesNearZeroPoint<float>(red, green, blue);
    QCOMPARE(red, 0.0f);
    QCOMPARE(green, 0.4f);
    QCOMPARE(blue, 1.2f);

    red = -0.1f;
    green = 0.4f;
    blue = 1.2f;
    possiblyClampValuesToSDR<float>(red, green, blue);
    QCOMPARE(red, 0.0f);
    QCOMPARE(green, 0.4f);
    QCOMPARE(blue, 1.0f);

    red = -0.1f;
    green = 0.4f;
    blue = 1.2f;
    possiblyFixNegativeValuesNearZeroPoint<quint8>(red, green, blue);
    QCOMPARE(red, -0.1f);
    QCOMPARE(green, 0.4f);
    QCOMPARE(blue, 1.2f);
    possiblyClampValuesToSDR<quint8>(red, green, blue);
    QCOMPARE(red, -0.1f);
    QCOMPARE(green, 0.4f);
    QCOMPARE(blue, 1.2f);
}

void KoCompositeOpFunctionsContractTest::freeChannelFunctionsPreserveObservedValues()
{
    for (const NamedChannelFunction &entry : freeChannelFunctions) {
        const float value = entry.function(sourceChannel, destinationChannel);
        QVERIFY2(std::isfinite(value), entry.name);
        QVERIFY2(closeTo(value, entry.expected), entry.name);
    }

    for (const NamedIntegerChannelFunction &entry : logicalChannelFunctions) {
        const quint8 value = entry.function(quint8(0x33), quint8(0xcc));
        QCOMPARE(value, entry.expected);
    }

    QCOMPARE(cfOver(0.25f, 0.75f), 0.25f);
    QCOMPARE(cfMultiply(0.0f, 0.75f), 0.0f);
    QCOMPARE(cfScreen(1.0f, 0.25f), 1.0f);
}

void KoCompositeOpFunctionsContractTest::alphaAwareAdditionPreservesObservedValues()
{
    float destination = 0.75f;
    float destinationAlpha = 0.4f;
    cfAdditionSAI<HSYType>(0.25f, 0.5f, destination, destinationAlpha);
    QVERIFY(closeTo(destination, 0.875f));
    QVERIFY(closeTo(destinationAlpha, 0.4f));
}

void KoCompositeOpFunctionsContractTest::singleChannelFunctorsPreserveObservedValues()
{
    for (const NamedChannelFunction &entry : singleChannelFunctors) {
        const float value = entry.function(sourceChannel, destinationChannel);
        QVERIFY2(std::isfinite(value), entry.name);
        QVERIFY2(closeTo(value, entry.expected), entry.name);
    }

    QCOMPARE(CFColorBurn<float>::composeChannel(0.25f, 1.0f), 1.0f);
    QCOMPARE(CFInverseSubtract<float>::composeChannel(1.0f, 0.25f), 0.25f);
}

void KoCompositeOpFunctionsContractTest::hsyRgbFunctorsPreserveObservedValues()
{
    for (const NamedRgbFunction &entry : hsyRgbFunctors) {
        float red = 0.7f;
        float green = 0.5f;
        float blue = 0.3f;
        entry.function(0.2f, 0.4f, 0.8f, red, green, blue);
        QVERIFY2(std::isfinite(red) && std::isfinite(green) && std::isfinite(blue), entry.name);
        QVERIFY2(closeTo(red, entry.expected[0]), entry.name);
        QVERIFY2(closeTo(green, entry.expected[1]), entry.name);
        QVERIFY2(closeTo(blue, entry.expected[2]), entry.name);
    }
}

void KoCompositeOpFunctionsContractTest::normalLightingAndTintFunctorsPreserveObservedValues()
{
    for (const NamedRgbFunction &entry : remainingRgbFunctors) {
        float red = 0.7f;
        float green = 0.5f;
        float blue = 0.3f;
        entry.function(0.2f, 0.4f, 0.8f, red, green, blue);
        QVERIFY2(std::isfinite(red) && std::isfinite(green) && std::isfinite(blue), entry.name);
        QVERIFY2(closeTo(red, entry.expected[0]), entry.name);
        QVERIFY2(closeTo(green, entry.expected[1]), entry.name);
        QVERIFY2(closeTo(blue, entry.expected[2]), entry.name);
    }

    float red = 0.7f;
    float green = 0.5f;
    float blue = 0.3f;
    CFReorientedNormalMapCombine<float>::composeChannels(0.2f, 0.4f, 0.0f, red, green, blue);
    QCOMPARE(red, 0.7f);
    QCOMPARE(green, 0.5f);
    QCOMPARE(blue, 0.3f);
}

QTEST_GUILESS_MAIN(KoCompositeOpFunctionsContractTest)

#include "KoCompositeOpFunctionsContractTest.moc"
