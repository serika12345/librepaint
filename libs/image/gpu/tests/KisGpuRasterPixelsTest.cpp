/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <gpu/KisGpuRasterPixels.h>
#include <limits>

using namespace LibrePaint::Image::Gpu;

class KisGpuRasterPixelsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void compressedPixelsRemainLosslessAndIndependent();
    void compressedPixelsRemainLosslessAndIndependent_data();
    void invalidTileExtentsAndByteCountsAreRejected();
    void rgba16CheckpointsPreserveLowBitsAndRejectWrongPrecision();
    void componentEncodingUsesDeclaredPrecisionAndByteOrder();
};

void KisGpuRasterPixelsTest::componentEncodingUsesDeclaredPrecisionAndByteOrder()
{
    const RgbaPixel color{1, 255, 256, 257};
    const auto bytes = QByteArray::fromHex("0100ff0000010101");
    QCOMPARE(rgbaBytes(color, PixelFormat::Rgba16), bytes);
    QCOMPARE(rgbaPixel(bytes, PixelFormat::Rgba16), std::optional<RgbaPixel>(color));
    QVERIFY(rgbaBytes(color, PixelFormat::Rgba8).isEmpty());
    const RgbaPixel byteColor{1, 255, 0, 128};
    QCOMPARE(rgbaBytes(byteColor, PixelFormat::Rgba8), QByteArray::fromHex("01ff0080"));
    QCOMPARE(rgbaPixel(QByteArray::fromHex("01ff0080"), PixelFormat::Rgba8), std::optional<RgbaPixel>(byteColor));
    QVERIFY(!rgbaPixel(bytes.chopped(1), PixelFormat::Rgba16));
    QVERIFY(!rgbaPixel(QByteArray(bytes + 'x'), PixelFormat::Rgba16));
    QVERIFY(!rgbaPixel(bytes, PixelFormat::Rgba8));
    QVERIFY(!rgbaPixel({}, static_cast<PixelFormat>(255)));
}

void KisGpuRasterPixelsTest::rgba16CheckpointsPreserveLowBitsAndRejectWrongPrecision()
{
    const auto pixelBytes = QByteArray::fromHex("0001ff0001000000feff0080ff7fffffffff00000101ff00");
    const auto bytes = pixelBytes.repeated(RgbaTile::byteCount(PixelFormat::Rgba16) / pixelBytes.size());
    // Include arbitrary low bits throughout the full tile, including its tail.
    QByteArray pixels = bytes;
    pixels.append(QByteArray(RgbaTile::byteCount(PixelFormat::Rgba16) - pixels.size(), '\x01'));
    auto input = RgbaTile{{-1, 0}, pixels, PixelFormat::Rgba16};
    const auto checkpoint = RasterTile::fromPixels(input);
    QVERIFY(checkpoint);
    QCOMPARE(checkpoint->format(), PixelFormat::Rgba16);
    QCOMPARE(checkpoint->byteCount(), pixels.size());
    QCOMPARE(checkpoint->rgba(), pixels);
    input.rgba.fill('\0');
    QCOMPARE(checkpoint->rgba(), pixels);
    QVERIFY(!RasterTile::fromPixels({input.coordinate, pixels}));
    QVERIFY(!RasterTile::fromPixels({input.coordinate, pixels.chopped(1), PixelFormat::Rgba16}));
    QVERIFY(!RasterTile::fromPixels({input.coordinate, pixels + 'x', PixelFormat::Rgba16}));
    const auto invalid = static_cast<PixelFormat>(255);
    QVERIFY(!RasterTile::fromPixels({input.coordinate, {}, invalid}));
}

void KisGpuRasterPixelsTest::compressedPixelsRemainLosslessAndIndependent()
{
    QFETCH(QByteArray, bytes);
    RgbaTile input{{2, -3}, bytes};
    const auto frozen = RasterTile::fromPixels(input);
    QVERIFY(frozen);
    QCOMPARE(frozen->coordinate(), input.coordinate);
    QCOMPARE(frozen->rgba(), bytes);
    QVERIFY(frozen->compressedByteCount() > 0);
    input.rgba.fill('\x7f');
    auto decoded = frozen->rgba();
    decoded.fill('\x3f');
    QCOMPARE(frozen->rgba(), bytes);
}

void KisGpuRasterPixelsTest::compressedPixelsRemainLosslessAndIndependent_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::newRow("transparent") << QByteArray(RgbaTile::ByteCount, '\0');
    QTest::newRow("alpha-and-invisible-color") << QByteArray::fromHex("ff0000ff00ff00800e1f3b0001020304").repeated(RgbaTile::ByteCount / 16);
    QByteArray pattern(RgbaTile::ByteCount, Qt::Uninitialized);
    for (qsizetype i = 0; i < pattern.size(); ++i) pattern[i] = char((i * 37 + i / 11) & 0xff);
    QTest::newRow("byte-pattern") << pattern;
}

void KisGpuRasterPixelsTest::invalidTileExtentsAndByteCountsAreRejected()
{
    const QByteArray bytes(RgbaTile::ByteCount, '\0');
    const int edge = RgbaTile::CoordinateLimit / RgbaTile::Size;
    QVERIFY(RasterTile::fromPixels({{-edge, -edge}, bytes}));
    QVERIFY(RasterTile::fromPixels({{edge - 1, edge - 1}, bytes}));
    for (const auto point : {QPoint(edge, 0), QPoint(0, edge), QPoint(-edge - 1, 0),
                             QPoint(0, -edge - 1), QPoint(std::numeric_limits<int>::max(), std::numeric_limits<int>::min())}) {
        QVERIFY(!RasterTile::fromPixels({point, bytes}));
    }
    QVERIFY(!RasterTile::fromPixels({{0, 0}, {}}));
    QVERIFY(!RasterTile::fromPixels({{0, 0}, bytes.chopped(1)}));
    QVERIFY(!RasterTile::fromPixels({{0, 0}, bytes + 'x'}));
}

QTEST_GUILESS_MAIN(KisGpuRasterPixelsTest)
#include "KisGpuRasterPixelsTest.moc"
