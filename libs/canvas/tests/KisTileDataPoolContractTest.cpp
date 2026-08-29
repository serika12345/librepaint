/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <tiles/kis_tile_data_buffer.h>
#include <tiles/kis_tile_data_pool.h>

#include <QSignalSpy>
#include <QTest>

#include <cstring>
#include <type_traits>
#include <utility>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected recoverable assertion: %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class KisTileDataPoolContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void poolAllocatesSizedChunksAndRequestsPurgeAtHighWatermark();
    void registrySharesPoolsByTileDimensions();
    void bufferConstructionAndAllocationExposeOwnedStorage();
    void moveAndSwapTransferCompleteBufferState();
    void bufferDestructionReleasesStorageAndPoolOwnership();
};

void KisTileDataPoolContractTest::poolAllocatesSizedChunksAndRequestsPurgeAtHighWatermark()
{
    KisTileDataPool pool(3, 2);
    QSignalSpy purgeSpy(&pool, &KisTileDataPool::purgeRequested);

    quint8 *data = pool.allocate(4);
    QVERIFY(data);
    QCOMPARE(pool.chunkSize(4), 24);
    std::memset(data, 0x6a, 24);
    QCOMPARE(data[0], quint8(0x6a));
    QCOMPARE(data[23], quint8(0x6a));
    pool.release(data, 4);
    pool.tryPurge(4, 1);
    QCOMPARE(purgeSpy.count(), 0);

    QVector<quint8 *> allocations;
    for (int i = 0; i < 65; ++i) {
        allocations.append(pool.allocate(2));
        QVERIFY(allocations.last());
    }
    QCOMPARE(pool.chunkSize(2), 12);
    for (quint8 *allocation : std::as_const(allocations)) {
        pool.release(allocation, 2);
    }

    QCOMPARE(purgeSpy.count(), 1);
    QCOMPARE(purgeSpy.at(0).at(0).toInt(), 2);
    QCOMPARE(purgeSpy.at(0).at(1).toInt(), 65);
    pool.tryPurge(2, 65);
}

void KisTileDataPoolContractTest::registrySharesPoolsByTileDimensions()
{
    static_assert(std::is_same_v<KisTileDataPoolSP, QSharedPointer<KisTileDataPool>>);

    KisTileDataPoolRegistry registry;
    const KisTileDataPoolSP first = registry.poolForTileSize(4, 5);
    const KisTileDataPoolSP same = registry.poolForTileSize(4, 5);
    const KisTileDataPoolSP transposed = registry.poolForTileSize(5, 4);

    QVERIFY(first);
    QCOMPARE(first, same);
    QVERIFY(first != transposed);

    QWeakPointer<KisTileDataPool> releasedPool;
    {
        const KisTileDataPoolSP temporary = registry.poolForTileSize(7, 8);
        releasedPool = temporary;
    }
    QVERIFY(releasedPool.isNull());
    QVERIFY(registry.poolForTileSize(7, 8));
}

void KisTileDataPoolContractTest::bufferConstructionAndAllocationExposeOwnedStorage()
{
    const KisTileDataPoolSP pool = KisTileDataPoolSP::create(4, 3);
    KisTileDataBuffer deferred(pool);

    QCOMPARE(deferred.pool(), pool);
    QVERIFY(!deferred.data());
    QCOMPARE(deferred.pixelSize(), 0);
    QCOMPARE(deferred.size(), 0);

    deferred.allocate(2);
    QVERIFY(deferred.data());
    QCOMPARE(deferred.pixelSize(), 2);
    QCOMPARE(deferred.size(), 24);
    deferred.data()[0] = 0x17;
    QCOMPARE(deferred.data()[0], quint8(0x17));

    KisTileDataBuffer immediate(4, pool);
    QCOMPARE(immediate.pool(), pool);
    QVERIFY(immediate.data());
    QCOMPARE(immediate.pixelSize(), 4);
    QCOMPARE(immediate.size(), 48);
}

void KisTileDataPoolContractTest::moveAndSwapTransferCompleteBufferState()
{
    const KisTileDataPoolSP firstPool = KisTileDataPoolSP::create(2, 2);
    const KisTileDataPoolSP secondPool = KisTileDataPoolSP::create(3, 1);

    KisTileDataBuffer source(1, firstPool);
    quint8 *sourceData = source.data();
    KisTileDataBuffer moved(std::move(source));
    QCOMPARE(moved.data(), sourceData);
    QCOMPARE(moved.pixelSize(), 1);
    QCOMPARE(moved.pool(), firstPool);
    QVERIFY(!source.data());
    QCOMPARE(source.pixelSize(), 0);
    QVERIFY(!source.pool());

    KisTileDataBuffer other(2, secondPool);
    quint8 *otherData = other.data();
    moved.swap(other);
    QCOMPARE(moved.data(), otherData);
    QCOMPARE(moved.pixelSize(), 2);
    QCOMPARE(moved.pool(), secondPool);
    QCOMPARE(moved.size(), 6);
    QCOMPARE(other.data(), sourceData);
    QCOMPARE(other.pixelSize(), 1);
    QCOMPARE(other.pool(), firstPool);
    QCOMPARE(other.size(), 4);

    KisTileDataBuffer assignmentSource(3, secondPool);
    quint8 *assignmentData = assignmentSource.data();
    KisTileDataBuffer *assignmentResult = &(other = std::move(assignmentSource));
    QCOMPARE(assignmentResult, &other);
    QCOMPARE(other.data(), assignmentData);
    QCOMPARE(other.pixelSize(), 3);
    QCOMPARE(other.pool(), secondPool);
    QCOMPARE(assignmentSource.data(), sourceData);
    QCOMPARE(assignmentSource.pixelSize(), 1);
    QCOMPARE(assignmentSource.pool(), firstPool);
}

void KisTileDataPoolContractTest::bufferDestructionReleasesStorageAndPoolOwnership()
{
    QWeakPointer<KisTileDataPool> weakPool;
    {
        KisTileDataPoolSP pool = KisTileDataPoolSP::create(2, 3);
        weakPool = pool;
        {
            KisTileDataBuffer buffer(4, pool);
            pool.clear();
            QVERIFY(!weakPool.isNull());
            QCOMPARE(buffer.size(), 24);
        }
        QVERIFY(weakPool.isNull());
    }
}

QTEST_GUILESS_MAIN(KisTileDataPoolContractTest)

#include "KisTileDataPoolContractTest.moc"
