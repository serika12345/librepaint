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

QColor interpolationProjectionColor(int x, int y)
{
    return QColor(16 + 16 * x, 32 + 16 * y, 48 + 8 * x + 4 * y, 255);
}
} // namespace

class KisQPainterCanvasDrawImageContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void drawsOnlyRequestedWidgetRegion();
    void mirrorsProjectionHorizontallyWithinKnownSamplingError();
    void rotatesProjectionClockwise90DegreesWithinKnownSamplingError();
    void rotatesProjection17Point3DegreesWithinInterpolationLimits();
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

void KisQPainterCanvasDrawImageContractTest::rotatesProjectionClockwise90DegreesWithinKnownSamplingError()
{
    constexpr int width = 8;
    constexpr int height = 8;

    KisImageSP image = new KisImage(nullptr,
                                    width,
                                    height,
                                    KoColorSpaceRegistry::instance()->rgb8(),
                                    QStringLiteral("QPainter rotation contract"));
    image->setResolution(1.0, 1.0);

    KisCoordinatesConverter converter;
    converter.setResolution(1.0, 1.0);
    converter.setImage(image);
    converter.setZoom(1.0);
    converter.setDocumentOffset(QPoint(0, 0));
    converter.setCanvasWidgetSize(QSize(width, height));
    converter.rotate(converter.makeWidgetStillPoint(converter.imageCenterInWidgetPixel()), 90.0);

    QCOMPARE(converter.viewportToWidget(QPointF(0.0, 0.0)), QPointF(width, 0.0));
    QCOMPARE(converter.viewportToWidget(QPointF(width, 0.0)), QPointF(width, height));
    QCOMPARE(converter.viewportToWidget(QPointF(width, height)), QPointF(0.0, height));
    QCOMPARE(converter.viewportToWidget(QPointF(0.0, height)), QPointF(0.0, 0.0));

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

    if (destination.pixelColor(0, 0) != projectionColor(0, width - 1)) {
        QEXPECT_FAIL("", "SmoothPixmapTransform samples one pixel inward after 90-degree rotation", Continue);
        QCOMPARE(destination.pixelColor(0, 0), projectionColor(0, width - 1));
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const QColor actual = destination.pixelColor(x, y);
            const QColor expected = projectionColor(y, width - 1 - x);
            QCOMPARE(actual.alpha(), expected.alpha());
            QVERIFY(qAbs(actual.red() - expected.red()) <= 1);
            QVERIFY(qAbs(actual.green() - expected.green()) <= 1);
            QVERIFY(qAbs(actual.blue() - expected.blue()) <= 2);
        }
    }
}

