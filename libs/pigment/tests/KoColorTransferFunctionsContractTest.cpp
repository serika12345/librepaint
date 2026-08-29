/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoColorTransferFunctions.h"

#include <QTest>

#include <array>
#include <cmath>
#include <type_traits>

namespace
{

bool closeFloat(float actual, float expected, float tolerance = 1e-5f)
{
    return std::abs(actual - expected) <= tolerance;
}

void verifyRgb(const float *actual, const std::array<float, 3> &expected, float tolerance = 1e-6f)
{
    for (std::size_t i = 0; i < expected.size(); ++i) {
        QVERIFY(closeFloat(actual[i], expected[i], tolerance));
    }
}

} // namespace

class KoColorTransferFunctionsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constantsAndPoliciesKeepStableValues();
    void pqCurvePreservesReferencePointsAndRoundTrips();
    void hlgCurvePreservesBranchesLimitsAndRoundTrips();
    void smpte428CurvePreservesReferencePointsAndRoundTrips();
    void hlgOotfUsesLumaAndGamma();
    void hlgOotfIgnoresNominalPeak();
    void vectorRemovalTransformsEveryLane();
};

void KoColorTransferFunctionsContractTest::constantsAndPoliciesKeepStableValues()
{
    QCOMPARE(max12bit, uint16_t(4095));
    QCOMPARE(max16bit, 65535.0f);
    QCOMPARE(multiplier10bit, 1.0f / 1023.0f);
    QCOMPARE(multiplier12bit, 1.0f / 4095.0f);
    QCOMPARE(multiplier16bit, 1.0f / 65535.0f);

    QCOMPARE(static_cast<int>(LinearizePolicy::KeepTheSame), 0);
    QCOMPARE(static_cast<int>(LinearizePolicy::LinearFromPQ), 1);
    QCOMPARE(static_cast<int>(LinearizePolicy::LinearFromHLG), 2);
    QCOMPARE(static_cast<int>(LinearizePolicy::LinearFromSMPTE428), 3);
    QCOMPARE(static_cast<int>(ConversionPolicy::KeepTheSame), 0);
    QCOMPARE(static_cast<int>(ConversionPolicy::ApplyPQ), 1);
    QCOMPARE(static_cast<int>(ConversionPolicy::ApplyHLG), 2);
    QCOMPARE(static_cast<int>(ConversionPolicy::ApplySMPTE428), 3);
}

void KoColorTransferFunctionsContractTest::pqCurvePreservesReferencePointsAndRoundTrips()
{
    const float encodedBlack = applySmpte2048Curve(0.0f);

    QVERIFY(closeFloat(encodedBlack, 7.309559e-7f, 1e-9f));
    QCOMPARE(applySmpte2048Curve(-1.0f), encodedBlack);
    QVERIFY(closeFloat(applySmpte2048Curve(1.0f), 0.5806889f, 5e-6f));
    QCOMPARE(removeSmpte2048Curve(0.0f), 0.0f);
    QVERIFY(closeFloat(removeSmpte2048Curve(applySmpte2048Curve(0.18f)), 0.18f, 1e-4f));
    QVERIFY(closeFloat(removeSmpte2048Curve(applySmpte2048Curve(1.0f)), 1.0f, 1e-4f));
    QVERIFY(closeFloat(applySmpte2048Curve(1.0f, 10000.0f), 1.0f, 1e-6f));
    QVERIFY(closeFloat(removeSmpte2048Curve(1.0f, 10000.0f), 1.0f, 1e-6f));
}

void KoColorTransferFunctionsContractTest::hlgCurvePreservesBranchesLimitsAndRoundTrips()
{
    constexpr float branchInput = 3.7743f / 12.0f;

    QCOMPARE(applyHLGCurve(0.0f), 0.0f);
    QVERIFY(closeFloat(applyHLGCurve(branchInput), 0.5f, 1e-6f));
    QVERIFY(closeFloat(removeHLGCurve(0.5f), branchInput, 1e-6f));
    QVERIFY(closeFloat(removeHLGCurve(applyHLGCurve(0.18f)), 0.18f, 1e-5f));
    QVERIFY(closeFloat(removeHLGCurve(applyHLGCurve(1.0f)), 1.0f, 1e-5f));
    QCOMPARE(applyHLGCurve(100.0f), 1.0f);
    QVERIFY(closeFloat(removeHLGCurve(1.0f), 3.7743f, 1e-5f));
}

void KoColorTransferFunctionsContractTest::smpte428CurvePreservesReferencePointsAndRoundTrips()
{
    constexpr float referenceInput = 52.37f / 48.0f;

    QCOMPARE(applySMPTE_ST_428Curve(0.0f), 0.0f);
    QCOMPARE(removeSMPTE_ST_428Curve(0.0f), 0.0f);
    QVERIFY(closeFloat(applySMPTE_ST_428Curve(referenceInput), 1.0f, 1e-6f));
    QVERIFY(closeFloat(removeSMPTE_ST_428Curve(1.0f), referenceInput, 1e-6f));
    QVERIFY(closeFloat(removeSMPTE_ST_428Curve(applySMPTE_ST_428Curve(0.18f)), 0.18f, 1e-5f));
    QVERIFY(closeFloat(removeSMPTE_ST_428Curve(applySMPTE_ST_428Curve(1.0f)), 1.0f, 1e-5f));
}

