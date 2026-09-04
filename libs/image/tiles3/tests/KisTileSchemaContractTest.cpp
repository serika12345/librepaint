/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/kis_tile.h"
#include "tiles3/kis_tiled_data_manager.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisTileSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tileOwnershipAndLifetimeSchemaRemainsStable();
    void tileManagerAttachmentAndDiagnosticsSignaturesRemainStable();
    void tileReadWriteLockSignaturesRemainStable();
    void tileDataAndGeometrySignaturesRemainStable();
    void tileLinkedTraversalSignaturesRemainStable();
    void tiledDataManagerTypeLifetimeAndDefaultPixelSchemaRemainStable();
    void tiledDataManagerTileAndHistorySignaturesRemainStable();
    void tiledDataManagerRegionAndGeometrySignaturesRemainStable();
    void tiledDataManagerTransferSignaturesRemainStable();
    void tiledDataManagerMaintenanceSignaturesRemainStable();
};

void KisTileSchemaContractTest::tileOwnershipAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_same_v<KisTileSP, KisSharedPtr<KisTile>>);
    static_assert(std::is_base_of_v<KisShared, KisTile>);
    static_assert(std::is_constructible_v<KisTile, qint32, qint32, KisTileData *, KisMementoManager *>);
    static_assert(std::is_constructible_v<KisTile, const KisTile &, qint32, qint32, KisMementoManager *>);
    static_assert(std::is_constructible_v<KisTile, const KisTile &, KisMementoManager *>);
    static_assert(std::is_copy_constructible_v<KisTile>);
    static_assert(std::is_destructible_v<KisTile>);
}

void KisTileSchemaContractTest::tileManagerAttachmentAndDiagnosticsSignaturesRemainStable()
{
    using ManagerSignature = void (KisTile::*)(KisMementoManager *);
    using VoidSignature = void (KisTile::*)();

    static_assert(std::is_same_v<decltype(&KisTile::notifyAttachedToDataManager), ManagerSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::notifyDeadWithoutDetaching), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::notifyDetachedFromDataManager), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::debugDumpTile), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::debugPrintInfo), VoidSignature>);
}

void KisTileSchemaContractTest::tileReadWriteLockSignaturesRemainStable()
{
    using ConstLockSignature = void (KisTile::*)() const;
    using LockSignature = void (KisTile::*)();

    static_assert(std::is_same_v<decltype(&KisTile::lockForRead), ConstLockSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::lockForWrite), LockSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::unlockForRead), ConstLockSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::unlockForWrite), LockSignature>);
}

void KisTileSchemaContractTest::tileDataAndGeometrySignaturesRemainStable()
{
    using DataSignature = quint8 *(KisTile::*)() const;
    using SetDataSignature = void (KisTile::*)(const quint8 *);
    using CoordinateSignature = qint32 (KisTile::*)() const;
    using ExtentSignature = QRect (KisTile::*)() const;
    using TileDataSignature = KisTileData *(KisTile::*)() const;

    static_assert(std::is_same_v<decltype(&KisTile::data), DataSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::setData), SetDataSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::row), CoordinateSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::col), CoordinateSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::extent), ExtentSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::pixelSize), CoordinateSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::tileData), TileDataSignature>);
}

void KisTileSchemaContractTest::tileLinkedTraversalSignaturesRemainStable()
{
    using NextSignature = KisTileSP (KisTile::*)() const;
    using SetNextSignature = void (KisTile::*)(KisTileSP);

    static_assert(std::is_same_v<decltype(&KisTile::next), NextSignature>);
    static_assert(std::is_same_v<decltype(&KisTile::setNext), SetNextSignature>);
}

void KisTileSchemaContractTest::tiledDataManagerTypeLifetimeAndDefaultPixelSchemaRemainStable()
{
    using AssignmentSignature = KisTiledDataManager &(KisTiledDataManager::*)(const KisTiledDataManager &);
    using DefaultPixelSignature = const quint8 *(KisTiledDataManager::*)() const;
    using SetDefaultPixelSignature = void (KisTiledDataManager::*)(const quint8 *);

    static_assert(std::is_same_v<KisTiledDataManagerSP, KisSharedPtr<KisTiledDataManager>>);
    static_assert(std::is_base_of_v<KisShared, KisTiledDataManager>);
    static_assert(std::is_constructible_v<KisTiledDataManager, quint32, const quint8 *>);
    static_assert(std::is_copy_constructible_v<KisTiledDataManager>);
    static_assert(std::has_virtual_destructor_v<KisTiledDataManager>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::operator=), AssignmentSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::defaultPixel), DefaultPixelSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::setDefaultPixel), SetDefaultPixelSignature>);
}

void KisTileSchemaContractTest::tiledDataManagerTileAndHistorySignaturesRemainStable()
{
    using GetTilesPairSignature = void (KisTiledDataManager::*)(qint32, qint32, bool, KisTileSP *, KisTileSP *);
    using GetTileSignature = KisTileSP (KisTiledDataManager::*)(qint32, qint32, bool);
    using GetReadOnlyTileSignature = KisTileSP (KisTiledDataManager::*)(qint32, qint32, bool &);
    using GetOldTileSignature = KisTileSP (KisTiledDataManager::*)(qint32, qint32);
    using GetOldTileExistingSignature = KisTileSP (KisTiledDataManager::*)(qint32, qint32, bool &);
    using GetMementoSignature = KisMementoSP (KisTiledDataManager::*)();
    using VoidSignature = void (KisTiledDataManager::*)();
    using MementoSignature = void (KisTiledDataManager::*)(KisMementoSP);
    using HasMementoSignature = bool (KisTiledDataManager::*)() const;

    static_assert(std::is_same_v<decltype(&KisTiledDataManager::getTilesPair), GetTilesPairSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::getTile), GetTileSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::getReadOnlyTileLazy), GetReadOnlyTileSignature>);
    static_assert(std::is_same_v<decltype(static_cast<GetOldTileSignature>(&KisTiledDataManager::getOldTile)),
                                 GetOldTileSignature>);
    static_assert(std::is_same_v<decltype(static_cast<GetOldTileExistingSignature>(&KisTiledDataManager::getOldTile)),
                                 GetOldTileExistingSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::getMemento), GetMementoSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::commit), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::rollback), MementoSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::rollforward), MementoSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::hasCurrentMemento), HasMementoSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::purgeHistory), MementoSignature>);
}

