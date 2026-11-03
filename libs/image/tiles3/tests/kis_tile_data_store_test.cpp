/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tile_data_store_test.h"
#include <simpletest.h>


#include "kis_image_config.h"

#include "tiles3/kis_tiled_data_manager.h"
#include "tiles_test_utils.h"

#include "tiles3/kis_tile_data_store.h"
#include "tiles3/kis_tile_data_store_iterators.h"
#include <QScopeGuard>
#include <QSemaphore>
#include <atomic>
#include <thread>
#include <cstring>

void KisTileDataStoreTest::testAllocationPreservesPixelsDuringDocumentMemoryRelease_data()
{
    QTest::addColumn<bool>("duplicate");
    QTest::newRow("new") << false;
    QTest::newRow("copy") << true;
}

void KisTileDataStoreTest::testAllocationPreservesPixelsDuringDocumentMemoryRelease()
{
    QFETCH(bool, duplicate);
    auto *store = KisTileDataStore::instance();
    store->testingSuspendPooler();
    const auto resumePooler = qScopeGuard([&] { store->testingResumePooler(); });
    const auto pixel = QByteArray::fromHex("33221149");
    const auto expected = pixel.repeated(KisTileData::WIDTH * KisTileData::HEIGHT);
    KisTileData *source = store->allocTileData(4, reinterpret_cast<const quint8 *>(pixel.constData()));
    source->acquire();
    const auto releaseSource = qScopeGuard([&] { source->release(); });
    QSemaphore start;
    std::atomic<bool> complete{false};
    std::atomic<bool> preserved{true};
    std::thread producer([&] {
        start.acquire();
        for (int iteration = 0; iteration < 2000; ++iteration) {
            auto *tile = duplicate ? store->duplicateTileData(source)
                : store->allocTileData(4, reinterpret_cast<const quint8 *>(pixel.constData()));
            tile->acquire();
            tile->blockSwapping();
            if (std::memcmp(tile->data(), expected.constData(), expected.size()) != 0) preserved = false;
            tile->unblockSwapping();
            tile->release();
            if (!preserved) break;
        }
        complete = true;
    });
    start.release();
    for (int iteration = 0; iteration < 2000 && !complete; ++iteration) {
        KisTiledDataManager::releaseInternalPools();
        std::this_thread::yield();
    }
    producer.join();
    QVERIFY(preserved.load());
    source->blockSwapping();
    const QByteArray original(reinterpret_cast<const char *>(source->data()), expected.size());
    source->unblockSwapping();
    QCOMPARE(original, expected);
}


void KisTileDataStoreTest::testClockIterator()
{
    KisTileDataStore *store = KisTileDataStore::instance();
    store->debugClear();

    const qint32 pixelSize = 1;
    quint8 defaultPixel = 128;

    QList<KisTileData*> tileDataList;
    KisTileData *item;

    item = new KisTileData(pixelSize, &defaultPixel, store, false);
    store->registerTileData(item);
    tileDataList.append(item);
    item = new KisTileData(pixelSize, &defaultPixel, store, false);
    store->registerTileData(item);
    tileDataList.append(item);
    item = new KisTileData(pixelSize, &defaultPixel, store, false);
    store->registerTileData(item);
    tileDataList.append(item);


    const auto releaseTiles = qScopeGuard([&] {
        for (auto *tile : tileDataList) store->freeTileData(tile);
    });
    KisTileDataStoreClockIterator *iter = nullptr;
    const auto finishIteration = qScopeGuard([&] {
        if (iter) store->endIteration(iter);
    });

    iter = store->beginClockIteration();
    QList<KisTileData *> cycle;
    while (iter->hasNext()) {
        item = iter->next();
        QVERIFY(tileDataList.contains(item));
        QVERIFY(!cycle.contains(item));
        cycle.append(item);
    }
    QCOMPARE(cycle.size(), tileDataList.size());
    QCOMPARE(cycle.first(), tileDataList.first());
    store->endIteration(iter);
    iter = nullptr;

    iter = store->beginClockIteration();
    QVERIFY(iter->hasNext());
    QCOMPARE(iter->next(), cycle[0]);
    QVERIFY(iter->hasNext());
    QCOMPARE(iter->peekNext(), cycle[1]);
    store->endIteration(iter);
    iter = nullptr;

    iter = store->beginClockIteration();
    for (auto *tile : {cycle[1], cycle[2], cycle[0]}) {
        QVERIFY(iter->hasNext());
        QCOMPARE(iter->next(), tile);
    }
    QVERIFY(!iter->hasNext());
    store->endIteration(iter);
    iter = nullptr;

    tileDataList.removeOne(cycle[0]);
    store->freeTileData(cycle[0]);
    iter = store->beginClockIteration();
    for (auto *tile : {cycle[1], cycle[2]}) {
        QVERIFY(iter->hasNext());
        QCOMPARE(iter->next(), tile);
    }
    QVERIFY(!iter->hasNext());
    store->endIteration(iter);
    iter = nullptr;

    tileDataList.removeOne(cycle[1]);
    store->freeTileData(cycle[1]);
    iter = store->beginClockIteration();
    QVERIFY(iter->hasNext());
    QCOMPARE(iter->next(), cycle[2]);
    QVERIFY(!iter->hasNext());

}

void KisTileDataStoreTest::testLeaks()
{
    KisTileDataStore::instance()->debugClear();

    QCOMPARE(KisTileDataStore::instance()->numTiles(), 0);

    const qint32 pixelSize = 1;
    quint8 defaultPixel = 128;
    KisTiledDataManager *dm = new KisTiledDataManager(pixelSize, &defaultPixel);

    KisTileSP tile = dm->getTile(0, 0, true);
    tile->lockForWrite();
    tile->unlockForWrite();

    tile = 0;

    delete dm;

    QCOMPARE(KisTileDataStore::instance()->numTiles(), 0);
}

#define COLUMN2COLOR(col) (col%255)

void KisTileDataStoreTest::testSwapping()
{
    KisImageConfig config(false);
    config.setMemoryHardLimitPercent(100.0 / KisImageConfig::totalRAM());
    config.setMemorySoftLimitPercent(0);

    KisTileDataStore::instance()->debugClear();



    const qint32 pixelSize = 1;
    quint8 defaultPixel = 128;
    KisTiledDataManager dm(pixelSize, &defaultPixel);

    for(qint32 col = 0; col < 1000; col++) {
        KisTileSP tile = dm.getTile(col, 0, true);
        tile->lockForWrite();

        KisTileData *td = tile->tileData();
        QVERIFY(memoryIsFilled(defaultPixel, td->data(), TILESIZE));

        memset(td->data(), COLUMN2COLOR(col), TILESIZE);
        QVERIFY(memoryIsFilled(COLUMN2COLOR(col), td->data(), TILESIZE));

        tile->unlockForWrite();
    }

    //KisTileDataStore::instance()->debugSwapAll();

    for(qint32 col = 0; col < 1000; col++) {
        KisTileSP tile = dm.getTile(col, 0, true);
        tile->lockForRead();

        KisTileData *td = tile->tileData();
        QVERIFY(memoryIsFilled(COLUMN2COLOR(col), td->data(), TILESIZE));
        tile->unlockForWrite();
    }
}

SIMPLE_TEST_MAIN(KisTileDataStoreTest)

