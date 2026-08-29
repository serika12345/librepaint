/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImageBarrierLock.h"

#include <QTest>

#include <type_traits>

class KisImage
{
public:
    void barrierLock(bool readOnly)
    {
        ++lockCallCount;
        lastReadOnly = readOnly;
        locked = true;
    }

    bool tryBarrierLock(bool readOnly)
    {
        ++tryLockCallCount;
        lastReadOnly = readOnly;
        locked = tryLockResult;
        return tryLockResult;
    }

    void unlock()
    {
        ++unlockCallCount;
        locked = false;
    }

    int lockCallCount = 0;
    int tryLockCallCount = 0;
    int unlockCallCount = 0;
    bool lastReadOnly = false;
    bool locked = false;
    bool tryLockResult = true;
};

class KisImageBarrierLockContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aliasesSelectImageOwnershipAndAccessMode();
    void adapterForwardsWritableLockLifecycle();
    void adapterForwardsReadOnlyTryResult();
    void rawLocksUseTheAdapterContract();
};

void KisImageBarrierLockContractTest::aliasesSelectImageOwnershipAndAccessMode()
{
    static_assert(std::is_same_v<KisImageBarrierLockAdapter, KisImageBarrierLockAdapterImpl<KisImageSP, false>>);
    static_assert(std::is_same_v<KisImageReadOnlyBarrierLockAdapter, KisImageBarrierLockAdapterImpl<KisImageSP, true>>);
    static_assert(std::is_same_v<KisImageBarrierLockAdapterRaw, KisImageBarrierLockAdapterImpl<KisImage *, false>>);
    static_assert(
        std::is_same_v<KisImageReadOnlyBarrierLockAdapterRaw, KisImageBarrierLockAdapterImpl<KisImage *, true>>);
    static_assert(std::is_class_v<KisImageBarrierLockAdapterImpl<KisImage *, false>>);
    static_assert(std::is_base_of_v<KisAdaptedLock<KisImageBarrierLockAdapterRaw>, KisImageBarrierLockRaw>);
    static_assert(
        std::is_base_of_v<KisAdaptedLock<KisImageReadOnlyBarrierLockAdapterRaw>, KisImageReadOnlyBarrierLockRaw>);

    QVERIFY(true);
}

void KisImageBarrierLockContractTest::adapterForwardsWritableLockLifecycle()
{
    KisImage image;
    KisImageBarrierLockAdapterRaw adapter(&image);

    adapter.lock();
    QCOMPARE(image.lockCallCount, 1);
    QVERIFY(!image.lastReadOnly);
    QVERIFY(image.locked);

    adapter.unlock();
    QCOMPARE(image.unlockCallCount, 1);
    QVERIFY(!image.locked);
}

void KisImageBarrierLockContractTest::adapterForwardsReadOnlyTryResult()
{
    KisImage image;
    KisImageReadOnlyBarrierLockAdapterRaw adapter(&image);

    image.tryLockResult = false;
    QVERIFY(!adapter.try_lock());
    QCOMPARE(image.tryLockCallCount, 1);
    QVERIFY(image.lastReadOnly);
    QVERIFY(!image.locked);

    image.tryLockResult = true;
    QVERIFY(adapter.try_lock());
    QCOMPARE(image.tryLockCallCount, 2);
    QVERIFY(image.lastReadOnly);
    QVERIFY(image.locked);

    adapter.unlock();
    QCOMPARE(image.unlockCallCount, 1);
}

void KisImageBarrierLockContractTest::rawLocksUseTheAdapterContract()
{
    KisImage writableImage;
    {
        KisImageBarrierLockRaw lock(&writableImage);
        QVERIFY(lock.owns_lock());
        QCOMPARE(writableImage.lockCallCount, 1);
        QVERIFY(!writableImage.lastReadOnly);
    }
    QCOMPARE(writableImage.unlockCallCount, 1);

    KisImage readOnlyImage;
    {
        KisImageReadOnlyBarrierLockRaw lock(&readOnlyImage, std::try_to_lock);
        QVERIFY(lock.owns_lock());
        QCOMPARE(readOnlyImage.tryLockCallCount, 1);
        QVERIFY(readOnlyImage.lastReadOnly);
    }
    QCOMPARE(readOnlyImage.unlockCallCount, 1);
}

QTEST_GUILESS_MAIN(KisImageBarrierLockContractTest)

#include "KisImageBarrierLockContractTest.moc"
