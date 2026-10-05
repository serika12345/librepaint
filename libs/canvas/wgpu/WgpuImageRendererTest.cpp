/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "WgpuImageRenderer.h"
#include <QTest>

using Krita::Canvas::WgpuImageRenderer;

namespace
{
// Position and alpha distinguish row alignment, channel order and orientation.
QImage fixedImage()
{
    QImage image(67, 39, QImage::Format_RGBA8888);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            image.setPixelColor(x, y, QColor(x * 3, y * 5, (x + y) * 2, 80 + x + y));
        }
    }
    return image;
}
}

class WgpuImageRendererTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void rendersFixedImage();
    void transfersOnlyDirtyPixels();
    void rejectsInvalidUpdateAndRetainsImage();
};

void WgpuImageRendererTest::rendersFixedImage()
{
    WgpuImageRenderer renderer;
    QVERIFY2(renderer.initialize(), qPrintable(renderer.error()));
#ifdef Q_OS_DARWIN
    QCOMPARE(renderer.backend(), WGPUBackendType_Metal);
#else
    QCOMPARE(renderer.backend(), WGPUBackendType_Vulkan);
#endif
    const QImage image = fixedImage();
    QVERIFY2(renderer.upload(image, image.rect()), qPrintable(renderer.error()));
    QCOMPARE(renderer.readback(), image);
    QCOMPARE(renderer.uploadedBytes(), quint64(image.width() * image.height() * 4));
}

void WgpuImageRendererTest::transfersOnlyDirtyPixels()
{
    WgpuImageRenderer renderer;
    QVERIFY2(renderer.initialize(), qPrintable(renderer.error()));
    QImage expected = fixedImage();
    QVERIFY(renderer.upload(expected, expected.rect()));
    QImage update(expected.size(), expected.format());
    update.fill(QColor(230, 40, 10, 190));
    const QRect dirty(3, 7, 11, 9);
    const quint64 before = renderer.uploadedBytes();
    QVERIFY2(renderer.upload(update, dirty), qPrintable(renderer.error()));
    for (int y = dirty.top(); y <= dirty.bottom(); ++y) {
        for (int x = dirty.left(); x <= dirty.right(); ++x) {
            expected.setPixelColor(x, y, update.pixelColor(x, y));
        }
    }
    QCOMPARE(renderer.readback(), expected);
    QCOMPARE(renderer.uploadedBytes() - before, quint64(dirty.width() * dirty.height() * 4));
}

void WgpuImageRendererTest::rejectsInvalidUpdateAndRetainsImage()
{
    WgpuImageRenderer renderer;
    QVERIFY2(renderer.initialize(), qPrintable(renderer.error()));
    const QImage image = fixedImage();
    QVERIFY(renderer.upload(image, image.rect()));
    QVERIFY(!renderer.upload({}, QRect(0, 0, 2, 2)));
    QVERIFY(!renderer.error().isEmpty());
    QVERIFY(!renderer.upload(image, QRect(-1, 0, 2, 2)));
    QVERIFY(!renderer.upload(QImage(8, 8, image.format()), QRect(0, 0, 2, 2)));
    QVERIFY(renderer.upload(image, {}));
    QCOMPARE(renderer.readback(), image);
}

QTEST_GUILESS_MAIN(WgpuImageRendererTest)
#include "WgpuImageRendererTest.moc"