void KoColorTransferFunctionsContractTest::hlgOotfUsesLumaAndGamma()
{
    const double lumaCoefficients[] = {0.25, 0.5, 0.25};
    float displayLinear[] = {0.25f, 0.5f, 0.75f};
    float sceneLinear[] = {0.25f, 0.5f, 0.75f};

    QCOMPARE(HLGOOTFGamma(1000.0f), 1.2f);
    QVERIFY(closeFloat(HLGOOTFGamma(2000.0f), 1.3332f, 1e-5f));

    applyHLGOOTF(displayLinear, lumaCoefficients, 2.0f);
    verifyRgb(displayLinear, {0.0625f, 0.125f, 0.1875f});

    removeHLGOOTF(sceneLinear, lumaCoefficients, 2.0f);
    const float multiplier = std::sqrt(0.5f);
    verifyRgb(sceneLinear, {0.25f * multiplier, 0.5f * multiplier, 0.75f * multiplier});
}

void KoColorTransferFunctionsContractTest::hlgOotfIgnoresNominalPeak()
{
    const double lumaCoefficients[] = {0.25, 0.5, 0.25};
    float lowPeakApply[] = {0.25f, 0.5f, 0.75f};
    float highPeakApply[] = {0.25f, 0.5f, 0.75f};
    float lowPeakRemove[] = {0.25f, 0.5f, 0.75f};
    float highPeakRemove[] = {0.25f, 0.5f, 0.75f};

    applyHLGOOTF(lowPeakApply, lumaCoefficients, 1.2f, 100.0f);
    applyHLGOOTF(highPeakApply, lumaCoefficients, 1.2f, 4000.0f);
    removeHLGOOTF(lowPeakRemove, lumaCoefficients, 1.2f, 100.0f);
    removeHLGOOTF(highPeakRemove, lumaCoefficients, 1.2f, 4000.0f);

    for (std::size_t i = 0; i < 3; ++i) {
        QCOMPARE(lowPeakApply[i], highPeakApply[i]);
        QCOMPARE(lowPeakRemove[i], highPeakRemove[i]);
    }
}

void KoColorTransferFunctionsContractTest::vectorRemovalTransformsEveryLane()
{
#if !defined(XSIMD_NO_SUPPORTED_ARCHITECTURE)
    using TransferFunctions = KoColorTransferFunctions<xsimd::current_arch>;
    using Batch = TransferFunctions::float_v;
    static_assert(std::is_same_v<Batch, xsimd::batch<float, xsimd::current_arch>>);

    constexpr std::size_t laneCount = Batch::size;
    std::array<float, laneCount> input{};
    std::array<float, laneCount> output{};

    const std::array<float, 4> pqValues{0.1f, 0.5f, 0.75f, 1.0f};
    for (std::size_t i = 0; i < laneCount; ++i) {
        input[i] = pqValues[i % pqValues.size()];
    }
    Batch values = Batch::load_unaligned(input.data());
    TransferFunctions::removeSmpte2048Curve(values);
    values.store_unaligned(output.data());
    for (std::size_t i = 0; i < laneCount; ++i) {
        QVERIFY(closeFloat(output[i], removeSmpte2048Curve(input[i]), 2e-4f));
    }

    const std::array<float, 4> hlgValues{0.0f, 0.5f, 0.75f, 1.0f};
    for (std::size_t i = 0; i < laneCount; ++i) {
        input[i] = hlgValues[i % hlgValues.size()];
    }
    values = Batch::load_unaligned(input.data());
    TransferFunctions::removeHLGCurve(values);
    values.store_unaligned(output.data());
    for (std::size_t i = 0; i < laneCount; ++i) {
        QVERIFY(closeFloat(output[i], removeHLGCurve(input[i]), 2e-5f));
    }

    const std::array<float, 4> smpte428Values{0.0f, 0.25f, 0.75f, 1.0f};
    for (std::size_t i = 0; i < laneCount; ++i) {
        input[i] = smpte428Values[i % smpte428Values.size()];
    }
    values = Batch::load_unaligned(input.data());
    TransferFunctions::removeSMPTE_ST_428Curve(values);
    values.store_unaligned(output.data());
    for (std::size_t i = 0; i < laneCount; ++i) {
        QVERIFY(closeFloat(output[i], removeSMPTE_ST_428Curve(input[i]), 2e-5f));
    }
#else
    QSKIP("xsimd has no supported architecture");
#endif
}

QTEST_GUILESS_MAIN(KoColorTransferFunctionsContractTest)

#include "KoColorTransferFunctionsContractTest.moc"
