/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/kis_tile.h"

#include <QTest>

#include <type_traits>

class KisTileSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tileOwnershipAndLifetimeSchemaRemainsStable();
    void tileManagerAttachmentAndDiagnosticsSignaturesRemainStable();
    void tileReadWriteLockSignaturesRemainStable();
    void tileDataAndGeometrySignaturesRemainStable();
    void tileLinkedTraversalSignaturesRemainStable();
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

QTEST_GUILESS_MAIN(KisTileSchemaContractTest)

#include "KisTileSchemaContractTest.moc"
