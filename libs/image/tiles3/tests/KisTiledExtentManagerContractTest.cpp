/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTiledExtentManager.h"
#include "kis_tile_data_interface.h"

#include <QSemaphore>
#include <QTest>

#include <thread>

namespace
{

bool recoverableAssertTriggered = false;

} // namespace

void kis_assert_recoverable(const char *, const char *, int)
{
    recoverableAssertTriggered = true;
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    recoverableAssertTriggered = true;
}

const qint32 KisTileData::WIDTH = __TILE_DATA_WIDTH;
const qint32 KisTileData::HEIGHT = __TILE_DATA_HEIGHT;

class KisTiledExtentManagerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void initialAndClearExposeEmptyExtent();
    void positiveAndNegativeTilesExpandPixelExtent();
    void duplicateCountsDelayLastRemoval();
    void removingEdgesShrinksExtent();
    void replaceUsesAllUnsortedUniqueBounds();
    void concurrentTileAdditionsPublishCompleteExtent();
};

void KisTiledExtentManagerContractTest::init()
{
    recoverableAssertTriggered = false;
}

void KisTiledExtentManagerContractTest::cleanup()
{
    QVERIFY(!recoverableAssertTriggered);
}

void KisTiledExtentManagerContractTest::initialAndClearExposeEmptyExtent()
{
    KisTiledExtentManager manager;
    QVERIFY(manager.extent().isEmpty());

    manager.notifyTileAdded(1, 2);
    QCOMPARE(manager.extent(), QRect(64, 128, 64, 64));

    manager.clear();
    QVERIFY(manager.extent().isEmpty());
}

void KisTiledExtentManagerContractTest::positiveAndNegativeTilesExpandPixelExtent()
{
    KisTiledExtentManager manager;

    manager.notifyTileAdded(2, -1);
    QCOMPARE(manager.extent(), QRect(128, -64, 64, 64));

    manager.notifyTileAdded(-2, 1);
    QCOMPARE(manager.extent(), QRect(-128, -64, 320, 192));
}

void KisTiledExtentManagerContractTest::duplicateCountsDelayLastRemoval()
{
    KisTiledExtentManager manager;

    manager.notifyTileAdded(3, 4);
    manager.notifyTileAdded(3, 4);
    manager.notifyTileRemoved(3, 4);
    QCOMPARE(manager.extent(), QRect(192, 256, 64, 64));

    manager.notifyTileRemoved(3, 4);
    QVERIFY(manager.extent().isEmpty());
}

void KisTiledExtentManagerContractTest::removingEdgesShrinksExtent()
{
    KisTiledExtentManager manager;

    manager.notifyTileAdded(-3, -2);
    manager.notifyTileAdded(0, 0);
    manager.notifyTileAdded(4, 3);
    QCOMPARE(manager.extent(), QRect(-192, -128, 512, 384));

    manager.notifyTileRemoved(-3, -2);
    QCOMPARE(manager.extent(), QRect(0, 0, 320, 256));

    manager.notifyTileRemoved(4, 3);
    QCOMPARE(manager.extent(), QRect(0, 0, 64, 64));
}

void KisTiledExtentManagerContractTest::replaceUsesAllUnsortedUniqueBounds()
{
    KisTiledExtentManager manager;
    manager.notifyTileAdded(20, 20);

    manager.replaceTileStats({QPoint(4, -2), QPoint(-1, 3), QPoint(4, -2), QPoint(0, 1)});
    QCOMPARE(manager.extent(), QRect(-64, -128, 384, 384));

    manager.replaceTileStats({QPoint(1, 2)});
    QCOMPARE(manager.extent(), QRect(64, 128, 64, 64));
}

void KisTiledExtentManagerContractTest::concurrentTileAdditionsPublishCompleteExtent()
{
    constexpr int rounds = 10000;
    const QRect expected(-64, -64, 128, 128);
    QRect actual;
    int completedRounds = 0;

    for (int iteration = 0; iteration < rounds; ++iteration) {
        KisTiledExtentManager manager;
        QSemaphore ready;
        QSemaphore start;
        auto addColumn = [&](int column) {
            ready.release();
            start.acquire();
            manager.notifyTileAdded(column, -1);
            manager.notifyTileAdded(column, 0);
        };
        std::thread negativeColumn(addColumn, -1);
        std::thread zeroColumn(addColumn, 0);
        ready.acquire(2);
        start.release(2);
        negativeColumn.join();
        zeroColumn.join();

        actual = manager.extent();
        completedRounds = iteration + 1;
        if (actual != expected) {
            break;
        }
    }

    qInfo() << "concurrent extent rounds" << completedRounds << "of" << rounds
            << "actual" << actual << "expected" << expected;
    QCOMPARE(actual, expected);
    QCOMPARE(completedRounds, rounds);
}

QTEST_MAIN(KisTiledExtentManagerContractTest)

#include "KisTiledExtentManagerContractTest.moc"
