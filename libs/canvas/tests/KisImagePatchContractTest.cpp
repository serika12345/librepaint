/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_image_patch.h>

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QTest>

class KisImagePatchContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultAndConfiguredGeometryRemainStable();
    void validityTracksAssignedImage();
    void preScaleDrawsRequestedAreaAndRestoresPainterState();
};

void KisImagePatchContractTest::defaultAndConfiguredGeometryRemainStable()
{
    KisImagePatch defaultPatch;
    QVERIFY(!defaultPatch.isValid());
    QCOMPARE(defaultPatch.patchRect(), QRect());

    KisImagePatch configuredPatch(QRect(10, 20, 4, 6), 2, 2.0, 3.0);
    QVERIFY(!configuredPatch.isValid());
    QCOMPARE(configuredPatch.patchRect(), QRect(16, 54, 16, 30));
}

void KisImagePatchContractTest::validityTracksAssignedImage()
{
    KisImagePatch patch(QRect(0, 0, 2, 3), 0, 1.0, 1.0);
    QVERIFY(!patch.isValid());

    QImage image(2, 3, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(20, 80, 140, 128));
    patch.setImage(image);
    QVERIFY(patch.isValid());

    patch.setImage(QImage());
    QVERIFY(!patch.isValid());
}

void KisImagePatchContractTest::preScaleDrawsRequestedAreaAndRestoresPainterState()
{
    const QColor backgroundColor(230, 210, 30, 255);
    KisImagePatch patch(QRect(0, 0, 4, 4), 0, 1.0, 1.0);
    QImage source(4, 4, QImage::Format_ARGB32_Premultiplied);
    source.fill(Qt::transparent);
    for (int y = 0; y < source.height(); ++y) {
        source.setPixelColor(2, y, QColor(20, 70 + 20 * y, 160));
        source.setPixelColor(3, y, QColor(180, 30 + 20 * y, 40));
    }
    patch.setImage(source);

    const QRectF destinationRect(1.0, 1.0, 7.0, 5.0);
    const QImage expectedImage = source.scaled(QSize(7, 5), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    patch.preScale(destinationRect);
    QVERIFY(patch.isValid());
    QCOMPARE(patch.patchRect(), QRect(0, 0, 4, 4));

    QImage destination(10, 8, QImage::Format_ARGB32_Premultiplied);
    destination.fill(backgroundColor);
    QPainter painter(&destination);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QPainter::CompositionMode originalCompositionMode = painter.compositionMode();
    const QPainter::RenderHints originalRenderHints = painter.renderHints();

    patch.drawMe(painter, destinationRect, QPainter::SmoothPixmapTransform);

    QCOMPARE(painter.compositionMode(), originalCompositionMode);
    QCOMPARE(painter.renderHints(), originalRenderHints);
    painter.end();

    QCOMPARE(destination.copy(QRect(1, 1, 7, 5)), expectedImage);
    QCOMPARE(destination.pixelColor(0, 0), backgroundColor);
    QCOMPARE(destination.pixelColor(8, 6), backgroundColor);
}

QTEST_GUILESS_MAIN(KisImagePatchContractTest)

#include "KisImagePatchContractTest.moc"
