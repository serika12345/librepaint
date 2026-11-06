/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QImage>
#include <QPainter>
#include <QElapsedTimer>
#include <QTest>
#include <cmath>
#include <kis_quick_shape.h>
#include "tool/KisQuickShapePreview.h"

namespace {
QImage sample()
{
    QImage image(80, 16, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.fillRect(QRect(4, 4, 72, 8), QColor(200, 40, 90, 32));
    return image;
}
QImage render(KisQuickShapePreview &preview, const QTransform &transform = QTransform())
{
    QImage image(512, 512, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    preview.paint(painter, transform, QRect(0, 0, 512, 512));
    return image;
}
}
class KisQuickShapePreviewContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void endpointFollowsImmediatelyUsingOneCapturedSample();
    void lengthChangesKeepWidthColorAndOpacity();
    void ellipseChangesKeepOpacityWithoutOverlappingSegments();
    void cancellationAndViewTransformRestorePainterState();
    void largeShapeDisplayHasBoundedWork();
    void largeFigureRasterStaysBounded();
    void fractionalViewTransformSmoothsEdges();
};
void KisQuickShapePreviewContractTest::endpointFollowsImmediatelyUsingOneCapturedSample()
{
    KisQuickShapePreview preview;
    preview.setStrokeSample(sample(), QLineF(8, 8, 72, 8), 1.0);
    KisQuickShape shape = KisQuickShape::recognize({QPointF(100, 100), QPointF(300, 100)});
    preview.update(shape);
    QCOMPARE(render(preview).pixelColor(250, 100).alpha(), 32);
    shape.setLineEnd(QPointF(100, 300));
    const QRectF dirty = preview.update(shape);
    QVERIFY(dirty.contains(QPointF(250, 100)));
    QVERIFY(dirty.contains(QPointF(100, 250)));
    const QImage changed = render(preview);
    QCOMPARE(changed.pixelColor(250, 100).alpha(), 0);
    QCOMPARE(changed.pixelColor(100, 250).alpha(), 32);
    QCOMPARE(preview.path().currentPosition(), QPointF(100, 300));
}
void KisQuickShapePreviewContractTest::lengthChangesKeepWidthColorAndOpacity()
{
    KisQuickShapePreview preview;
    preview.setStrokeSample(sample(), QLineF(8, 8, 72, 8), 1.0);
    KisQuickShape shape = KisQuickShape::recognize({QPointF(100, 100), QPointF(300, 100)});
    int width = -1;
    for (int end : {250, 400, 200}) {
        shape.setLineEnd(QPointF(end, 100));
        preview.update(shape);
        const QImage frame = render(preview);
        const QColor color = frame.pixelColor(150, 100);
        QCOMPARE(color.alpha(), 32);
        QVERIFY(std::abs(color.red() - 200) <= 4);
        QVERIFY(std::abs(color.blue() - 90) <= 4);
        int currentWidth = 0;
        for (int y = 70; y < 130; ++y) currentWidth += frame.pixelColor(150, y).alpha() > 0;
        if (width < 0) width = currentWidth;
        QCOMPARE(currentWidth, width);
    }
}
void KisQuickShapePreviewContractTest::ellipseChangesKeepOpacityWithoutOverlappingSegments()
{
    QVector<QPointF> points;
    for (int i = 0; i <= 100; ++i) {
        const qreal angle = i * 2 * M_PI / 100;
        points.append(QPointF(250 + 100 * std::cos(angle), 250 + 60 * std::sin(angle)));
    }
    KisQuickShape shape = KisQuickShape::recognize(points);
    QCOMPARE(shape.type(), KisQuickShape::Ellipse);
    KisQuickShapePreview preview;
    preview.setStrokeSample(sample(), QLineF(8, 8, 72, 8), 1.0);
    QImage previous;
    for (int i = 0; i < 4; ++i) {
        shape.setScale(1.0 + i * 0.1);
        shape.setRotation(i * 0.3);
        shape.setSnappedToCircle(i == 3);
        preview.update(shape);
        const QImage frame = render(preview);
        QVERIFY(frame != previous);
        int painted = 0;
        for (int y = 0; y < frame.height(); ++y) for (int x = 0; x < frame.width(); ++x) {
            const int alpha = frame.pixelColor(x, y).alpha();
            QVERIFY2(alpha <= 32, "Texture segments must replace, rather than accumulate opacity at their joins");
            painted += alpha > 0;
        }
        QVERIFY(painted > 500);
        for (int point = 0; point < 200; ++point) {
            const QPointF position = preview.path().pointAtPercent(point / 200.0);
            QVERIFY2(frame.pixelColor(qRound(position.x()), qRound(position.y())).alpha() >= 28,
                     "The brush must remain continuous across mesh and texture joins");
        }
        if (i == 2 && qEnvironmentVariableIsSet("LIBREPAINT_PREVIEW_ARTIFACT")) {
            QImage displayed(frame.size(), frame.format());
            displayed.fill(Qt::white);
            QPainter painter(&displayed);
            painter.drawImage(QPoint(), frame);
            painter.end();
            QVERIFY(displayed.save(qEnvironmentVariable("LIBREPAINT_PREVIEW_ARTIFACT")));
        }
        previous = frame;
    }
}
void KisQuickShapePreviewContractTest::cancellationAndViewTransformRestorePainterState()
{
    KisQuickShapePreview preview;
    preview.setStrokeSample(sample(), QLineF(8, 8, 72, 8), 1.0);
    preview.update(KisQuickShape::recognize({QPointF(50, 50), QPointF(150, 50)}));
    QImage frame(512, 512, QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::transparent);
    QPainter painter(&frame);
    const QPen pen = painter.pen();
    preview.paint(painter, QTransform::fromScale(2, 2), QRect(0, 0, 512, 512));
    QCOMPARE(painter.transform(), QTransform());
    QCOMPARE(painter.pen(), pen);
    QCOMPARE(painter.opacity(), 1.0);
    painter.end();
    QCOMPARE(frame.pixelColor(200, 100).alpha(), 32);
    QVERIFY(!preview.clear().isEmpty());
    QVERIFY(preview.path().isEmpty());
    QCOMPARE(render(preview).pixelColor(200, 100).alpha(), 0);
    QVERIFY(preview.clear().isEmpty());
}
void KisQuickShapePreviewContractTest::largeShapeDisplayHasBoundedWork()
{
    QVector<QPointF> points;
    for (int i = 0; i <= 100; ++i) {
        const qreal angle = i * 2 * M_PI / 100;
        points.append(QPointF(1750 + 950 * std::cos(angle), 1240 + 650 * std::sin(angle)));
    }
    KisQuickShape shape = KisQuickShape::recognize(points);
    KisQuickShapePreview preview;
    preview.setStrokeSample(sample(), QLineF(8, 8, 72, 8), 1.0);
    QImage frame(1718, 1200, QImage::Format_ARGB32_Premultiplied);
    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < 60; ++i) {
        shape.setRotation(i * 0.01);
        preview.update(shape);
        frame.fill(Qt::transparent);
        QPainter painter(&frame);
        preview.paint(painter, QTransform::fromScale(0.479, 0.479), QRect(0, 0, 1718, 1200));
    }
    qInfo() << "large shape preview ms/frame" << timer.elapsed() / 60.0;
}

