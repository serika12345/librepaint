/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoBgrColorSpaceTraits.h"
#include "KoColorSpacePreserveLightnessUtils.h"

#include <QTest>

#include <array>

class KoColorSpacePreserveLightnessUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void grayBrushFillsWithSourceHueAndMaskLightness();
    void grayBrushModulatesExistingLightnessAndPreservesAlpha();
};

void KoColorSpacePreserveLightnessUtilsContractTest::grayBrushFillsWithSourceHueAndMaskLightness()
{
    using Traits = KoBgrU8Traits;
    using Pixel = Traits::Pixel;

    Pixel brushColor{0, 0, 255, 200};
    std::array<QRgb, 2> brush{qRgba(0, 0, 0, 0), qRgba(255, 255, 255, 255)};
    std::array<Pixel, 2> pixels{};

    fillGrayBrushWithColorPreserveLightnessRGB<Traits>(
        reinterpret_cast<quint8 *>(pixels.data()),
        brush.data(),
        reinterpret_cast<quint8 *>(&brushColor),
        1.0,
        qint32(pixels.size()));

    QCOMPARE(pixels[0].blue, quint8(0));
    QCOMPARE(pixels[0].green, quint8(0));
    QCOMPARE(pixels[0].red, quint8(0));
    QCOMPARE(pixels[0].alpha, quint8(0));

    QCOMPARE(pixels[1].blue, quint8(255));
    QCOMPARE(pixels[1].green, quint8(255));
    QCOMPARE(pixels[1].red, quint8(255));
    QCOMPARE(pixels[1].alpha, quint8(200));
}

void KoColorSpacePreserveLightnessUtilsContractTest::grayBrushModulatesExistingLightnessAndPreservesAlpha()
{
    using Traits = KoBgrU8Traits;
    using Pixel = Traits::Pixel;

    std::array<Pixel, 2> pixels{Pixel{0, 0, 255, 77}, Pixel{0, 0, 255, 88}};
    std::array<QRgb, 2> brush{qRgba(0, 0, 0, 255), qRgba(255, 255, 255, 255)};

    modulateLightnessByGrayBrushRGB<Traits>(
        reinterpret_cast<quint8 *>(pixels.data()), brush.data(), 1.0, qint32(pixels.size()));

    QCOMPARE(pixels[0].blue, quint8(0));
    QCOMPARE(pixels[0].green, quint8(0));
    QCOMPARE(pixels[0].red, quint8(0));
    QCOMPARE(pixels[0].alpha, quint8(77));

    QCOMPARE(pixels[1].blue, quint8(255));
    QCOMPARE(pixels[1].green, quint8(255));
    QCOMPARE(pixels[1].red, quint8(255));
    QCOMPARE(pixels[1].alpha, quint8(88));
}

QTEST_GUILESS_MAIN(KoColorSpacePreserveLightnessUtilsContractTest)

#include "KoColorSpacePreserveLightnessUtilsContractTest.moc"
