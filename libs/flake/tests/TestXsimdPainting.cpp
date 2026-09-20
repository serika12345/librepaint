/*
 *  SPDX-FileCopyrightText: 2023 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestXsimdPainting.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QTest>

#include "KoClipMaskPainter.h"

void kis_assert_recoverable(const char *, const char *, int)
{
}

namespace
{
void compareSolidImage(const QImage &image, const QColor &expected)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QCOMPARE(image.pixelColor(x, y), expected);
        }
    }
}

void comparePainterState(const QPainter &actual, const QPainter &expected)
{
    QCOMPARE(actual.transform(), expected.transform());
    QCOMPARE(actual.hasClipping(), expected.hasClipping());
    QCOMPARE(actual.clipPath(), expected.clipPath());
    QCOMPARE(actual.opacity(), expected.opacity());
    QCOMPARE(actual.brush(), expected.brush());
    QCOMPARE(actual.pen(), expected.pen());
}
} // namespace

void TestXsimdPainting::testKoClipMaskPainting_data()
{
    QTest::addColumn<QColor>("colorSource");
    QTest::addColumn<QColor>("colorMask");
    QTest::addColumn<QColor>("colorFinal");

    QTest::addRow("visibleWhite") << QColor(255, 255, 255, 255) << QColor(255, 255, 255, 255)
                                  << QColor(255, 255, 255, 255);
    QTest::addRow("completelyMasked") << QColor(255, 255, 255, 255) << QColor(0, 0, 0, 255) << QColor(0, 0, 0, 0);
    QTest::addRow("greyMask") << QColor(255, 255, 255, 255) << QColor(128, 128, 128, 255) << QColor(255, 255, 255, 128);
    QTest::addRow("semiTransparent") << QColor(255, 255, 255, 255) << QColor(255, 255, 255, 128)
                                     << QColor(255, 255, 255, 128);
    QTest::addRow("semiCyan") << QColor(255, 255, 255, 255) << QColor(128, 255, 255, 128) << QColor(255, 255, 255, 114);
    QTest::addRow("semiMagenta") << QColor(255, 255, 255, 255) << QColor(255, 128, 255, 128)
                                 << QColor(255, 255, 255, 82);
    QTest::addRow("semiYellow") << QColor(255, 255, 255, 255) << QColor(255, 255, 128, 128)
                                << QColor(255, 255, 255, 123);
    QTest::addRow("color1") << QColor(255, 0, 0, 255) << QColor(64, 128, 255, 128) << QColor(255, 0, 0, 62);
    QTest::addRow("color2") << QColor(0, 255, 0, 255) << QColor(255, 128, 64, 128) << QColor(0, 255, 0, 75);
    QTest::addRow("color3") << QColor(0, 0, 255, 255) << QColor(128, 64, 255, 128) << QColor(0, 0, 255, 46);
}

void TestXsimdPainting::testKoClipMaskPainting()
{
    QFETCH(QColor, colorSource);
    QFETCH(QColor, colorMask);
    QFETCH(QColor, colorFinal);

    const QRect imgRect(0, 0, 5, 3);

    QImage img = QImage(imgRect.size(), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);

    KoClipMaskPainter clip(&p, imgRect);
    clip.shapePainter()->fillRect(imgRect, colorSource);
    clip.maskPainter()->fillRect(imgRect, colorMask);

    clip.renderOnGlobalPainter();

    compareSolidImage(img, colorFinal);
}

void TestXsimdPainting::paintersAreDistinctAndInheritGlobalPainterState()
{
    QImage globalImage(64, 48, QImage::Format_ARGB32);
    globalImage.fill(Qt::transparent);
    QPainter globalPainter(&globalImage);

    QTransform transform;
    transform.translate(7.0, 5.0);
    transform.scale(1.5, 0.75);
    globalPainter.setTransform(transform);

    QPainterPath clipPath;
    clipPath.addRoundedRect(QRectF(4.0, 6.0, 20.0, 14.0), 2.0, 2.0);
    globalPainter.setClipPath(clipPath);
    globalPainter.setOpacity(0.625);
    globalPainter.setBrush(QBrush(QColor(12, 34, 56, 78), Qt::Dense3Pattern));
    globalPainter.setPen(QPen(QColor(87, 65, 43, 21), 3.5, Qt::DashDotLine));

    const QRectF globalClipRect(11.25, 9.5, 24.0, 18.0);
    const QRect alignedClipRect = globalClipRect.toAlignedRect();
    KoClipMaskPainter clip(&globalPainter, globalClipRect);

    QPainter *shapePainter = clip.shapePainter();
    QPainter *maskPainter = clip.maskPainter();
    QVERIFY(shapePainter);
    QVERIFY(maskPainter);
    QVERIFY(shapePainter != maskPainter);
    QVERIFY(shapePainter->isActive());
    QVERIFY(maskPainter->isActive());

    QImage expectedImage(alignedClipRect.size(), QImage::Format_ARGB32);
    QPainter expectedPainter(&expectedImage);
    expectedPainter.setTransform(QTransform::fromTranslate(-alignedClipRect.x(), -alignedClipRect.y()));
    expectedPainter.setTransform(globalPainter.transform(), true);
    expectedPainter.setClipPath(globalPainter.clipPath());
    expectedPainter.setOpacity(globalPainter.opacity());
    expectedPainter.setBrush(globalPainter.brush());
    expectedPainter.setPen(globalPainter.pen());

    comparePainterState(*shapePainter, expectedPainter);
    comparePainterState(*maskPainter, expectedPainter);
}

void TestXsimdPainting::rendersAtAlignedGlobalClipRect()
{
    QImage globalImage(12, 10, QImage::Format_ARGB32);
    globalImage.fill(Qt::transparent);
    QPainter globalPainter(&globalImage);

    const QRectF globalClipRect(3.25, 2.5, 4.5, 3.25);
    const QRect alignedClipRect = globalClipRect.toAlignedRect();
    KoClipMaskPainter clip(&globalPainter, globalClipRect);

    clip.shapePainter()->resetTransform();
    clip.maskPainter()->resetTransform();
    clip.shapePainter()->fillRect(QRect(QPoint(), alignedClipRect.size()), Qt::white);
    clip.maskPainter()->fillRect(QRect(QPoint(), alignedClipRect.size()), Qt::white);
    clip.renderOnGlobalPainter();

    for (int y = 0; y < globalImage.height(); ++y) {
        for (int x = 0; x < globalImage.width(); ++x) {
            const QColor expected = alignedClipRect.contains(x, y) ? QColor(Qt::white) : QColor(Qt::transparent);
            QCOMPARE(globalImage.pixelColor(x, y), expected);
        }
    }
}

void TestXsimdPainting::scopedLifetimePreservesBorrowedPainterAndState()
{
    QImage globalImage(16, 16, QImage::Format_ARGB32);
    globalImage.fill(Qt::transparent);
    QPainter globalPainter(&globalImage);
    globalPainter.translate(2.0, 3.0);
    globalPainter.setClipRect(QRectF(1.0, 1.0, 10.0, 9.0));
    globalPainter.setOpacity(0.75);
    globalPainter.setBrush(QColor(20, 40, 60));
    globalPainter.setPen(QPen(QColor(80, 100, 120), 2.0));

    const QTransform transform = globalPainter.transform();
    const QPainterPath clipPath = globalPainter.clipPath();
    const qreal opacity = globalPainter.opacity();
    const QBrush brush = globalPainter.brush();
    const QPen pen = globalPainter.pen();

    {
        KoClipMaskPainter clip(&globalPainter, QRectF(2.0, 3.0, 8.0, 7.0));
        QVERIFY(clip.shapePainter()->isActive());
        QVERIFY(clip.maskPainter()->isActive());
        clip.shapePainter()->fillRect(QRectF(2.0, 3.0, 2.0, 2.0), Qt::white);
        clip.maskPainter()->fillRect(QRectF(2.0, 3.0, 2.0, 2.0), Qt::white);
        clip.renderOnGlobalPainter();
    }

    QVERIFY(globalPainter.isActive());
    QCOMPARE(globalPainter.transform(), transform);
    QCOMPARE(globalPainter.clipPath(), clipPath);
    QCOMPARE(globalPainter.opacity(), opacity);
    QCOMPARE(globalPainter.brush(), brush);
    QCOMPARE(globalPainter.pen(), pen);

    globalPainter.setCompositionMode(QPainter::CompositionMode_Source);
    globalPainter.resetTransform();
    globalPainter.setClipping(false);
    globalPainter.setOpacity(1.0);
    globalPainter.fillRect(QRect(0, 0, 1, 1), QColor(1, 2, 3, 255));
    QCOMPARE(globalImage.pixelColor(0, 0), QColor(1, 2, 3, 255));
}

QTEST_GUILESS_MAIN(TestXsimdPainting)
