/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <surfacecolormanagement/KisSurfaceColorimetryIccUtils.h>

#include <QTest>

#include <array>

using namespace KisSurfaceColorimetry;
using namespace KoColorimetryUtils;

class KisSurfaceColorimetryIccUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void namedPrimariesMapToPigmentConstants();
    void namedTransferFunctionsMapToPigmentConstants();
    void colorSpacesMapToProfileQueries();
};

void KisSurfaceColorimetryIccUtilsContractTest::namedPrimariesMapToPigmentConstants()
{
    const std::array<std::pair<NamedPrimaries, ColorPrimaries>, 6> mappings{{
        {NamedPrimaries::primaries_unknown, PRIMARIES_UNSPECIFIED},
        {NamedPrimaries::primaries_srgb, PRIMARIES_ITU_R_BT_709_5},
        {NamedPrimaries::primaries_bt2020, PRIMARIES_ITU_R_BT_2020_2_AND_2100_0},
        {NamedPrimaries::primaries_dci_p3, PRIMARIES_SMPTE_RP_431_2},
        {NamedPrimaries::primaries_display_p3, PRIMARIES_SMPTE_EG_432_1},
        {NamedPrimaries::primaries_adobe_rgb, PRIMARIES_ADOBE_RGB_1998},
    }};

    for (const auto &[surfacePrimaries, pigmentPrimaries] : mappings) {
        QCOMPARE(namedPrimariesToPigmentPrimaries(surfacePrimaries), pigmentPrimaries);
    }
    QCOMPARE(namedPrimariesToPigmentPrimaries(static_cast<NamedPrimaries>(-1)), PRIMARIES_UNSPECIFIED);
}

void KisSurfaceColorimetryIccUtilsContractTest::namedTransferFunctionsMapToPigmentConstants()
{
    const std::array<std::pair<NamedTransferFunction, TransferCharacteristics>, 9> mappings{{
        {NamedTransferFunction::transfer_function_unknown, TRC_UNSPECIFIED},
        {NamedTransferFunction::transfer_function_bt1886, TRC_ITU_R_BT_709_5},
        {NamedTransferFunction::transfer_function_gamma22, TRC_ITU_R_BT_470_6_SYSTEM_M},
        {NamedTransferFunction::transfer_function_gamma28, TRC_ITU_R_BT_470_6_SYSTEM_B_G},
        {NamedTransferFunction::transfer_function_ext_linear, TRC_LINEAR},
        {NamedTransferFunction::transfer_function_srgb, TRC_IEC_61966_2_1},
        {NamedTransferFunction::transfer_function_ext_srgb, TRC_UNSPECIFIED},
        {NamedTransferFunction::transfer_function_st2084_pq, TRC_ITU_R_BT_2100_0_PQ},
        {NamedTransferFunction::transfer_function_st428, TRC_SMPTE_ST_428_1},
    }};

    for (const auto &[surfaceTransfer, pigmentTransfer] : mappings) {
        QCOMPARE(namedTransferFunctionToPigmentTransferFunction(surfaceTransfer), pigmentTransfer);
    }
    QCOMPARE(namedTransferFunctionToPigmentTransferFunction(static_cast<NamedTransferFunction>(-1)), TRC_UNSPECIFIED);
}

