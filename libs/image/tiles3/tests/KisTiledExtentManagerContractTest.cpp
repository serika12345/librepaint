/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTiledExtentManager.h"
#include "kis_tile_data_interface.h"

#include <QTest>

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

QTEST_MAIN(KisTiledExtentManagerContractTest)

#include "KisTiledExtentManagerContractTest.moc"
