/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <cstring>
#include <utility>

#include <animation/kis_frame_cache_store.h>
#include <tiles/kis_tile_data_pool.h>

namespace
{
KisFrameCacheStore::Frame makeFrame(KisTileDataPoolSP pool, quint8 changedValue)
{
    KisFrameCacheStore::Frame frame;
    frame.dirtyImageRect = QRect(4, 8, 4, 4);
    frame.imageBounds = QRect(0, 0, 32, 32);
    frame.levelOfDetail = 1;
    frame.data.pixelSize = 4;

    KisFrameDataSerializer::FrameTile tile(pool);
    tile.col = 1;
    tile.row = 2;
    tile.rect = QRect(2, 4, 4, 4);
    tile.data.allocate(frame.data.pixelSize);
    std::memset(tile.data.data(), 17,
                tile.rect.width() * tile.rect.height() * frame.data.pixelSize);
    tile.data.data()[12] = changedValue;
    frame.data.frameTiles.push_back(std::move(tile));

    return frame;
}
}

class KisFrameCacheStoreTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storesFullAndDifferenceFramesWithoutUiTypes();
};

void KisFrameCacheStoreTest::storesFullAndDifferenceFramesWithoutUiTypes()
{
    KisTileDataPoolRegistry registry;
    KisTileDataPoolSP pool = registry.poolForTileSize(4, 4);
    KisFrameCacheStore store;

    store.saveFrame(10, makeFrame(pool, 21));
    store.saveFrame(20, makeFrame(pool, 29));

    QVERIFY(store.hasFrame(10));
    QVERIFY(store.hasFrame(20));
    QCOMPARE(store.frameDirtyRect(20), QRect(4, 8, 4, 4));
    QCOMPARE(store.frameLevelOfDetail(20), 1);

    KisFrameCacheStore::Frame loaded = store.loadFrame(20, pool);
    QVERIFY(loaded.isValid());
    QCOMPARE(loaded.imageBounds, QRect(0, 0, 32, 32));
    QCOMPARE(loaded.data.frameTiles.size(), size_t(1));
    QCOMPARE(loaded.data.frameTiles.front().data.data()[12], quint8(29));

    store.moveFrame(20, 25);
    QVERIFY(!store.hasFrame(20));
    QVERIFY(store.hasFrame(25));
    store.forgetFrame(25);
    QVERIFY(!store.hasFrame(25));
}

QTEST_GUILESS_MAIN(KisFrameCacheStoreTest)

#include "kis_frame_cache_store_test.moc"
