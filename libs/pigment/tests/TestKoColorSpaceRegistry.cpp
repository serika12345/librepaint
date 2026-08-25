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
