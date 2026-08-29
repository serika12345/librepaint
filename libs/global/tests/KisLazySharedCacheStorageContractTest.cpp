/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLazySharedCacheStorage.h"

#include <QSemaphore>
#include <QTest>

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace
{
struct CacheValue {
    explicit CacheValue(int value)
        : value(value)
    {
    }

    int value;
};

using LocalDataStorage = KisLazySharedCacheStorageDetail::DataStorage<QSharedPointer<CacheValue>, CacheValue, int>;
using LocalDataWrapper = KisLazySharedCacheStorageDetail::DataWrapperLocal<CacheValue, int>;
using SharedDataWrapper = KisLazySharedCacheStorageDetail::DataWrapperShared<CacheValue, int>;
using LocalCacheBase = KisLazySharedCacheStorageBase<LocalDataWrapper, CacheValue, int>;
using LocalCache = KisLazySharedCacheStorage<CacheValue, int>;
using LinkedCache = KisLazySharedCacheStorageLinked<CacheValue, int>;

template<typename CacheA, typename CacheB>
void verifyConcurrentConstruction(CacheA &firstCache, CacheB &secondCache, std::atomic<int> &factoryCalls)
{
    constexpr int threadCount = 8;
    QSemaphore start;
    std::vector<const CacheValue *> values(threadCount, nullptr);
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&, i]() {
            start.acquire();
            values[i] = i % 2 == 0 ? firstCache.value(73) : secondCache.value(73);
        });
    }

    start.release(threadCount);
    for (std::thread &thread : threads) {
        thread.join();
    }

    QCOMPARE(factoryCalls.load(), 1);
    QVERIFY(values.front());
    QCOMPARE(values.front()->value, 73);
    for (const CacheValue *value : values) {
        QCOMPARE(value, values.front());
    }
}
} // namespace

class KisLazySharedCacheStorageContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAliasesDescribeStoragePolicies();
    void localDataStorageCopiesValuesAndDetachesReset();
    void sharedDataWrapperSharesInitializationUntilReset();
    void cacheLifecycleForwardsArgumentsAndBuildsOnce();
    void localAndLinkedCachesPreserveCopyContracts();
    void concurrentAccessConstructsOnce();
};

void KisLazySharedCacheStorageContractTest::typeAliasesDescribeStoragePolicies()
{
    using ExpectedFactory = CacheValue *(int);
    using ExpectedLocalCache = KisLazySharedCacheStorageBase<LocalDataWrapper, CacheValue, int>;
    using ExpectedLinkedCache = KisLazySharedCacheStorageBase<SharedDataWrapper, CacheValue, int>;

    QVERIFY((std::is_same_v<LocalDataStorage::ConstType, const CacheValue>));
    QVERIFY((std::is_same_v<LocalDataStorage::FactoryType, ExpectedFactory>));
    QVERIFY((std::is_same_v<LocalDataWrapper, LocalDataStorage>));
    QVERIFY((std::is_same_v<SharedDataWrapper::ConstType, const CacheValue>));
    QVERIFY((std::is_same_v<SharedDataWrapper::FactoryType, ExpectedFactory>));
    QVERIFY((std::is_same_v<LocalCacheBase::ConstType, const CacheValue>));
    QVERIFY((std::is_same_v<LocalCacheBase::FactoryType, ExpectedFactory>));
    QVERIFY((std::is_same_v<LocalCache, ExpectedLocalCache>));
    QVERIFY((std::is_same_v<LinkedCache, ExpectedLinkedCache>));
}

void KisLazySharedCacheStorageContractTest::localDataStorageCopiesValuesAndDetachesReset()
{
    LocalDataStorage empty;
    QVERIFY(!empty.hasValue());

    int factoryCalls = 0;
    const CacheValue *initialized = empty.lazyInitialize(
        [&](int value) {
            ++factoryCalls;
            return new CacheValue(value);
        },
        11);
    QCOMPARE(factoryCalls, 1);
    QCOMPARE(initialized->value, 11);
    QCOMPARE(empty.lazyInitialize(
                 [](int value) {
                     return new CacheValue(value);
                 },
                 99),
             initialized);

    LocalDataStorage copied(empty);
    LocalDataStorage assigned;
    assigned = empty;
    QCOMPARE(copied.lazyInitialize(
                 [](int value) {
                     return new CacheValue(value);
                 },
                 12),
             initialized);
    QCOMPARE(assigned.lazyInitialize(
                 [](int value) {
                     return new CacheValue(value);
                 },
                 13),
             initialized);

    empty.reset();
    QVERIFY(!empty.hasValue());
    QVERIFY(copied.hasValue());
    QVERIFY(assigned.hasValue());

    CacheValue *directValue = new CacheValue(21);
    LocalDataStorage direct(directValue);
    int directFactoryCalls = 0;
    QCOMPARE(direct.lazyInitialize(
                 [&](int value) {
                     ++directFactoryCalls;
                     return new CacheValue(value);
                 },
                 22),
             directValue);
    QCOMPARE(directFactoryCalls, 0);
}

