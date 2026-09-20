/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_qimage_pyramid.h>

#include <QTest>

#include <cmath>
#include <memory>

namespace
{
int safeAssertCount = 0;

QImage patternedImage()
{
    QImage image(4, 4, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            image.setPixelColor(x, y, (x + y) % 2 == 0 ? Qt::black : Qt::white);
        }
    }
    return image;
}

bool imagesDiffer(const QImage &first, const QImage &second)
{
    if (first.size() != second.size() || first.format() != second.format()) {
        return true;
    }

    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            if (first.pixel(x, y) != second.pixel(x, y)) {
                return true;
            }
        }
    }
    return false;
}
} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

class KisQImagePyramidContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultAndConstructedImagesHaveIndependentLifetimes();
    void geometryTracksShapeAndSubpixelOffset();
    void closestLevelReportsScaleAndWorkaroundBorder();
    void enlargementModeChangesInterpolationWithoutChangingGeometry();
};

void KisQImagePyramidContractTest::defaultAndConstructedImagesHaveIndependentLifetimes()
{
    KisQImagePyramid emptyPyramid;
    QVERIFY(emptyPyramid.createImage(KisDabShape(), 0.0, 0.0).isNull());

    QImage source = patternedImage();
    const QImage expected = source;
    QImage generated;
    {
        auto pyramid = std::make_unique<KisQImagePyramid>(source);
        source.fill(Qt::red);
        generated = pyramid->createImage(KisDabShape(), 0.0, 0.0);
        QCOMPARE(generated, expected);
        QCOMPARE(generated.format(), QImage::Format_ARGB32);
    }

    QCOMPARE(generated, expected);
    QCOMPARE(safeAssertCount, 0);
}

void KisQImagePyramidContractTest::geometryTracksShapeAndSubpixelOffset()
{
    const QSize originalSize(40, 20);
    const KisDabShape scaledShape(2.0, 0.5, 0.0);

    QCOMPARE(KisQImagePyramid::characteristicSize(originalSize, scaledShape), QSizeF(80.0, 20.0));
    QCOMPARE(KisQImagePyramid::imageSize(originalSize, scaledShape, 0.0, 0.0), QSize(80, 20));
    QCOMPARE(KisQImagePyramid::imageSize(QSize(10, 10), KisDabShape(), 0.25, 0.5), QSize(11, 11));
    QCOMPARE(KisQImagePyramid::imageSize(QSize(150, 150), KisDabShape(1.0, 0.5, M_PI_4), 0.0, 0.0), QSize(160, 160));

    const QImage source = patternedImage();
    KisQImagePyramid pyramid(source);
    const QImage transformed = pyramid.createImage(scaledShape, 0.0, 0.0);
    QCOMPARE(transformed.size(), KisQImagePyramid::imageSize(source.size(), scaledShape, 0.0, 0.0));
    QCOMPARE(transformed.format(), QImage::Format_ARGB32);
    QCOMPARE(safeAssertCount, 0);
}

void KisQImagePyramidContractTest::closestLevelReportsScaleAndWorkaroundBorder()
{
    const QImage source = patternedImage();
    KisQImagePyramid pyramid(source);

    qreal borderedScale = -1.0;
    const QImage bordered = pyramid.getClosest(QTransform(), &borderedScale);
    QCOMPARE(borderedScale, 1.0);
    QCOMPARE(bordered.size(), source.size() + QSize(2, 2));
    QCOMPARE(bordered.pixelColor(0, 0).alpha(), 0);
    QCOMPARE(bordered.copy(1, 1, source.width(), source.height()), source);

    qreal unborderedScale = -1.0;
    const QImage unbordered = pyramid.getClosestWithoutWorkaroundBorder(QTransform(), &unborderedScale);
    QCOMPARE(unborderedScale, 1.0);
    QCOMPARE(unbordered, source);
    QCOMPARE(safeAssertCount, 0);
}

void KisQImagePyramidContractTest::enlargementModeChangesInterpolationWithoutChangingGeometry()
{
    const QImage source = patternedImage();
    const KisQImagePyramid smoothPyramid(source);
    const KisQImagePyramid fastPyramid(source, false);
    const QTransform enlargement = QTransform::fromScale(4.0, 4.0);

    qreal smoothScale = -1.0;
    qreal fastScale = -1.0;
    const QImage smooth = smoothPyramid.getClosestWithoutWorkaroundBorder(enlargement, &smoothScale);
    const QImage fast = fastPyramid.getClosestWithoutWorkaroundBorder(enlargement, &fastScale);

    QCOMPARE(smoothScale, 4.0);
    QCOMPARE(fastScale, 4.0);
    QCOMPARE(smooth.size(), QSize(16, 16));
    QCOMPARE(fast.size(), smooth.size());
    QVERIFY(imagesDiffer(smooth, fast));
    QCOMPARE(safeAssertCount, 0);
}

QTEST_GUILESS_MAIN(KisQImagePyramidContractTest)

#include "KisQImagePyramidContractTest.moc"