void KisTileSchemaContractTest::tiledDataManagerRegionAndGeometrySignaturesRemainStable()
{
    using ExtentSignature = QRect (KisTiledDataManager::*)() const;
    using ExtentComponentsSignature = void (KisTiledDataManager::*)(qint32 &, qint32 &, qint32 &, qint32 &) const;
    using SetExtentSignature = void (KisTiledDataManager::*)(QRect);
    using SetExtentComponentsSignature = void (KisTiledDataManager::*)(qint32, qint32, qint32, qint32);
    using RegionSignature = KisRegion (KisTiledDataManager::*)() const;
    using ClearSignature = void (KisTiledDataManager::*)();
    using ClearRectPixelSignature = void (KisTiledDataManager::*)(QRect, const quint8 *);
    using ClearRectValueSignature = void (KisTiledDataManager::*)(QRect, quint8);
    using ClearAreaPixelSignature = void (KisTiledDataManager::*)(qint32, qint32, qint32, qint32, const quint8 *);
    using ClearAreaValueSignature = void (KisTiledDataManager::*)(qint32, qint32, qint32, qint32, quint8);
    using ContiguousSignature = qint32 (KisTiledDataManager::*)(qint32, qint32, qint32) const;
    using RowStrideSignature = qint32 (KisTiledDataManager::*)(qint32, qint32) const;

    static_assert(
        std::is_same_v<decltype(static_cast<ExtentSignature>(&KisTiledDataManager::extent)), ExtentSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ExtentComponentsSignature>(&KisTiledDataManager::extent)),
                                 ExtentComponentsSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetExtentSignature>(&KisTiledDataManager::setExtent)), SetExtentSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetExtentComponentsSignature>(&KisTiledDataManager::setExtent)),
                                 SetExtentComponentsSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::region), RegionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ClearSignature>(&KisTiledDataManager::clear)), ClearSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ClearRectPixelSignature>(&KisTiledDataManager::clear)),
                                 ClearRectPixelSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ClearRectValueSignature>(&KisTiledDataManager::clear)),
                                 ClearRectValueSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ClearAreaPixelSignature>(&KisTiledDataManager::clear)),
                                 ClearAreaPixelSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ClearAreaValueSignature>(&KisTiledDataManager::clear)),
                                 ClearAreaValueSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::numContiguousColumns), ContiguousSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::numContiguousRows), ContiguousSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::rowStride), RowStrideSignature>);
}

void KisTileSchemaContractTest::tiledDataManagerTransferSignaturesRemainStable()
{
    using BitBltSignature = void (KisTiledDataManager::*)(KisTiledDataManager *, const QRect &);
    using SetPixelSignature = void (KisTiledDataManager::*)(qint32, qint32, const quint8 *);
    using ReadBytesSignature = void (KisTiledDataManager::*)(quint8 *, qint32, qint32, qint32, qint32, qint32) const;
    using WriteBytesSignature = void (KisTiledDataManager::*)(const quint8 *, qint32, qint32, qint32, qint32, qint32);
    using ReadPlanarBytesSignature =
        QVector<quint8 *> (KisTiledDataManager::*)(QVector<qint32>, qint32, qint32, qint32, qint32) const;
    using WritePlanarBytesSignature =
        void (KisTiledDataManager::*)(QVector<quint8 *>, QVector<qint32>, qint32, qint32, qint32, qint32);

    static_assert(std::is_same_v<decltype(&KisTiledDataManager::bitBlt), BitBltSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::bitBltOldData), BitBltSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::bitBltRough), BitBltSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::bitBltRoughOldData), BitBltSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::setPixel), SetPixelSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::readBytes), ReadBytesSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::writeBytes), WriteBytesSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::readPlanarBytes), ReadPlanarBytesSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::writePlanarBytes), WritePlanarBytesSignature>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisTiledDataManager &>()
                                    .readBytes(std::declval<quint8 *>(), qint32{}, qint32{}, qint32{}, qint32{})),
                       void>);
    static_assert(std::is_same_v<
                  decltype(std::declval<KisTiledDataManager &>()
                               .writeBytes(std::declval<const quint8 *>(), qint32{}, qint32{}, qint32{}, qint32{})),
                  void>);
}

void KisTileSchemaContractTest::tiledDataManagerMaintenanceSignaturesRemainStable()
{
    using ReleasePoolsSignature = void (*)();
    using DebugSignature = void (KisTiledDataManager::*)();

    static_assert(std::is_same_v<decltype(&KisTiledDataManager::releaseInternalPools), ReleasePoolsSignature>);
    static_assert(std::is_same_v<decltype(&KisTiledDataManager::debugPrintInfo), DebugSignature>);
}

QTEST_GUILESS_MAIN(KisTileSchemaContractTest)

#include "KisTileSchemaContractTest.moc"
