/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisClipboardImage.h"

#include <QImage>
#include <QMimeData>
#include <QTest>

class KisClipboardImageContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void detectsAlphaImages();
    void usesNativePngForAlphaOnWindows();
    void keepsImageDataForAlphaOutsideWindows();
    void keepsImageDataForOpaqueImagesOnWindows();
    void ignoresNullImages();
};

void KisClipboardImageContractTest::detectsAlphaImages()
{
    QImage alpha(8, 8, QImage::Format_ARGB32_Premultiplied);
    alpha.fill(QColor(10, 20, 30, 128));
    QVERIFY(KisClipboardImage::needsAlphaPreservingPng(alpha));

    QImage opaque(8, 8, QImage::Format_RGB32);
    opaque.fill(QColor(10, 20, 30));
    QVERIFY(!KisClipboardImage::needsAlphaPreservingPng(opaque));

    QVERIFY(!KisClipboardImage::needsAlphaPreservingPng(QImage()));
}

void KisClipboardImageContractTest::usesNativePngForAlphaOnWindows()
{
    QImage image(8, 8, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(10, 20, 30, 128));

    QMimeData mimeData;
    KisClipboardImage::setImage(&mimeData, image, true);

    QVERIFY(mimeData.hasFormat(KisClipboardImage::pngClipboardFormat()));
    QVERIFY(mimeData.hasFormat(KisClipboardImage::pngMimeType()));
    QVERIFY(!mimeData.hasFormat(QStringLiteral("application/x-qt-image")));

    QImage decoded;
    QVERIFY(decoded.loadFromData(mimeData.data(KisClipboardImage::pngClipboardFormat()), "PNG"));
    QCOMPARE(decoded.size(), image.size());
    QVERIFY(decoded.hasAlphaChannel());
    QCOMPARE(qAlpha(decoded.pixel(0, 0)), 128);
}

void KisClipboardImageContractTest::keepsImageDataForAlphaOutsideWindows()
{
    QImage image(8, 8, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(10, 20, 30, 128));

    QMimeData mimeData;
    KisClipboardImage::setImage(&mimeData, image, false);

    QVERIFY(mimeData.hasFormat(QStringLiteral("application/x-qt-image")));
    QVERIFY(!mimeData.hasFormat(KisClipboardImage::pngClipboardFormat()));
}

void KisClipboardImageContractTest::keepsImageDataForOpaqueImagesOnWindows()
{
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(QColor(10, 20, 30));

    QMimeData mimeData;
    KisClipboardImage::setImage(&mimeData, image, true);

    QVERIFY(mimeData.hasFormat(QStringLiteral("application/x-qt-image")));
    QVERIFY(!mimeData.hasFormat(KisClipboardImage::pngClipboardFormat()));
}

void KisClipboardImageContractTest::ignoresNullImages()
{
    QMimeData mimeData;
    KisClipboardImage::setImage(&mimeData, QImage(), true);
    QVERIFY(mimeData.formats().isEmpty());
}

QTEST_GUILESS_MAIN(KisClipboardImageContractTest)

#include "KisClipboardImageContractTest.moc"
