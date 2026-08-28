/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoGrayColorSpaceTraits.h"

#include <QTest>

#include <type_traits>

class KoGrayColorSpaceTraitsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pixelLayoutPreservesGrayAndAlphaChannels();
    void accessorsReadAndWriteOnlyGrayChannel();
    void namedTraitsPreserveChannelTypes();
};

void KoGrayColorSpaceTraitsContractTest::pixelLayoutPreservesGrayAndAlphaChannels()
{
    using Traits = KoGrayTraits<quint16>;
    using Pixel = Traits::Pixel;

    static_assert(std::is_same_v<Traits::channels_type, quint16>);
    static_assert(std::is_same_v<Traits::parent, KoColorSpaceTrait<quint16, 2, 1>>);

    QCOMPARE(qint32(Traits::gray_pos), qint32(0));

    const Pixel pixel{123, 456};
    QCOMPARE(pixel.gray, quint16(123));
    QCOMPARE(pixel.alpha, quint16(456));
}

void KoGrayColorSpaceTraitsContractTest::accessorsReadAndWriteOnlyGrayChannel()
{
    KoGrayU16Traits::Pixel pixel{1000, 50000};
    auto *bytes = reinterpret_cast<quint8 *>(&pixel);

    QCOMPARE(KoGrayU16Traits::gray(bytes), quint16(1000));
    KoGrayU16Traits::setGray(bytes, 2000);
    QCOMPARE(pixel.gray, quint16(2000));
    QCOMPARE(pixel.alpha, quint16(50000));
}

void KoGrayColorSpaceTraitsContractTest::namedTraitsPreserveChannelTypes()
{
    static_assert(std::is_base_of_v<KoGrayTraits<quint8>, KoGrayU8Traits>);
    static_assert(std::is_base_of_v<KoGrayTraits<quint16>, KoGrayU16Traits>);
    static_assert(std::is_base_of_v<KoGrayTraits<quint32>, KoGrayU32Traits>);
    static_assert(std::is_base_of_v<KoGrayTraits<float>, KoGrayF32Traits>);
    static_assert(std::is_base_of_v<KoGrayTraits<double>, KoGrayF64Traits>);
#ifdef HAVE_OPENEXR
    static_assert(std::is_base_of_v<KoGrayTraits<half>, KoGrayF16Traits>);
#endif

    QCOMPARE(quint32(KoGrayU8Traits::pixelSize), quint32(2));
    QCOMPARE(quint32(KoGrayU16Traits::pixelSize), quint32(4));
    QCOMPARE(quint32(KoGrayU32Traits::pixelSize), quint32(8));
}

QTEST_GUILESS_MAIN(KoGrayColorSpaceTraitsContractTest)

#include "KoGrayColorSpaceTraitsContractTest.moc"
