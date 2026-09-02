/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/kis_tile_data_interface.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisTileDataSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tileDataCacheTypesAndOperationsSchemaRemainsStable();
    void tileDataIdentityStateAndLifetimeSchemaRemainsStable();
    void tileDataStorageAndReferenceSignaturesRemainStable();
    void tileDataSwapHistoryAndAgeSignaturesRemainStable();
    void tileDataMemoryLifecycleSignaturesRemainStable();
};

void KisTileDataSchemaContractTest::tileDataCacheTypesAndOperationsSchemaRemainsStable()
{
    using CacheOperationSignature = bool (SimpleCache::*)(int, quint8 *&);
    using ClearSignature = void (SimpleCache::*)();

    static_assert(std::is_same_v<KisTileDataCache, KisLocklessStack<KisTileData *>>);
    static_assert(std::is_same_v<KisTileDataList, QLinkedList<KisTileData *>>);
    static_assert(std::is_same_v<KisTileDataListConstIterator, KisTileDataList::const_iterator>);
    static_assert(std::is_same_v<KisTileDataListIterator, KisTileDataList::iterator>);
    static_assert(std::is_class_v<SimpleCache>);
    static_assert(std::is_default_constructible_v<SimpleCache>);
    static_assert(std::is_destructible_v<SimpleCache>);
    static_assert(std::is_same_v<decltype(&SimpleCache::push), CacheOperationSignature>);
    static_assert(std::is_same_v<decltype(&SimpleCache::pop), CacheOperationSignature>);
    static_assert(std::is_same_v<decltype(&SimpleCache::clear), ClearSignature>);
}

void KisTileDataSchemaContractTest::tileDataIdentityStateAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisTileData>);
    static_assert(std::is_enum_v<KisTileData::EnumTileDataState>);
    static_assert(KisTileData::NORMAL == 0);
    static_assert(KisTileData::COMPRESSED == 1);
    static_assert(KisTileData::SWAPPED == 2);
    static_assert(std::is_same_v<decltype(KisTileData::WIDTH), const qint32>);
    static_assert(std::is_same_v<decltype(KisTileData::HEIGHT), const qint32>);
    static_assert(std::is_constructible_v<KisTileData, qint32, const quint8 *, KisTileDataStore *, bool>);
    static_assert(std::is_destructible_v<KisTileData>);
}

void KisTileDataSchemaContractTest::tileDataStorageAndReferenceSignaturesRemainStable()
{
    using DataSignature = quint8 *(KisTileData::*)() const;
    using SetDataSignature = void (KisTileData::*)(const quint8 *);
    using PixelSizeSignature = quint32 (KisTileData::*)() const;
    using BooleanSignature = bool (KisTileData::*)();
    using ConstBooleanSignature = bool (KisTileData::*)() const;
    using CloneSignature = KisTileData *(KisTileData::*)();
    using UserCountSignature = qint32 (KisTileData::*)() const;

    static_assert(std::is_same_v<decltype(&KisTileData::data), DataSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::setData), SetDataSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::pixelSize), PixelSizeSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::acquire), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::release), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::ref), ConstBooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::deref), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::clone), CloneSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::numUsers), UserCountSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::historical), ConstBooleanSignature>);
}

void KisTileDataSchemaContractTest::tileDataSwapHistoryAndAgeSignaturesRemainStable()
{
    using VoidSignature = void (KisTileData::*)();
    using SwapChunkSignature = KisChunk (KisTileData::*)() const;
    using SetSwapChunkSignature = void (KisTileData::*)(KisChunk);
    using BooleanSignature = bool (KisTileData::*)() const;
    using SetBooleanSignature = void (KisTileData::*)(bool);
    using AgeSignature = int (KisTileData::*)() const;

    static_assert(std::is_same_v<decltype(&KisTileData::blockSwapping), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::unblockSwapping), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::swapChunk), SwapChunkSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::setSwapChunk), SetSwapChunkSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::mementoed), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::setMementoed), SetBooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::age), AgeSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::resetAge), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::markOld), VoidSignature>);
}

void KisTileDataSchemaContractTest::tileDataMemoryLifecycleSignaturesRemainStable()
{
    using MemorySignature = void (KisTileData::*)();
    using ReleasePoolsSignature = void (*)();

    static_assert(std::is_same_v<decltype(&KisTileData::allocateMemory), MemorySignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::releaseMemory), MemorySignature>);
    static_assert(std::is_same_v<decltype(&KisTileData::releaseInternalPools), ReleasePoolsSignature>);
}

QTEST_GUILESS_MAIN(KisTileDataSchemaContractTest)

#include "KisTileDataSchemaContractTest.moc"
