/*
 *  SPDX-FileCopyrightText: 2010 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "TestKoColorSpaceRegistry.h"

#include <simpletest.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>
#include <KoColorProfileQuery.h>
#include <KoColorSpace.h>
#include <KoCompositeOp.h>
#include <KoCompositeOpIds.h>
#include <array>
#include <QColorSpace>

TestBaseColorSpaceRegistry::TestBaseColorSpaceRegistry()
{
}

void TestBaseColorSpaceRegistry::testLab16()
{
    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->lab16();
    QCOMPARE(cs->colorModelId().id(), LABAColorModelID.id());
    QCOMPARE(cs->colorDepthId().id(), Integer16BitsColorDepthID.id());
    QVERIFY(*cs == *KoColorSpaceRegistry::instance()->colorSpace(LABAColorModelID.id(), Integer16BitsColorDepthID.id(), 0));
}

void TestBaseColorSpaceRegistry::testRgb8()
{
    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->rgb8();
    QCOMPARE(cs->colorModelId().id(), RGBAColorModelID.id());
    QCOMPARE(cs->colorDepthId().id(), Integer8BitsColorDepthID.id());
    QVERIFY(*cs == *KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), 0));
}

void TestBaseColorSpaceRegistry::testRgb16()
{
    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->rgb16();
    QCOMPARE(cs->colorModelId().id(), RGBAColorModelID.id());
    QCOMPARE(cs->colorDepthId().id(), Integer16BitsColorDepthID.id());
    QVERIFY(*cs == *KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), 0));
}

namespace {
template<typename Component>
void compareTransparentNormalComposition(const KoColorSpace *colorSpace,
                                         const std::array<Component, 4> &initial,
                                         const std::array<Component, 4> &paint,
                                         float opacity, bool masked, int destinationAlpha)
{
    using Pixel = std::array<Component, 4>;
    alignas(64) std::array<Pixel, 64> source{};
    alignas(64) std::array<Pixel, 64> destination{};
    alignas(64) std::array<Pixel, 64> individual{};
    std::array<quint8, 64> mask{};
    destination.fill(initial);
    for (std::size_t i = 0; i < destination.size(); ++i) {
        if (destinationAlpha == 1 || (destinationAlpha == 2 && i % 3 != 0)) {
            destination[i][3] = paint[3];
        }
    }
    individual = destination;
    const auto original = destination;
    if (masked) {
        source.fill(paint);
        mask[17] = 255;
    } else {
        source[17] = paint;
    }

    const KoCompositeOp *operation = colorSpace->compositeOp(COMPOSITE_OVER);
    QVERIFY(operation);
    KoCompositeOp::ParameterInfo parameters;
    parameters.srcRowStart = reinterpret_cast<const quint8 *>(source.data());
    parameters.dstRowStart = reinterpret_cast<quint8 *>(destination.data());
    parameters.srcRowStride = sizeof(source);
    parameters.dstRowStride = sizeof(destination);
    parameters.maskRowStart = masked ? mask.data() : nullptr;
    parameters.maskRowStride = mask.size();
    parameters.cols = source.size();
    parameters.rows = 1;
    parameters.opacity = opacity;
    operation->composite(parameters);

    parameters.cols = 1;
    for (std::size_t i = 0; i < source.size(); ++i) {
        parameters.srcRowStart = reinterpret_cast<const quint8 *>(&source[i]);
        parameters.dstRowStart = reinterpret_cast<quint8 *>(&individual[i]);
        parameters.maskRowStart = masked ? &mask[i] : nullptr;
        operation->composite(parameters);
    }

    // Painting a span or separate pixels must preserve the same hidden colors,
    // signed zero alpha and direct-copy HDR values in a transparent destination.
    for (std::size_t i = 0; i < destination.size(); ++i) {
        const QByteArray actual(reinterpret_cast<const char *>(&destination[i]), sizeof(Pixel));
        const QByteArray expected(reinterpret_cast<const char *>(&individual[i]), sizeof(Pixel));
        QVERIFY2(destinationAlpha != 0 || actual == expected, qPrintable(QStringLiteral("pixel %1: %2, expected %3")
                     .arg(i).arg(QString::fromLatin1(actual.toHex()), QString::fromLatin1(expected.toHex()))));
        if (i != 17 || opacity == 0.0f) {
            QCOMPARE(actual, QByteArray(reinterpret_cast<const char *>(&original[i]), sizeof(Pixel)));
        }
    }
}
}

void TestBaseColorSpaceRegistry::testNormalPreservesTransparentPixels_data()
{
    QTest::addColumn<bool>("floatingPoint");
    QTest::addColumn<float>("opacity");
    QTest::addColumn<bool>("masked");
    QTest::addColumn<int>("destinationAlpha");
    for (bool floatingPoint : {false, true}) {
        for (float opacity : {1.0f, 0.5f, 0.0f}) {
            for (bool masked : {false, true}) {
                for (int destinationAlpha : {0, 1, 2}) {
                    QTest::addRow("%s-opacity-%g-%s-destination-%d", floatingPoint ? "f32" : "u16", double(opacity),
                                  masked ? "mask" : "source", destinationAlpha)
                        << floatingPoint << opacity << masked << destinationAlpha;
                }
            }
        }
    }
}

void TestBaseColorSpaceRegistry::testNormalPreservesTransparentPixels()
{
    QFETCH(bool, floatingPoint);
    QFETCH(float, opacity);
    QFETCH(bool, masked);
    QFETCH(int, destinationAlpha);
    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->colorSpace(
        RGBAColorModelID.id(), floatingPoint ? Float32BitsColorDepthID.id() : Integer16BitsColorDepthID.id(),
        QStringLiteral("sRGB-elle-V2-g10.icc"));
    QVERIFY(colorSpace);
    QCOMPARE(colorSpace->pixelSize(), floatingPoint ? 16U : 8U);
    if (floatingPoint) {
        compareTransparentNormalComposition<float>(colorSpace, {1.0e20f, -0.0f, 0.125f, -0.0f},
                                                    {0.125f, -0.25f, 2.0f, 1.0f}, opacity, masked, destinationAlpha);
    } else {
        compareTransparentNormalComposition<quint16>(colorSpace, {1, 257, 65534, 0},
                                                      {1111, 2222, 3333, 65535}, opacity, masked, destinationAlpha);
    }
}

void TestBaseColorSpaceRegistry::testComparisonOperators()
{
    const KoColorSpace *rgb8 = KoColorSpaceRegistry::instance()->rgb8();
    const KoColorSpace *sameRgb8 =
        KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), nullptr);
    const KoColorSpace *lab16 = KoColorSpaceRegistry::instance()->lab16();

    QVERIFY(*rgb8 == *sameRgb8);
    QVERIFY(!(*rgb8 != *sameRgb8));
    QCOMPARE(*rgb8 == *lab16, *lab16 == *rgb8);
    QCOMPARE(*rgb8 != *lab16, *lab16 != *rgb8);
    QCOMPARE(*rgb8 != *lab16, !(*rgb8 == *lab16));
}

void TestBaseColorSpaceRegistry::testProfileByUniqueId()
{
    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->rgb16();
    const KoColorProfile *profile = cs->profile();
    QVERIFY(profile);

    const KoColorProfile *fetchedProfile =
        KoColorSpaceRegistry::instance()->profileByUniqueId(profile->uniqueId());

    QCOMPARE(*fetchedProfile, *profile);
}

void TestBaseColorSpaceRegistry::testQColorSpaceConversionRoundtrip_data()
{
    QTest::addColumn<int>("QPrimaries");
    QTest::addColumn<int>("QTransfer");
    QTest::addColumn<int>("primaries");
    QTest::addColumn<int>("transfer");

    QTest::addRow("sRGB, sRGB-transfer") << int(QColorSpace::Primaries::SRgb) << int(QColorSpace::TransferFunction::SRgb)
                                         << int(PRIMARIES_ITU_R_BT_709_5) << int(TRC_IEC_61966_2_1);
    QTest::addRow("sRGB, linear") << int(QColorSpace::Primaries::SRgb) << int(QColorSpace::TransferFunction::Linear)
                                         << int(PRIMARIES_ITU_R_BT_709_5) << int(TRC_LINEAR);
    // Because every dev's system is a little different, we can only really test the profiles that we can assure, because otherwise unexpected profiles might show up.
    //QTest::addRow("Display p3") << int(QColorSpace::Primaries::DciP3D65) << int(QColorSpace::TransferFunction::SRgb)
    //                              << int(PRIMARIES_SMPTE_EG_432_1) << int(TRC_IEC_61966_2_1);
    //QTest::addRow("Prophoto") << int(QColorSpace::Primaries::ProPhotoRgb) << int(QColorSpace::TransferFunction::ProPhotoRgb)
    //                              << int(PRIMARIES_PROPHOTO) << int(TRC_PROPHOTO);
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QTest::addRow("rec2020") << int(QColorSpace::Primaries::Bt2020) << int(QColorSpace::TransferFunction::Bt2020)
                                  << int(PRIMARIES_ITU_R_BT_2020_2_AND_2100_0) << int(TRC_ITU_R_BT_2020_2_12bit);
    QTest::addRow("rec2100pq") << int(QColorSpace::Primaries::Bt2020) << int(QColorSpace::TransferFunction::St2084)
                             << int(PRIMARIES_ITU_R_BT_2020_2_AND_2100_0) << int(TRC_ITU_R_BT_2100_0_PQ);
#endif
}

void TestBaseColorSpaceRegistry::testQColorSpaceConversionRoundtrip()
{
    QFETCH(int, QPrimaries);
    QFETCH(int, QTransfer);
    QFETCH(int, primaries);
    QFETCH(int, transfer);

    KoColorProfileQuery query((ColorPrimaries(primaries)), TransferCharacteristics(transfer));
    const KoColorProfile *reference = KoColorSpaceRegistry::instance()->profileFor(query);
    QVERIFY(reference);

    QColorSpace space = KoColorSpaceRegistry::instance()->QColorSpaceForProfile(reference);

    QVERIFY2(int(space.primaries()) == QPrimaries, QString("QColorspace primaries don't match").toLatin1());
    // While we can create the functions, Qt(5) cannot recognise the transfer function.
    //QVERIFY2(int(space.transferFunction()) == QTransfer, QString("QColorspace transfer doesn't match.").toLatin1());

    const KoColorProfile *roundtrip = KoColorSpaceRegistry::instance()->profileForQColorSpace(space);

    QVERIFY2(roundtrip->name() == reference->name(), QString("Profile didn't roundtrip correctly. Got %1, expected %2").arg(roundtrip->name()).arg(reference->name()).toLatin1());
}

SIMPLE_TEST_MAIN(TestBaseColorSpaceRegistry)