void KisQuickShapePreviewContractTest::largeFigureRasterStaysBounded()
{
    /**
     * The preview raster covers the whole figure, so its cost would otherwise
     * grow with the on-screen area. A figure that fills a retina viewport has
     * to stay inside the raster budget, otherwise the frame rate collapses.
     */
    QImage retinaSample(197, 69, QImage::Format_ARGB32_Premultiplied);
    retinaSample.fill(Qt::transparent);
    {
        QPainter samplePainter(&retinaSample);
        samplePainter.fillRect(QRect(0, 31, 197, 7), QColor(200, 40, 90, 32));
    }
    QVector<QPointF> fullScreenPoints;
    for (int i = 0; i <= 100; ++i) {
        const qreal angle = i * 2 * M_PI / 100;
        fullScreenPoints.append(QPointF(1000 + 900 * std::cos(angle), 800 + 600 * std::sin(angle)));
    }
    KisQuickShape fullScreenShape = KisQuickShape::recognize(fullScreenPoints);
    KisQuickShapePreview preview;
    preview.setStrokeSample(retinaSample, QLineF(34, 34, 162, 34), 4.7);
    QImage frame(2360, 1640, QImage::Format_ARGB32_Premultiplied);
    frame.setDevicePixelRatio(2);
    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < 30; ++i) {
        fullScreenShape.setRotation(i * 0.002);
        preview.update(fullScreenShape);
        frame.fill(Qt::transparent);
        QPainter painter(&frame);
        preview.paint(painter, QTransform(), QRect(0, 0, 1180, 820));
    }
    qInfo() << "retina full-screen preview ms/frame" << timer.elapsed() / 30.0;

    QVERIFY2(preview.frameDevicePixelCount() <= 1500 * 1000,
             qPrintable(QStringLiteral("preview raster grew to %1 device pixels")
                            .arg(preview.frameDevicePixelCount())));
}
void KisQuickShapePreviewContractTest::fractionalViewTransformSmoothsEdges()
{
    KisQuickShapePreview preview;
    preview.setStrokeSample(sample(), QLineF(8, 8, 72, 8), 1.0);
    preview.update(KisQuickShape::recognize({QPointF(100, 100), QPointF(300, 100)}));
    const QImage frame = render(preview, QTransform::fromTranslate(0.25, 0.25));
    bool interpolated = false;
    for (int y = 85; y < 115; ++y) {
        const int alpha = frame.pixelColor(200, y).alpha();
        interpolated |= alpha > 0 && alpha < 32;
    }
    QVERIFY2(interpolated, "Fractional positioning must interpolate coverage along the body of the stroke");
    QImage hidpi(1024, 1024, QImage::Format_ARGB32_Premultiplied);
    hidpi.setDevicePixelRatio(2);
    hidpi.fill(Qt::transparent);
    QPainter painter(&hidpi);
    preview.paint(painter, QTransform::fromTranslate(0.125, 0.125), QRect(0, 0, 512, 512));
    painter.end();
    interpolated = false;
    for (int y = 170; y < 230; ++y) {
        const int alpha = hidpi.pixelColor(400, y).alpha();
        interpolated |= alpha > 0 && alpha < 32;
    }
    QVERIFY2(interpolated, "High-density displays must preserve smooth subpixel coverage");
}
QTEST_GUILESS_MAIN(KisQuickShapePreviewContractTest)
#include "KisQuickShapePreviewContractTest.moc"
