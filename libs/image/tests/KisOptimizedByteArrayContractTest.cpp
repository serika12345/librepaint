/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisOptimizedByteArray.h>

#include <QByteArray>
#include <QTest>
#include <QVector>

#include <utility>

namespace
{
class TrackingAllocator final : public KisOptimizedByteArray::MemoryAllocator
{
public:
    using MemoryChunk = KisOptimizedByteArray::MemoryChunk;

    explicit TrackingAllocator(int spareCapacity = 0, bool *destroyed = nullptr)
        : spareCapacity(spareCapacity)
        , destroyed(destroyed)
    {
    }

    ~TrackingAllocator() override
    {
        for (quint8 *pointer : allocations) {
            delete[] pointer;
        }

        if (destroyed) {
            *destroyed = true;
        }
    }

    MemoryChunk alloc(int size) override
    {
        const int capacity = size + spareCapacity;
        quint8 *pointer = new quint8[capacity];
        allocations.append(pointer);
        return MemoryChunk(pointer, capacity);
    }

    void free(MemoryChunk chunk) override
    {
        if (chunk.first) {
            QVERIFY(allocations.removeOne(chunk.first));
            delete[] chunk.first;
        }
    }

    int spareCapacity = 0;
    bool *destroyed = nullptr;
    QVector<quint8 *> allocations;
};

class TrackingPooledAllocator final : public KisOptimizedByteArray::PooledMemoryAllocator
{
public:
    explicit TrackingPooledAllocator(bool *destroyed)
        : destroyed(destroyed)
    {
    }

    ~TrackingPooledAllocator() override
    {
        *destroyed = true;
    }

private:
    bool *destroyed;
};
} // namespace

class KisOptimizedByteArrayContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionPreservesAllocatorAndEmptyState();
    void explicitFillAndResizeTrackLogicalStorage();
    void copiesAndAssignmentsPreserveIndependentValues();
    void pooledAllocatorProvidesRequestedStorage();
};

void KisOptimizedByteArrayContractTest::constructionPreservesAllocatorAndEmptyState()
{
    KisOptimizedByteArray defaultArray;
    QCOMPARE(defaultArray.size(), 0);
    QVERIFY(defaultArray.isEmpty());
    QCOMPARE(defaultArray.data(), nullptr);

    const KisOptimizedByteArray &constDefaultArray = defaultArray;
    QCOMPARE(constDefaultArray.constData(), nullptr);
    QVERIFY(defaultArray.customMemoryAllocator());

    bool destroyed = false;
    KisOptimizedByteArray::MemoryAllocatorSP customAllocator(new TrackingAllocator(0, &destroyed));
    QWeakPointer<KisOptimizedByteArray::MemoryAllocator> weakAllocator(customAllocator);

    {
        KisOptimizedByteArray customArray(customAllocator);
        QCOMPARE(customArray.customMemoryAllocator().data(), customAllocator.data());
        customAllocator.clear();
        QVERIFY(!weakAllocator.isNull());
    }

    QVERIFY(weakAllocator.isNull());
    QVERIFY(destroyed);
}

void KisOptimizedByteArrayContractTest::explicitFillAndResizeTrackLogicalStorage()
{
    QSharedPointer<TrackingAllocator> allocator(new TrackingAllocator(4));

    {
        KisOptimizedByteArray array(allocator);
        array.fill(0xa5, 4);

        QCOMPARE(array.size(), 4);
        QVERIFY(!array.isEmpty());

        quint8 *initialData = array.data();
        QVERIFY(initialData);
        const KisOptimizedByteArray &constArray = array;
        QCOMPARE(constArray.constData(), initialData);
        for (int i = 0; i < array.size(); ++i) {
            QCOMPARE(initialData[i], quint8(0xa5));
        }

        array.resize(2);
        QCOMPARE(array.size(), 2);

        array.resize(8);
        QCOMPARE(array.size(), 8);

        array.resize(9);
        QCOMPARE(array.size(), 9);

        array.fill(0x3c, 6);
        QCOMPARE(array.size(), 6);
        for (int i = 0; i < array.size(); ++i) {
            QCOMPARE(array.constData()[i], quint8(0x3c));
        }

        array.resize(0);
        QCOMPARE(array.size(), 0);
        QVERIFY(array.isEmpty());
    }
    QVERIFY(allocator->allocations.isEmpty());
}

void KisOptimizedByteArrayContractTest::copiesAndAssignmentsPreserveIndependentValues()
{
    QSharedPointer<TrackingAllocator> allocator(new TrackingAllocator());

    {
        KisOptimizedByteArray original(allocator);
        original.fill(0x21, 4);

        KisOptimizedByteArray copied(original);
        KisOptimizedByteArray assigned;
        KisOptimizedByteArray &assignmentResult = (assigned = original);

        QCOMPARE(&assignmentResult, &assigned);
        QCOMPARE(copied.customMemoryAllocator().data(), allocator.data());
        QCOMPARE(assigned.customMemoryAllocator().data(), allocator.data());

        quint8 *copiedData = copied.data();
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(copiedData), copied.size()), QByteArray(4, char(0x21)));
        copiedData[0] = 0x42;
        QCOMPARE(original.constData()[0], quint8(0x21));

        quint8 *assignedData = assigned.data();
        assignedData[1] = 0x63;
        QCOMPARE(original.constData()[1], quint8(0x21));
    }
    QVERIFY(allocator->allocations.isEmpty());
}

void KisOptimizedByteArrayContractTest::pooledAllocatorProvidesRequestedStorage()
{
    bool destroyed = false;
    KisOptimizedByteArray::MemoryAllocator *allocator = new TrackingPooledAllocator(&destroyed);

    const KisOptimizedByteArray::MemoryChunk first = allocator->alloc(16);
    QVERIFY(first.first);
    QVERIFY(first.second >= 16);
    allocator->free(first);

    const KisOptimizedByteArray::MemoryChunk reused = allocator->alloc(8);
    QVERIFY(reused.first);
    QVERIFY(reused.second >= 8);
    allocator->free(reused);
    allocator->free(KisOptimizedByteArray::MemoryChunk(nullptr, 0));

    delete allocator;
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KisOptimizedByteArrayContractTest)

#include "KisOptimizedByteArrayContractTest.moc"
