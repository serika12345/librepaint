/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoCheckerBoardPainter.h"

#include <QImage>
#include <QPainter>
#include <QTest>

namespace
{
QImage renderPattern(const KoCheckerBoardPainter &checker,
                     const QSize &size,
                     const QPointF *patternOrigin = nullptr,
                     const QRectF *paintRect = nullptr,
                     const QColor &background = Qt::transparent)
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(background);

    QPainter painter(&image);
    const QRectF rect = paintRect ? *paintRect : QRectF(QPointF(), QSizeF(size));
    if (patternOrigin) {
        checker.paint(painter, rect, *patternOrigin);
    } else {
        checker.paint(painter, rect);
    }
    painter.end();

    return image;
}
} // namespace

class KoCheckerBoardPainterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorPaintsDefaultColorsAtRequestedPeriod();
    void settersRebuildThePatternWithNewColorsAndSize();
    void zeroOriginMatchesConvenienceOverload();
    void patternOriginMovesTheCheckerPhase();
    void paintingPreservesPixelsOutsideTheRectangle();
};

void KoCheckerBoardPainterContractTest::constructorPaintsDefaultColorsAtRequestedPeriod()
{
    KoCheckerBoardPainter checker(2);
    const QImage image = renderPattern(checker, QSize(8, 8));

    QCOMPARE(image.pixelColor(0, 0), QColor(Qt::lightGray));
    QCOMPARE(image.pixelColor(2, 0), QColor(Qt::darkGray));
    QCOMPARE(image.pixelColor(0, 2), QColor(Qt::darkGray));
    QCOMPARE(image.pixelColor(2, 2), QColor(Qt::lightGray));
    QCOMPARE(image.pixelColor(4, 0), QColor(Qt::lightGray));
    QCOMPARE(image.pixelColor(0, 4), QColor(Qt::lightGray));
}

void KoCheckerBoardPainterContractTest::settersRebuildThePatternWithNewColorsAndSize()
{
    const QColor lightColor(241, 223, 197);
    const QColor darkColor(37, 59, 83);
    KoCheckerBoardPainter checker(1);

    checker.setCheckerColors(lightColor, darkColor);
    checker.setCheckerSize(3);
    const QImage image = renderPattern(checker, QSize(12, 12));

    QCOMPARE(image.pixelColor(0, 0), lightColor);
    QCOMPARE(image.pixelColor(2, 2), lightColor);
    QCOMPARE(image.pixelColor(3, 0), darkColor);
    QCOMPARE(image.pixelColor(0, 3), darkColor);
    QCOMPARE(image.pixelColor(3, 3), lightColor);
    QCOMPARE(image.pixelColor(6, 0), lightColor);
}

void KoCheckerBoardPainterContractTest::zeroOriginMatchesConvenienceOverload()
{
    KoCheckerBoardPainter checker(3);
    const QPointF zeroOrigin;

    const QImage convenienceResult = renderPattern(checker, QSize(12, 12));
    const QImage explicitResult = renderPattern(checker, QSize(12, 12), &zeroOrigin);

    QCOMPARE(explicitResult, convenienceResult);
}

void KoCheckerBoardPainterContractTest::patternOriginMovesTheCheckerPhase()
{
    const QColor lightColor(239, 211, 173);
    const QColor darkColor(29, 53, 79);
    KoCheckerBoardPainter checker(2);
    checker.setCheckerColors(lightColor, darkColor);
    const QPointF patternOrigin(1, 0);

    const QImage image = renderPattern(checker, QSize(8, 8), &patternOrigin);

    QCOMPARE(image.pixelColor(0, 0), darkColor);
    QCOMPARE(image.pixelColor(1, 0), lightColor);
    QCOMPARE(image.pixelColor(3, 0), darkColor);
    QCOMPARE(image.pixelColor(0, 2), lightColor);
}

void KoCheckerBoardPainterContractTest::paintingPreservesPixelsOutsideTheRectangle()
{
    const QColor background(11, 97, 71);
    KoCheckerBoardPainter checker(2);
    const QRectF paintRect(2, 3, 4, 5);

    const QImage image = renderPattern(checker, QSize(10, 10), nullptr, &paintRect, background);

    QCOMPARE(image.pixelColor(0, 0), background);
    QCOMPARE(image.pixelColor(1, 3), background);
    QCOMPARE(image.pixelColor(2, 3), QColor(Qt::lightGray));
    QCOMPARE(image.pixelColor(5, 7), QColor(Qt::darkGray));
    QCOMPARE(image.pixelColor(6, 7), background);
    QCOMPARE(image.pixelColor(2, 8), background);
}

QTEST_MAIN(KoCheckerBoardPainterContractTest)

#include "KoCheckerBoardPainterContractTest.moc"