void KisSurfaceColorimetryIccUtilsContractTest::colorSpacesMapToProfileQueries()
{
    ColorSpace namedSpace;
    namedSpace.primaries = NamedPrimaries::primaries_srgb;
    namedSpace.transferFunction = NamedTransferFunction::transfer_function_srgb;

    const KoColorProfileQuery namedRequest = colorSpaceToRequest(namedSpace);
    QCOMPARE(namedRequest.primaries, PRIMARIES_ITU_R_BT_709_5);
    QCOMPARE(namedRequest.transfer, TRC_IEC_61966_2_1);
    QCOMPARE(namedRequest.whitePoint, (xy{0.0, 0.0}));
    QVERIFY(namedRequest.rgbColorants.isEmpty());
    QVERIFY(!namedRequest.hdrReferenceWhite);

    const std::array<std::pair<uint32_t, TransferCharacteristics>, 6> gammaMappings{{
        {10000, TRC_LINEAR},
        {18000, TRC_GAMMA_1_8},
        {22000, TRC_ITU_R_BT_470_6_SYSTEM_M},
        {24000, TRC_GAMMA_2_4},
        {28000, TRC_ITU_R_BT_470_6_SYSTEM_B_G},
        {23000, TRC_UNSPECIFIED},
    }};
    for (const auto &[encodedGamma, pigmentTransfer] : gammaMappings) {
        namedSpace.transferFunction = encodedGamma;
        const KoColorProfileQuery gammaRequest = colorSpaceToRequest(namedSpace);
        QCOMPARE(gammaRequest.primaries, PRIMARIES_ITU_R_BT_709_5);
        QCOMPARE(gammaRequest.transfer, pigmentTransfer);
    }

    ColorSpace chromaticitySpace;
    chromaticitySpace.primaries = Colorimetry::DisplayP3;
    chromaticitySpace.transferFunction = NamedTransferFunction::transfer_function_ext_linear;

    const KoColorProfileQuery chromaticityRequest = colorSpaceToRequest(chromaticitySpace);
    QCOMPARE(chromaticityRequest.primaries, PRIMARIES_UNSPECIFIED);
    QCOMPARE(chromaticityRequest.transfer, TRC_LINEAR);
    QCOMPARE(chromaticityRequest.whitePoint, Colorimetry::DisplayP3.white().toxy());
    QCOMPARE(chromaticityRequest.rgbColorants,
             (QList<xy>{
                 Colorimetry::DisplayP3.red().toxy(),
                 Colorimetry::DisplayP3.green().toxy(),
                 Colorimetry::DisplayP3.blue().toxy(),
             }));
    QVERIFY(!chromaticityRequest.hdrReferenceWhite);

    ColorSpace supportedPqSpace;
    supportedPqSpace.primaries = NamedPrimaries::primaries_bt2020;
    supportedPqSpace.transferFunction = NamedTransferFunction::transfer_function_st2084_pq;
    supportedPqSpace.luminance = Luminance(5, 1000, 203);

    const KoColorProfileQuery supportedPqRequest = colorSpaceToRequest(supportedPqSpace);
    QCOMPARE(supportedPqRequest.primaries, PRIMARIES_ITU_R_BT_2020_2_AND_2100_0);
    QCOMPARE(supportedPqRequest.transfer, TRC_ITU_R_BT_2100_0_PQ);
    QCOMPARE(supportedPqRequest.hdrReferenceWhite, std::make_optional(203.0));

    ColorSpace unsupportedNamedPqSpace = supportedPqSpace;
    unsupportedNamedPqSpace.primaries = NamedPrimaries::primaries_display_p3;
    const KoColorProfileQuery unsupportedNamedPqRequest = colorSpaceToRequest(unsupportedNamedPqSpace);
    QCOMPARE(unsupportedNamedPqRequest.primaries, PRIMARIES_SMPTE_EG_432_1);
    QCOMPARE(unsupportedNamedPqRequest.transfer, TRC_UNSPECIFIED);
    QVERIFY(unsupportedNamedPqRequest.rgbColorants.isEmpty());
    QVERIFY(!unsupportedNamedPqRequest.hdrReferenceWhite);

    ColorSpace unsupportedChromaticityPqSpace = supportedPqSpace;
    unsupportedChromaticityPqSpace.primaries = Colorimetry::BT2020;
    const KoColorProfileQuery unsupportedChromaticityPqRequest = colorSpaceToRequest(unsupportedChromaticityPqSpace);
    QCOMPARE(unsupportedChromaticityPqRequest.primaries, PRIMARIES_UNSPECIFIED);
    QCOMPARE(unsupportedChromaticityPqRequest.transfer, TRC_UNSPECIFIED);
    QCOMPARE(unsupportedChromaticityPqRequest.whitePoint, Colorimetry::BT2020.white().toxy());
    QVERIFY(unsupportedChromaticityPqRequest.rgbColorants.isEmpty());
    QVERIFY(!unsupportedChromaticityPqRequest.hdrReferenceWhite);
}

QTEST_GUILESS_MAIN(KisSurfaceColorimetryIccUtilsContractTest)

#include "KisSurfaceColorimetryIccUtilsContractTest.moc"
