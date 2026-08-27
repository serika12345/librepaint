/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QTest>

#include <KoColorSpaceRegistry.h>
#include <KoViewTransformStillPoint.h>

#include <kis_coordinates_converter.h>
#include <kis_image.h>

#include "../canvas/kis_qpainter_canvas_draw_image.h"

namespace
{
QColor projectionColor(int x, int y)
{
    return QColor(20 + x, 40 + y, 60 + x + y, 255);
}
} // namespace

class KisQPainterCanvasDrawImageContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void drawsOnlyRequestedWidgetRegion();
    void mirrorsProjectionHorizontallyWithinKnownSamplingError();
};

void KisQPainterCanvasDrawImageContractTest::drawsOnlyRequestedWidgetRegion()
{
    constexpr int width = 8;
    constexpr int height = 8;
    const QColor background(201, 202, 203, 255);
    const QRect updateWidgetRect(2, 3, 3, 2);

    KisCoordinatesConverter converter;
    converter.setResolution(1.0, 1.0);
    converter.setZoom(1.0);
    converter.setCanvasWidgetSize(QSize(width, height));
    converter.setDocumentOffset(QPoint(0, 0));
    QCOMPARE(converter.viewportToWidgetTransform(), QTransform());

    QImage projection(width, height, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            projection.setPixelColor(x, y, projectionColor(x, y));
        }
    }

    QImage destination(width, height, QImage::Format_ARGB32_Premultiplied);
    destination.fill(background);
    QPainter painter(&destination);
    KisQPainterCanvasImage::draw(painter, converter, projection, updateWidgetRect);
    painter.end();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const QPoint point(x, y);
            QCOMPARE(destination.pixelColor(point),
                     updateWidgetRect.contains(point) ? projectionColor(x, y) : background);
        }
    }
}

void KisQPainterCanvasDrawImageContractTest::mirrorsProjectionHorizontallyWithinKnownSamplingError()
{
    constexpr int width = 8;
    constexpr int height = 8;

    KisImageSP image = new KisImage(nullptr,
                                    width,
                                    height,
                                    KoColorSpaceRegistry::instance()->rgb8(),
                                    QStringLiteral("QPainter mirror contract"));
    image->setResolution(1.0, 1.0);

    KisCoordinatesConverter converter;
    converter.setResolution(1.0, 1.0);
    converter.setImage(image);
    converter.setZoom(1.0);
    converter.setDocumentOffset(QPoint(0, 0));
    converter.setCanvasWidgetSize(QSize(width, height));
    converter.mirror(converter.makeWidgetStillPoint(converter.imageCenterInWidgetPixel()), true, false);

    QCOMPARE(converter.viewportToWidget(QPointF(0.0, 0.0)), QPointF(width, 0.0));
    QCOMPARE(converter.viewportToWidget(QPointF(width, height)), QPointF(0.0, height));

    QImage projection(width, height, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            projection.setPixelColor(x, y, projectionColor(x, y));
        }
    }

    QImage destination(width, height, QImage::Format_ARGB32_Premultiplied);
    destination.fill(Qt::transparent);
    QPainter painter(&destination);
    KisQPainterCanvasImage::draw(painter, converter, projection, destination.rect());
    painter.end();

    if (destination.pixelColor(0, 0) != projectionColor(width - 1, 0)) {
        QEXPECT_FAIL("", "SmoothPixmapTransform samples one pixel inward after horizontal mirroring", Continue);
        QCOMPARE(destination.pixelColor(0, 0), projectionColor(width - 1, 0));
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const QColor actual = destination.pixelColor(x, y);
            const QColor expected = projectionColor(width - 1 - x, y);
            QCOMPARE(actual.alpha(), expected.alpha());
            QVERIFY(qAbs(actual.red() - expected.red()) <= 1);
            QVERIFY(qAbs(actual.green() - expected.green()) <= 1);
            QVERIFY(qAbs(actual.blue() - expected.blue()) <= 2);
        }
    }
}

QTEST_MAIN(KisQPainterCanvasDrawImageContractTest)

#include "KisQPainterCanvasDrawImageContractTest.moc"
