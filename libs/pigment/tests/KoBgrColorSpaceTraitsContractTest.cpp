/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoBgrColorSpaceTraits.h"

#include <QTest>

#include <type_traits>

class KoBgrColorSpaceTraitsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pixelLayoutPreservesBlueGreenRedAndAlphaChannels();
    void accessorsReadAndWriteOnlyTheirColorChannel();
    void namedTraitsPreserveChannelTypes();
};

void KoBgrColorSpaceTraitsContractTest::pixelLayoutPreservesBlueGreenRedAndAlphaChannels()
{
    using Traits = KoBgrTraits<quint16>;
    using Pixel = Traits::Pixel;

    static_assert(std::is_same_v<Traits::channels_type, quint16>);
    static_assert(std::is_same_v<Traits::parent, KoColorSpaceTrait<quint16, 4, 3>>);

    QCOMPARE(qint32(Traits::blue_pos), qint32(0));
    QCOMPARE(qint32(Traits::green_pos), qint32(1));
    QCOMPARE(qint32(Traits::red_pos), qint32(2));

    const Pixel pixel{100, 200, 300, 400};
    QCOMPARE(pixel.blue, quint16(100));
    QCOMPARE(pixel.green, quint16(200));
    QCOMPARE(pixel.red, quint16(300));
    QCOMPARE(pixel.alpha, quint16(400));
}

void KoBgrColorSpaceTraitsContractTest::accessorsReadAndWriteOnlyTheirColorChannel()
{
    KoBgrU16Traits::Pixel pixel{1000, 2000, 3000, 50000};
    auto *bytes = reinterpret_cast<quint8 *>(&pixel);

    QCOMPARE(KoBgrU16Traits::blue(bytes), quint16(1000));
    QCOMPARE(KoBgrU16Traits::green(bytes), quint16(2000));
    QCOMPARE(KoBgrU16Traits::red(bytes), quint16(3000));

    KoBgrU16Traits::setBlue(bytes, 1100);
    QCOMPARE(pixel.blue, quint16(1100));
    QCOMPARE(pixel.green, quint16(2000));
    QCOMPARE(pixel.red, quint16(3000));
    QCOMPARE(pixel.alpha, quint16(50000));

    KoBgrU16Traits::setGreen(bytes, 2200);
    QCOMPARE(pixel.blue, quint16(1100));
    QCOMPARE(pixel.green, quint16(2200));
    QCOMPARE(pixel.red, quint16(3000));
    QCOMPARE(pixel.alpha, quint16(50000));

    KoBgrU16Traits::setRed(bytes, 3300);
    QCOMPARE(pixel.blue, quint16(1100));
    QCOMPARE(pixel.green, quint16(2200));
    QCOMPARE(pixel.red, quint16(3300));
    QCOMPARE(pixel.alpha, quint16(50000));
}

void KoBgrColorSpaceTraitsContractTest::namedTraitsPreserveChannelTypes()
{
    static_assert(std::is_base_of_v<KoBgrTraits<quint8>, KoBgrU8Traits>);
    static_assert(std::is_base_of_v<KoBgrTraits<quint16>, KoBgrU16Traits>);
    static_assert(std::is_base_of_v<KoBgrTraits<quint32>, KoBgrU32Traits>);
    static_assert(std::is_base_of_v<KoBgrTraits<float>, KoBgrF32Traits>);
    static_assert(std::is_base_of_v<KoBgrTraits<double>, KoBgrF64Traits>);
#ifdef HAVE_OPENEXR
    static_assert(std::is_base_of_v<KoBgrTraits<half>, KoBgrF16Traits>);
#endif

    QCOMPARE(quint32(KoBgrU8Traits::pixelSize), quint32(4));
    QCOMPARE(quint32(KoBgrU16Traits::pixelSize), quint32(8));
    QCOMPARE(quint32(KoBgrU32Traits::pixelSize), quint32(16));
}

QTEST_GUILESS_MAIN(KoBgrColorSpaceTraitsContractTest)

#include "KoBgrColorSpaceTraitsContractTest.moc"
