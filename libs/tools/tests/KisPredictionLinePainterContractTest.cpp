/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPredictionLinePainter.h"

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QTest>
#include <QVector>

#include <limits>

namespace
{

const int kCanvasWidth = 100;
const int kCanvasHeight = 40;

QImage makeCanvas()
{
    QImage image(kCanvasWidth, kCanvasHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    return image;
}

bool isDark(const QColor &color)
{
    return color.red() < 40 && color.green() < 40 && color.blue() < 40;
}

bool isWhite(const QColor &color)
{
    return color.red() > 230 && color.green() > 230 && color.blue() > 230;
}

bool isTestRed(const QColor &color)
{
    return color.red() > 200 && color.green() < 200 && color.blue() < 200;
}

bool isAllWhite(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (!isWhite(image.pixelColor(x, y))) {
                return false;
            }
        }
    }
    return true;
}

QVector<QPointF> threePoints()
{
    return QVector<QPointF>{QPointF(10, 20), QPointF(40, 20), QPointF(60, 20)};
}

KisPredictionLineStyle normalStyle(qreal diameter)
{
    KisPredictionLineStyle style;
    style.color = Qt::black;
    style.diameter = diameter;
    return style;
}

KisPredictionLineStyle testStyle(qreal diameter)
{
    KisPredictionLineStyle style;
    style.color = Qt::black;
    style.diameter = diameter;
    style.testColorRgb = 0xFF0000;
    return style;
}

} // namespace

class KisPredictionLinePainterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalDrawsWholePolyline();
    void testColorDrawsOnlyThePredictedPart();
    void testColorWithoutALatestIndexDrawsNothing();
    void scaledTransformScalesWidths();
    void tooFewOrNonFinitePointsDrawNothing();
    void painterStateIsRestored();
    void boundsGrowsByHalfDiameterPlusTwo();
};

void KisPredictionLinePainterContractTest::normalDrawsWholePolyline()
{
    QImage image = makeCanvas();
    QPainter gc(&image);
    KisPredictionLinePainter::paint(gc, threePoints(), 1, normalStyle(8.0));
    gc.end();

    QVERIFY(isDark(image.pixelColor(25, 20)));
    QVERIFY(isDark(image.pixelColor(50, 20)));
    QVERIFY(isWhite(image.pixelColor(25, 28)));
    QVERIFY(isWhite(image.pixelColor(50, 28)));
}

void KisPredictionLinePainterContractTest::testColorDrawsOnlyThePredictedPart()
{
    QImage image = makeCanvas();
    QPainter gc(&image);
    KisPredictionLinePainter::paint(gc, threePoints(), 1, testStyle(8.0));
    gc.end();

    QVERIFY(isTestRed(image.pixelColor(50, 20)));
    QVERIFY(isTestRed(image.pixelColor(50, 23)));
    // The overlap with the displayed line is not drawn at all.
    QVERIFY(isWhite(image.pixelColor(25, 20)));
    QVERIFY(isWhite(image.pixelColor(25, 23)));
}

void KisPredictionLinePainterContractTest::testColorWithoutALatestIndexDrawsNothing()
{
    for (const int latestIndex : {-1, 2, 5}) {
        QImage image = makeCanvas();
        QPainter gc(&image);
        KisPredictionLinePainter::paint(gc, threePoints(), latestIndex, testStyle(8.0));
        gc.end();

        QVERIFY2(isAllWhite(image), "latestIndex does not split the polyline");
    }
}

void KisPredictionLinePainterContractTest::scaledTransformScalesWidths()
{
    const QVector<QPointF> polyline{QPointF(5, 10), QPointF(20, 10), QPointF(30, 10)};

    {
        QImage image = makeCanvas();
        QPainter gc(&image);
        gc.scale(2.0, 2.0);
        KisPredictionLinePainter::paint(gc, polyline, 1, normalStyle(4.0));
        gc.end();

        QVERIFY(isDark(image.pixelColor(50, 20)));
    }

    {
        QImage image = makeCanvas();
        QPainter gc(&image);
        gc.scale(2.0, 2.0);
        KisPredictionLinePainter::paint(gc, polyline, 1, testStyle(4.0));
        gc.end();

        QVERIFY(isWhite(image.pixelColor(20, 20)));
        QVERIFY(isTestRed(image.pixelColor(50, 20)));
        QVERIFY(isTestRed(image.pixelColor(50, 23)));
    }
}

void KisPredictionLinePainterContractTest::tooFewOrNonFinitePointsDrawNothing()
{
    {
        QImage image = makeCanvas();
        QPainter gc(&image);
        KisPredictionLinePainter::paint(gc, QVector<QPointF>{QPointF(50, 20)}, 0, normalStyle(8.0));
        gc.end();
        QVERIFY(isAllWhite(image));
    }

    {
        QImage image = makeCanvas();
        QPainter gc(&image);
        KisPredictionLinePainter::paint(gc, QVector<QPointF>(), -1, normalStyle(8.0));
        gc.end();
        QVERIFY(isAllWhite(image));
    }

    {
        QImage image = makeCanvas();
        QPainter gc(&image);
        const QVector<QPointF> polyline{
            QPointF(10, 20),
            QPointF(std::numeric_limits<qreal>::quiet_NaN(), 20),
            QPointF(60, 20)};
        KisPredictionLinePainter::paint(gc, polyline, 1, normalStyle(8.0));
        gc.end();
        QVERIFY(isAllWhite(image));
    }

    {
        QImage image = makeCanvas();
        QPainter gc(&image);
        const QVector<QPointF> polyline{
            QPointF(10, 20),
            QPointF(std::numeric_limits<qreal>::infinity(), 20),
            QPointF(60, 20)};
        KisPredictionLinePainter::paint(gc, polyline, 1, testStyle(8.0));
        gc.end();
        QVERIFY(isAllWhite(image));
    }
}

void KisPredictionLinePainterContractTest::painterStateIsRestored()
{
    QImage image = makeCanvas();
    QPainter gc(&image);

    QPen pen(Qt::blue);
    pen.setWidthF(3.0);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    gc.setPen(pen);
    gc.setBrush(Qt::green);
    gc.setOpacity(0.5);
    gc.setRenderHint(QPainter::Antialiasing, false);

    const QPen penBefore = gc.pen();
    const QBrush brushBefore = gc.brush();
    const qreal opacityBefore = gc.opacity();
    const QPainter::RenderHints hintsBefore = gc.renderHints();

    KisPredictionLinePainter::paint(gc, threePoints(), 1, normalStyle(8.0));

    QCOMPARE(gc.pen(), penBefore);
    QCOMPARE(gc.brush(), brushBefore);
    QCOMPARE(gc.opacity(), opacityBefore);
    QCOMPARE(gc.renderHints(), hintsBefore);

    gc.end();
}

void KisPredictionLinePainterContractTest::boundsGrowsByHalfDiameterPlusTwo()
{
    const QVector<QPointF> polyline{QPointF(10, 20), QPointF(60, 20)};
    const QRectF rect = KisPredictionLinePainter::bounds(polyline, 8.0);

    QCOMPARE(rect, QRectF(QPointF(4, 14), QPointF(66, 26)));

    const QVector<QPointF> singlePoint{QPointF(10, 20)};
    QVERIFY(KisPredictionLinePainter::bounds(singlePoint, 8.0).isEmpty());
}

QTEST_GUILESS_MAIN(KisPredictionLinePainterContractTest)

#include "KisPredictionLinePainterContractTest.moc"
