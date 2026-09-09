/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/kis_tile_data_pooler.h"
#include "tiles3/swap/kis_legacy_tile_compressor.h"
#include "tiles3/swap/kis_swapped_data_store.h"
#include "tiles3/swap/kis_tile_compressor_2.h"
#include "tiles3/swap/kis_tile_data_swapper.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_SIGNATURE(type, method, ...)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&type::method)), __VA_ARGS__>)

} // namespace

class KisTileStorageSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void poolerTypeConstructionAndLifetimeSchemaRemainStable();
    void poolerControlAndMetricSignaturesRemainStable();
    void swapperTypeConstructionAndLifetimeSchemaRemainStable();
    void swapperControlSignaturesRemainStable();
    void storeTypeConstructionAndLifetimeSchemaRemainStable();
    void storeTileTransferSignaturesRemainStable();
    void storeMetricAndDiagnosticSignaturesRemainStable();
    void legacyCompressorSchemaRemainsStable();
    void currentCompressorSchemaRemainsStable();
};

void KisTileStorageSchemaContractTest::poolerTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisTileDataPooler>);
    static_assert(std::is_base_of_v<QThread, KisTileDataPooler>);
    static_assert(std::is_constructible_v<KisTileDataPooler, KisTileDataStore *>);
    static_assert(std::is_constructible_v<KisTileDataPooler, KisTileDataStore *, qint32>);
    static_assert(std::has_virtual_destructor_v<KisTileDataPooler>);

    QVERIFY(true);
}

void KisTileStorageSchemaContractTest::poolerControlAndMetricSignaturesRemainStable()
{
    ASSERT_SIGNATURE(KisTileDataPooler, kick, void (KisTileDataPooler::*)());
    ASSERT_SIGNATURE(KisTileDataPooler, terminatePooler, void (KisTileDataPooler::*)());
    ASSERT_SIGNATURE(KisTileDataPooler, testingRereadConfig, void (KisTileDataPooler::*)());
    ASSERT_SIGNATURE(KisTileDataPooler, forceUpdateMemoryStats, void (KisTileDataPooler::*)());
    ASSERT_SIGNATURE(KisTileDataPooler, lastPoolMemoryMetric, qint64 (KisTileDataPooler::*)() const);
    ASSERT_SIGNATURE(KisTileDataPooler, lastRealMemoryMetric, qint64 (KisTileDataPooler::*)() const);
    ASSERT_SIGNATURE(KisTileDataPooler, lastHistoricalMemoryMetric, qint64 (KisTileDataPooler::*)() const);
}

void KisTileStorageSchemaContractTest::swapperTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisTileDataSwapper>);
    static_assert(std::is_base_of_v<QThread, KisTileDataSwapper>);
    static_assert(std::is_constructible_v<KisTileDataSwapper, KisTileDataStore *>);
    static_assert(std::has_virtual_destructor_v<KisTileDataSwapper>);

    QVERIFY(true);
}

void KisTileStorageSchemaContractTest::swapperControlSignaturesRemainStable()
{
    ASSERT_SIGNATURE(KisTileDataSwapper, kick, void (KisTileDataSwapper::*)());
    ASSERT_SIGNATURE(KisTileDataSwapper, terminateSwapper, void (KisTileDataSwapper::*)());
    ASSERT_SIGNATURE(KisTileDataSwapper, checkFreeMemory, void (KisTileDataSwapper::*)());
    ASSERT_SIGNATURE(KisTileDataSwapper, testingRereadConfig, void (KisTileDataSwapper::*)());
}

void KisTileStorageSchemaContractTest::storeTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisSwappedDataStore>);
    static_assert(std::is_default_constructible_v<KisSwappedDataStore>);
    static_assert(std::is_destructible_v<KisSwappedDataStore>);

    QVERIFY(true);
}

void KisTileStorageSchemaContractTest::storeTileTransferSignaturesRemainStable()
{
    ASSERT_SIGNATURE(KisSwappedDataStore, trySwapOutTileData, bool (KisSwappedDataStore::*)(KisTileData *));
    ASSERT_SIGNATURE(KisSwappedDataStore, swapInTileData, void (KisSwappedDataStore::*)(KisTileData *));
    ASSERT_SIGNATURE(KisSwappedDataStore, forgetTileData, void (KisSwappedDataStore::*)(KisTileData *));
}

void KisTileStorageSchemaContractTest::storeMetricAndDiagnosticSignaturesRemainStable()
{
    ASSERT_SIGNATURE(KisSwappedDataStore, numTiles, quint64 (KisSwappedDataStore::*)() const);
    ASSERT_SIGNATURE(KisSwappedDataStore, totalSwapMemoryUsed, qint64 (KisSwappedDataStore::*)() const);
    ASSERT_SIGNATURE(KisSwappedDataStore, debugStatistics, void (KisSwappedDataStore::*)());
}

void KisTileStorageSchemaContractTest::legacyCompressorSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisLegacyTileCompressor>);
    static_assert(std::is_base_of_v<KisAbstractTileCompressor, KisLegacyTileCompressor>);
    static_assert(std::is_default_constructible_v<KisLegacyTileCompressor>);
    static_assert(std::has_virtual_destructor_v<KisLegacyTileCompressor>);
    ASSERT_SIGNATURE(KisLegacyTileCompressor,
                     writeTile,
                     bool (KisLegacyTileCompressor::*)(KisTileSP, KisPaintDeviceWriter &));
    ASSERT_SIGNATURE(KisLegacyTileCompressor,
                     readTile,
                     bool (KisLegacyTileCompressor::*)(QIODevice *, KisTiledDataManager *));
    ASSERT_SIGNATURE(KisLegacyTileCompressor,
                     compressTileData,
                     void (KisLegacyTileCompressor::*)(KisTileData *, quint8 *, qint32, qint32 &));
    ASSERT_SIGNATURE(KisLegacyTileCompressor,
                     decompressTileData,
                     bool (KisLegacyTileCompressor::*)(quint8 *, qint32, KisTileData *));
    ASSERT_SIGNATURE(KisLegacyTileCompressor, tileDataBufferSize, qint32 (KisLegacyTileCompressor::*)(KisTileData *));
}

void KisTileStorageSchemaContractTest::currentCompressorSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisTileCompressor2>);
    static_assert(std::is_base_of_v<KisAbstractTileCompressor, KisTileCompressor2>);
    static_assert(std::is_default_constructible_v<KisTileCompressor2>);
    static_assert(std::has_virtual_destructor_v<KisTileCompressor2>);
    ASSERT_SIGNATURE(KisTileCompressor2, writeTile, bool (KisTileCompressor2::*)(KisTileSP, KisPaintDeviceWriter &));
    ASSERT_SIGNATURE(KisTileCompressor2, readTile, bool (KisTileCompressor2::*)(QIODevice *, KisTiledDataManager *));
    ASSERT_SIGNATURE(KisTileCompressor2,
                     compressTileData,
                     void (KisTileCompressor2::*)(KisTileData *, quint8 *, qint32, qint32 &));
    ASSERT_SIGNATURE(KisTileCompressor2,
                     decompressTileData,
                     bool (KisTileCompressor2::*)(quint8 *, qint32, KisTileData *));
    ASSERT_SIGNATURE(KisTileCompressor2, tileDataBufferSize, qint32 (KisTileCompressor2::*)(KisTileData *));
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(KisTileStorageSchemaContractTest)

#include "KisTileStorageSchemaContractTest.moc"
