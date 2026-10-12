/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <gpu/KisGpuRasterPixels.h>
#include <limits>

using namespace Krita::Image::Gpu;

class KisGpuRasterPixelsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void compressedPixelsRemainLosslessAndIndependent();
    void compressedPixelsRemainLosslessAndIndependent_data();
    void invalidTileExtentsAndByteCountsAreRejected();
    void corruptedOrOversizedCompressedPixelsAreRejected();
};

void KisGpuRasterPixelsTest::compressedPixelsRemainLosslessAndIndependent()
{
    QFETCH(QByteArray, bytes);
    RgbaTile input{{2, -3}, bytes};
    const auto frozen = RasterTile::fromPixels(input);
    QVERIFY(frozen);
    QCOMPARE(frozen->coordinate(), input.coordinate);
    QCOMPARE(frozen->rgba(), bytes);
    QVERIFY(!frozen->compressedData().isEmpty());
    auto encoded = frozen->compressedData();
    const auto restored = RasterTile::fromCompressed(input.coordinate, encoded);
    QVERIFY(restored);
    QCOMPARE(restored->coordinate(), input.coordinate);
    QCOMPARE(restored->rgba(), bytes);
    encoded[0] = char(encoded[0] ^ 0xff);
    input.rgba.fill('\x7f');
    auto decoded = frozen->rgba();
    decoded.fill('\x3f');
    QCOMPARE(frozen->rgba(), bytes);
    QCOMPARE(restored->rgba(), bytes);
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

void KisGpuRasterPixelsTest::corruptedOrOversizedCompressedPixelsAreRejected()
{
    const auto tile = RasterTile::fromPixels({{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')});
    QVERIFY(tile);
    const auto encoded = tile->compressedData();
    auto corrupted = encoded;
    corrupted[corrupted.size() - 1] = char(corrupted.back() ^ 1);
    const auto oversized = qCompress(QByteArray(RgbaTile::ByteCount + 1, '\0'), 6).mid(4);
    const auto undersized = qCompress(QByteArray(RgbaTile::ByteCount - 1, '\0'), 6).mid(4);
    for (const auto &invalid : QList<QByteArray>{QByteArray{}, QByteArrayLiteral("invalid"), encoded.chopped(1),
                               encoded + 'x', corrupted, oversized, undersized,
                               QByteArray(RgbaTile::ByteCount + 1024, 'x')}) {
        QVERIFY(!RasterTile::fromCompressed({0, 0}, invalid));
    }
    const int outside = RgbaTile::CoordinateLimit / RgbaTile::Size;
    QVERIFY(!RasterTile::fromCompressed({outside, 0}, encoded));
    QCOMPARE(tile->rgba(), QByteArray(RgbaTile::ByteCount, '\0'));
}

QTEST_GUILESS_MAIN(KisGpuRasterPixelsTest)
#include "KisGpuRasterPixelsTest.moc"
