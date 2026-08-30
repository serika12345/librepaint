/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/swap/kis_tile_compressor_factory.h"

#include <QTest>

namespace
{

int legacyConstructions = 0;
int legacyDestructions = 0;
int currentConstructions = 0;
int currentDestructions = 0;

void resetLifetimeObservations()
{
    legacyConstructions = 0;
    legacyDestructions = 0;
    currentConstructions = 0;
    currentDestructions = 0;
}

} // namespace

KisAbstractTileCompressor::KisAbstractTileCompressor() = default;

KisAbstractTileCompressor::~KisAbstractTileCompressor() = default;

KisLegacyTileCompressor::KisLegacyTileCompressor()
{
    ++legacyConstructions;
}

KisLegacyTileCompressor::~KisLegacyTileCompressor()
{
    ++legacyDestructions;
}

bool KisLegacyTileCompressor::writeTile(KisTileSP, KisPaintDeviceWriter &)
{
    return false;
}

bool KisLegacyTileCompressor::readTile(QIODevice *, KisTiledDataManager *)
{
    return false;
}

void KisLegacyTileCompressor::compressTileData(KisTileData *, quint8 *, qint32, qint32 &bytesWritten)
{
    bytesWritten = 0;
}

bool KisLegacyTileCompressor::decompressTileData(quint8 *, qint32, KisTileData *)
{
    return false;
}

qint32 KisLegacyTileCompressor::tileDataBufferSize(KisTileData *)
{
    return 0;
}

KisTileCompressor2::KisTileCompressor2()
    : m_compression(nullptr)
{
    ++currentConstructions;
}

KisTileCompressor2::~KisTileCompressor2()
{
    ++currentDestructions;
}

bool KisTileCompressor2::writeTile(KisTileSP, KisPaintDeviceWriter &)
{
    return false;
}

bool KisTileCompressor2::readTile(QIODevice *, KisTiledDataManager *)
{
    return false;
}

void KisTileCompressor2::compressTileData(KisTileData *, quint8 *, qint32, qint32 &bytesWritten)
{
    bytesWritten = 0;
}

bool KisTileCompressor2::decompressTileData(quint8 *, qint32, KisTileData *)
{
    return false;
}

qint32 KisTileCompressor2::tileDataBufferSize(KisTileData *)
{
    return 0;
}

class KisTileCompressorFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void versionOneCreatesDistinctOwnedLegacyCompressors();
    void versionTwoCreatesDistinctOwnedCurrentCompressors();
};

void KisTileCompressorFactoryContractTest::versionOneCreatesDistinctOwnedLegacyCompressors()
{
    resetLifetimeObservations();

    KisAbstractTileCompressorSP first = KisTileCompressorFactory::create(1);
    KisAbstractTileCompressorSP second = KisTileCompressorFactory::create(1);

    QVERIFY(first);
    QVERIFY(second);
    QVERIFY(first.data() != second.data());
    QVERIFY(dynamic_cast<KisLegacyTileCompressor *>(first.data()));
    QVERIFY(dynamic_cast<KisLegacyTileCompressor *>(second.data()));
    QCOMPARE(legacyConstructions, 2);
    QCOMPARE(currentConstructions, 0);

    KisAbstractTileCompressorSP sharedFirst = first;
    first.clear();
    QCOMPARE(legacyDestructions, 0);

    sharedFirst.clear();
    QCOMPARE(legacyDestructions, 1);

    second.clear();
    QCOMPARE(legacyDestructions, 2);
    QCOMPARE(currentDestructions, 0);
}

void KisTileCompressorFactoryContractTest::versionTwoCreatesDistinctOwnedCurrentCompressors()
{
    resetLifetimeObservations();

    KisAbstractTileCompressorSP first = KisTileCompressorFactory::create(2);
    KisAbstractTileCompressorSP second = KisTileCompressorFactory::create(2);

    QVERIFY(first);
    QVERIFY(second);
    QVERIFY(first.data() != second.data());
    QVERIFY(dynamic_cast<KisTileCompressor2 *>(first.data()));
    QVERIFY(dynamic_cast<KisTileCompressor2 *>(second.data()));
    QCOMPARE(currentConstructions, 2);
    QCOMPARE(legacyConstructions, 0);

    KisAbstractTileCompressorSP sharedFirst = first;
    first.clear();
    QCOMPARE(currentDestructions, 0);

    sharedFirst.clear();
    QCOMPARE(currentDestructions, 1);

    second.clear();
    QCOMPARE(currentDestructions, 2);
    QCOMPARE(legacyDestructions, 0);
}

QTEST_GUILESS_MAIN(KisTileCompressorFactoryContractTest)

#include "KisTileCompressorFactoryContractTest.moc"
