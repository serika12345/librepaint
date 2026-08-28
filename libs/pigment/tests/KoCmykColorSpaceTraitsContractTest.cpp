/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoCmykColorSpaceTraits.h"

#include <QTest>

#include <type_traits>

namespace
{
template<typename Traits>
void verifyFloatingNormalisation()
{
    using Channel = typename Traits::channels_type;
    typename Traits::Pixel pixel{
        Channel(25.0f), Channel(50.0f), Channel(75.0f), Channel(100.0f), Channel(0.5f)};
    auto *bytes = reinterpret_cast<quint8 *>(&pixel);

    QCOMPARE(Traits::normalisedChannelValueText(bytes, Traits::c_pos), QStringLiteral("25"));
    QCOMPARE(Traits::normalisedChannelValueText(bytes, 4), QStringLiteral("50"));
    QCOMPARE(Traits::normalisedChannelValueText(bytes, 6), QStringLiteral("Error"));

    QVector<float> normalised(5);
    Traits::normalisedChannelsValue(bytes, normalised);
    QCOMPARE(normalised[0], 0.25f);
    QCOMPARE(normalised[1], 0.5f);
    QCOMPARE(normalised[2], 0.75f);
    QCOMPARE(normalised[3], 1.0f);
    QCOMPARE(normalised[4], 0.5f);

    typename Traits::Pixel restored{};
    Traits::fromNormalisedChannelsValue(reinterpret_cast<quint8 *>(&restored), normalised);
    QCOMPARE(qreal(restored.cyan), qreal(25.0));
    QCOMPARE(qreal(restored.magenta), qreal(50.0));
    QCOMPARE(qreal(restored.yellow), qreal(75.0));
    QCOMPARE(qreal(restored.black), qreal(100.0));
    QCOMPARE(qreal(restored.alpha), qreal(0.5));
}
}

class KoCmykColorSpaceTraitsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pixelLayoutPreservesCmykAndAlphaChannels();
    void accessorsReadAndWriteOnlyTheirColorChannel();
    void integerNamedTraitsPreserveChannelTypes();
    void float32NormalisationUsesCmykPercentAndUnitAlpha();
    void float64NormalisationUsesCmykPercentAndUnitAlpha();
#ifdef HAVE_OPENEXR
    void float16NormalisationUsesCmykPercentAndUnitAlpha();
#endif
};

void KoCmykColorSpaceTraitsContractTest::pixelLayoutPreservesCmykAndAlphaChannels()
{
    using Traits = KoCmykTraits<quint16>;
    using Pixel = Traits::Pixel;

    static_assert(std::is_same_v<Traits::channels_type, quint16>);
    static_assert(std::is_same_v<Traits::parent, KoColorSpaceTrait<quint16, 5, 4>>);

    QCOMPARE(qint32(Traits::c_pos), qint32(0));
    QCOMPARE(qint32(Traits::m_pos), qint32(1));
    QCOMPARE(qint32(Traits::y_pos), qint32(2));
    QCOMPARE(qint32(Traits::k_pos), qint32(3));

    const Pixel pixel{100, 200, 300, 400, 500};
    QCOMPARE(pixel.cyan, quint16(100));
    QCOMPARE(pixel.magenta, quint16(200));
    QCOMPARE(pixel.yellow, quint16(300));
    QCOMPARE(pixel.black, quint16(400));
    QCOMPARE(pixel.alpha, quint16(500));
}

void KoCmykColorSpaceTraitsContractTest::accessorsReadAndWriteOnlyTheirColorChannel()
{
    KoCmykU16Traits::Pixel pixel{1000, 2000, 3000, 4000, 50000};
    auto *bytes = reinterpret_cast<quint8 *>(&pixel);

    QCOMPARE(KoCmykU16Traits::C(bytes), quint16(1000));
    QCOMPARE(KoCmykU16Traits::M(bytes), quint16(2000));
    QCOMPARE(KoCmykU16Traits::Y(bytes), quint16(3000));
    QCOMPARE(KoCmykU16Traits::k(bytes), quint16(4000));

    KoCmykU16Traits::setC(bytes, 1100);
    KoCmykU16Traits::setM(bytes, 2200);
    KoCmykU16Traits::setY(bytes, 3300);
    KoCmykU16Traits::setK(bytes, 4400);

    QCOMPARE(pixel.cyan, quint16(1100));
    QCOMPARE(pixel.magenta, quint16(2200));
    QCOMPARE(pixel.yellow, quint16(3300));
    QCOMPARE(pixel.black, quint16(4400));
    QCOMPARE(pixel.alpha, quint16(50000));
}

void KoCmykColorSpaceTraitsContractTest::integerNamedTraitsPreserveChannelTypes()
{
    static_assert(std::is_base_of_v<KoCmykTraits<quint8>, KoCmykU8Traits>);
    static_assert(std::is_base_of_v<KoCmykTraits<quint16>, KoCmykU16Traits>);

    QCOMPARE(quint32(KoCmykU8Traits::pixelSize), quint32(5));
    QCOMPARE(quint32(KoCmykU16Traits::pixelSize), quint32(10));
}

void KoCmykColorSpaceTraitsContractTest::float32NormalisationUsesCmykPercentAndUnitAlpha()
{
    static_assert(std::is_base_of_v<KoCmykTraits<float>, KoCmykF32Traits>);
    verifyFloatingNormalisation<KoCmykF32Traits>();
}

void KoCmykColorSpaceTraitsContractTest::float64NormalisationUsesCmykPercentAndUnitAlpha()
{
    static_assert(std::is_base_of_v<KoCmykTraits<double>, KoCmykF64Traits>);
    verifyFloatingNormalisation<KoCmykF64Traits>();
}

#ifdef HAVE_OPENEXR
void KoCmykColorSpaceTraitsContractTest::float16NormalisationUsesCmykPercentAndUnitAlpha()
{
    static_assert(std::is_base_of_v<KoCmykTraits<half>, KoCmykF16Traits>);
    verifyFloatingNormalisation<KoCmykF16Traits>();
}
#endif

QTEST_GUILESS_MAIN(KoCmykColorSpaceTraitsContractTest)

#include "KoCmykColorSpaceTraitsContractTest.moc"
