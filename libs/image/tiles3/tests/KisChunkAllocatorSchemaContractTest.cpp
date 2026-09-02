/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tiles3/swap/kis_chunk_allocator.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisChunkAllocatorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void chunkContainerTypeSchemaRemainsStable();
    void chunkDataIntervalSchemaRemainsStable();
    void chunkHandleSchemaRemainsStable();
    void chunkAllocatorLifetimeAndCapacitySchemaRemainsStable();
    void chunkAllocatorOperationSchemaRemainsStable();
};

void KisChunkAllocatorSchemaContractTest::chunkContainerTypeSchemaRemainsStable()
{
    static_assert(std::is_same_v<KisChunkDataList, QLinkedList<KisChunkData>>);
    static_assert(std::is_same_v<KisChunkDataListIterator, KisChunkDataList::iterator>);
    static_assert(std::is_class_v<KisChunkData>);
    static_assert(std::is_class_v<KisChunk>);
    static_assert(std::is_class_v<KisChunkAllocator>);

    QVERIFY(true);
}

void KisChunkAllocatorSchemaContractTest::chunkDataIntervalSchemaRemainsStable()
{
    using EqualitySignature = bool (KisChunkData::*)(const KisChunkData &) const;
    using SetChunkSignature = void (KisChunkData::*)(quint64, quint64);
    using SizeSignature = quint64 (KisChunkData::*)() const;

    static_assert(std::is_same_v<decltype(KisChunkData::m_begin), quint64>);
    static_assert(std::is_same_v<decltype(KisChunkData::m_end), quint64>);
    static_assert(std::is_constructible_v<KisChunkData, quint64, quint64>);
    static_assert(
        std::is_same_v<decltype(static_cast<EqualitySignature>(&KisChunkData::operator==)), EqualitySignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetChunkSignature>(&KisChunkData::setChunk)), SetChunkSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SizeSignature>(&KisChunkData::size)), SizeSignature>);

    QVERIFY(true);
}

void KisChunkAllocatorSchemaContractTest::chunkHandleSchemaRemainsStable()
{
    using BoundarySignature = quint64 (KisChunk::*)() const;
    using PositionSignature = KisChunkDataListIterator (KisChunk::*)();
    using DataSignature = const KisChunkData &(KisChunk::*)();

    static_assert(std::is_default_constructible_v<KisChunk>);
    static_assert(std::is_constructible_v<KisChunk, KisChunkDataListIterator>);
    static_assert(std::is_same_v<decltype(static_cast<BoundarySignature>(&KisChunk::begin)), BoundarySignature>);
    static_assert(std::is_same_v<decltype(static_cast<BoundarySignature>(&KisChunk::end)), BoundarySignature>);
    static_assert(std::is_same_v<decltype(static_cast<BoundarySignature>(&KisChunk::size)), BoundarySignature>);
    static_assert(std::is_same_v<decltype(static_cast<PositionSignature>(&KisChunk::position)), PositionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<DataSignature>(&KisChunk::data)), DataSignature>);

    QVERIFY(true);
}

void KisChunkAllocatorSchemaContractTest::chunkAllocatorLifetimeAndCapacitySchemaRemainsStable()
{
    using NumChunksSignature = quint64 (KisChunkAllocator::*)() const;

    static_assert(std::is_default_constructible_v<KisChunkAllocator>);
    static_assert(std::is_constructible_v<KisChunkAllocator, quint64>);
    static_assert(std::is_constructible_v<KisChunkAllocator, quint64, quint64>);
    static_assert(std::is_destructible_v<KisChunkAllocator>);
    static_assert(
        std::is_same_v<decltype(static_cast<NumChunksSignature>(&KisChunkAllocator::numChunks)), NumChunksSignature>);

    QVERIFY(true);
}

void KisChunkAllocatorSchemaContractTest::chunkAllocatorOperationSchemaRemainsStable()
{
    using GetChunkSignature = KisChunk (KisChunkAllocator::*)(quint64);
    using FreeChunkSignature = void (KisChunkAllocator::*)(KisChunk);
    using DebugChunksSignature = void (KisChunkAllocator::*)();
    using SanityCheckSignature = bool (KisChunkAllocator::*)(bool);
    using FragmentationSignature = qreal (KisChunkAllocator::*)(bool);

    static_assert(
        std::is_same_v<decltype(static_cast<GetChunkSignature>(&KisChunkAllocator::getChunk)), GetChunkSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<FreeChunkSignature>(&KisChunkAllocator::freeChunk)), FreeChunkSignature>);
    static_assert(std::is_same_v<decltype(static_cast<DebugChunksSignature>(&KisChunkAllocator::debugChunks)),
                                 DebugChunksSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SanityCheckSignature>(&KisChunkAllocator::sanityCheck)),
                                 SanityCheckSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FragmentationSignature>(&KisChunkAllocator::debugFragmentation)),
                                 FragmentationSignature>);
    static_assert(std::is_same_v<decltype(std::declval<KisChunkAllocator &>().sanityCheck()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<KisChunkAllocator &>().debugFragmentation()), qreal>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisChunkAllocatorSchemaContractTest)

#include "KisChunkAllocatorSchemaContractTest.moc"