void KisLazySharedCacheStorageContractTest::sharedDataWrapperSharesInitializationUntilReset()
{
    SharedDataWrapper original;
    SharedDataWrapper copied(original);
    SharedDataWrapper assigned;
    assigned = original;
    QVERIFY(!original.hasValue());
    QVERIFY(!copied.hasValue());
    QVERIFY(!assigned.hasValue());

    int factoryCalls = 0;
    const CacheValue *initialized = copied.lazyInitialize(
        [&](int value) {
            ++factoryCalls;
            return new CacheValue(value);
        },
        31);
    QCOMPARE(factoryCalls, 1);
    QVERIFY(original.hasValue());
    QVERIFY(assigned.hasValue());
    QCOMPARE(original.lazyInitialize(
                 [](int value) {
                     return new CacheValue(value);
                 },
                 32),
             initialized);
    QCOMPARE(assigned.lazyInitialize(
                 [](int value) {
                     return new CacheValue(value);
                 },
                 33),
             initialized);

    original.reset();
    QVERIFY(!original.hasValue());
    QVERIFY(copied.hasValue());
    QVERIFY(assigned.hasValue());

    const CacheValue *replacement = original.lazyInitialize(
        [&](int value) {
            ++factoryCalls;
            return new CacheValue(value);
        },
        34);
    QCOMPARE(factoryCalls, 2);
    QCOMPARE(replacement->value, 34);
    QVERIFY(replacement != initialized);

    CacheValue *directValue = new CacheValue(35);
    SharedDataWrapper direct(directValue);
    SharedDataWrapper directCopy(direct);
    QCOMPARE(directCopy.lazyInitialize(
                 [](int value) {
                     return new CacheValue(value);
                 },
                 36),
             directValue);
    direct.reset();
    QVERIFY(!direct.hasValue());
    QVERIFY(directCopy.hasValue());
}

void KisLazySharedCacheStorageContractTest::cacheLifecycleForwardsArgumentsAndBuildsOnce()
{
    LocalCache cache;
    QVERIFY(cache.isNull());

    int factoryCalls = 0;
    cache.setFactory([&](int value) {
        ++factoryCalls;
        return new CacheValue(value);
    });
    cache.initialize(41);
    QCOMPARE(factoryCalls, 1);
    QVERIFY(!cache.isNull());
    const CacheValue *initialized = cache.value(42);
    QCOMPARE(initialized->value, 41);
    QCOMPARE(factoryCalls, 1);

    cache.reset();
    QVERIFY(cache.isNull());
    const CacheValue *replacement = cache.value(43);
    QCOMPARE(factoryCalls, 2);
    QCOMPARE(replacement->value, 43);

    int constructedFactoryCalls = 0;
    LocalCache constructed([&](int value) {
        ++constructedFactoryCalls;
        return new CacheValue(value);
    });
    QVERIFY(constructed.isNull());
    QCOMPARE(constructed.value(44)->value, 44);
    QCOMPARE(constructedFactoryCalls, 1);
}

void KisLazySharedCacheStorageContractTest::localAndLinkedCachesPreserveCopyContracts()
{
    int localFactoryCalls = 0;
    LocalCache localSource([&](int value) {
        ++localFactoryCalls;
        return new CacheValue(value);
    });
    LocalCache localCopyBeforeInitialization(localSource);

    const CacheValue *localSourceValue = localSource.value(51);
    const CacheValue *localCopyBeforeValue = localCopyBeforeInitialization.value(52);
    QCOMPARE(localFactoryCalls, 2);
    QVERIFY(localSourceValue != localCopyBeforeValue);
    QCOMPARE(localSourceValue->value, 51);
    QCOMPARE(localCopyBeforeValue->value, 52);

    LocalCache localCopyAfterInitialization(localSource);
    QCOMPARE(localCopyAfterInitialization.value(53), localSourceValue);
    QCOMPARE(localFactoryCalls, 2);
    localSource.reset();
    QVERIFY(localSource.isNull());
    QVERIFY(!localCopyAfterInitialization.isNull());
    QCOMPARE(localCopyAfterInitialization.value(54), localSourceValue);
    QCOMPARE(localSource.value(55)->value, 55);
    QCOMPARE(localFactoryCalls, 3);

    int linkedFactoryCalls = 0;
    LinkedCache linkedSource([&](int value) {
        ++linkedFactoryCalls;
        return new CacheValue(value);
    });
    LinkedCache linkedCopyBeforeInitialization(linkedSource);

    const CacheValue *linkedValue = linkedSource.value(61);
    QVERIFY(!linkedCopyBeforeInitialization.isNull());
    QCOMPARE(linkedCopyBeforeInitialization.value(62), linkedValue);
    QCOMPARE(linkedFactoryCalls, 1);
    QCOMPARE(linkedValue->value, 61);

    LinkedCache linkedCopyAfterInitialization(linkedSource);
    QCOMPARE(linkedCopyAfterInitialization.value(63), linkedValue);
    linkedSource.reset();
    QVERIFY(linkedSource.isNull());
    QVERIFY(!linkedCopyBeforeInitialization.isNull());
    QVERIFY(!linkedCopyAfterInitialization.isNull());
    QCOMPARE(linkedCopyBeforeInitialization.value(64), linkedValue);
    QCOMPARE(linkedCopyAfterInitialization.value(65), linkedValue);
    QCOMPARE(linkedSource.value(66)->value, 66);
    QCOMPARE(linkedFactoryCalls, 2);
}

void KisLazySharedCacheStorageContractTest::concurrentAccessConstructsOnce()
{
    std::atomic<int> localFactoryCalls{0};
    LocalCache local([&](int value) {
        ++localFactoryCalls;
        return new CacheValue(value);
    });
    verifyConcurrentConstruction(local, local, localFactoryCalls);

    std::atomic<int> linkedFactoryCalls{0};
    LinkedCache linked([&](int value) {
        ++linkedFactoryCalls;
        return new CacheValue(value);
    });
    LinkedCache linkedCopy(linked);
    verifyConcurrentConstruction(linked, linkedCopy, linkedFactoryCalls);
}

QTEST_GUILESS_MAIN(KisLazySharedCacheStorageContractTest)

#include "KisLazySharedCacheStorageContractTest.moc"