void KisQPainterCanvasDrawImageContractTest::rotatesProjection17Point3DegreesWithinInterpolationLimits()
{
    constexpr int patchSize = 8;
    constexpr int canvasSize = 16;

    KisImageSP image = new KisImage(nullptr,
                                    canvasSize,
                                    canvasSize,
                                    KoColorSpaceRegistry::instance()->rgb8(),
                                    QStringLiteral("QPainter arbitrary rotation contract"));
    image->setResolution(1.0, 1.0);

    KisCoordinatesConverter converter;
    converter.setResolution(1.0, 1.0);
    converter.setImage(image);
    converter.setZoom(1.0);
    converter.setCanvasWidgetSize(QSize(canvasSize, canvasSize));
    converter.setDocumentOffset(QPoint(0, 0));
    converter.rotate(converter.makeWidgetStillPoint(converter.imageCenterInWidgetPixel()), 17.3);

    const QRectF fullViewportRect = converter.widgetToViewport(QRectF(0.0, 0.0, canvasSize, canvasSize));
    const QPointF viewportCenter = converter.widgetToViewport(QPointF(canvasSize / 2.0, canvasSize / 2.0));
    const QPoint patchOffset(qRound(viewportCenter.x() - patchSize / 2.0),
                             qRound(viewportCenter.y() - patchSize / 2.0));
    const QPolygonF viewportPolygon{QPointF(patchOffset),
                                    QPointF(patchOffset + QPoint(patchSize, 0)),
                                    QPointF(patchOffset + QPoint(patchSize, patchSize)),
                                    QPointF(patchOffset + QPoint(0, patchSize))};
    const QPolygonF widgetPolygon = converter.viewportToWidget(viewportPolygon);

    QImage projection(fullViewportRect.toAlignedRect().size(), QImage::Format_ARGB32_Premultiplied);
    projection.fill(Qt::transparent);
    for (int y = 0; y < patchSize; ++y) {
        for (int x = 0; x < patchSize; ++x) {
            projection.setPixelColor(patchOffset.x() + x, patchOffset.y() + y, interpolationProjectionColor(x, y));
        }
    }

    QImage destination(canvasSize, canvasSize, QImage::Format_ARGB32_Premultiplied);
    destination.fill(Qt::transparent);
    QPainter painter(&destination);
    KisQPainterCanvasImage::draw(painter, converter, projection, destination.rect());
    painter.end();

    int transparentPixels = 0;
    int partiallyTransparentPixels = 0;
    int opaquePixels = 0;
    int minimumPartialAlpha = 255;
    int maximumPartialAlpha = 0;
    QRect nonTransparentBounds;
    qreal maximumOpaqueXError = 0.0;
    qreal maximumOpaqueYError = 0.0;
    qreal maximumOpaqueBlueError = 0.0;

    for (int y = 0; y < canvasSize; ++y) {
        for (int x = 0; x < canvasSize; ++x) {
            const QColor actual = destination.pixelColor(x, y);
            if (actual.alpha() == 0) {
                ++transparentPixels;
                continue;
            }

            nonTransparentBounds |= QRect(x, y, 1, 1);
            if (actual.alpha() < 255) {
                ++partiallyTransparentPixels;
                minimumPartialAlpha = qMin(minimumPartialAlpha, actual.alpha());
                maximumPartialAlpha = qMax(maximumPartialAlpha, actual.alpha());
                continue;
            }

            ++opaquePixels;
            const QPointF expectedSource =
                converter.widgetToViewport(QPointF(x + 0.5, y + 0.5)) - QPointF(patchOffset) - QPointF(0.5, 0.5);
            const qreal encodedSourceX = (actual.red() - 16) / 16.0;
            const qreal encodedSourceY = (actual.green() - 32) / 16.0;
            maximumOpaqueXError = qMax(maximumOpaqueXError, qAbs(encodedSourceX - expectedSource.x()));
            maximumOpaqueYError = qMax(maximumOpaqueYError, qAbs(encodedSourceY - expectedSource.y()));
            const qreal expectedBlue = 48.0 + 8.0 * expectedSource.x() + 4.0 * expectedSource.y();
            maximumOpaqueBlueError = qMax(maximumOpaqueBlueError, qAbs(actual.blue() - expectedBlue));
        }
    }

    QCOMPARE(fullViewportRect.toAlignedRect(), QRect(0, 0, 21, 21));
    QCOMPARE(projection.size(), QSize(21, 21));
    QCOMPARE(patchOffset, QPoint(6, 6));
    QCOMPARE(widgetPolygon.boundingRect().toAlignedRect(), QRect(2, 2, 11, 11));
    QCOMPARE(nonTransparentBounds, QRect(3, 3, 10, 10));
    QVERIFY(widgetPolygon.boundingRect().toAlignedRect().contains(nonTransparentBounds));

    QCOMPARE(transparentPixels + partiallyTransparentPixels + opaquePixels, canvasSize * canvasSize);
    QVERIFY(transparentPixels >= 168 && transparentPixels <= 184);
    QVERIFY(partiallyTransparentPixels >= 20 && partiallyTransparentPixels <= 36);
    QVERIFY(opaquePixels >= 48 && opaquePixels <= 64);
    QVERIFY(minimumPartialAlpha > 0 && minimumPartialAlpha <= 32);
    QVERIFY(maximumPartialAlpha >= 160 && maximumPartialAlpha < 255);

    QVERIFY(maximumOpaqueXError <= 0.05);
    QVERIFY(maximumOpaqueYError <= 0.05);
    QVERIFY(maximumOpaqueBlueError <= 1.1);
}

QTEST_MAIN(KisQPainterCanvasDrawImageContractTest)

#include "KisQPainterCanvasDrawImageContractTest.moc"
