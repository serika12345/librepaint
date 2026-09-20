/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImageBarrierLock.h"

#include <QTest>

class KisImage
{
public:
    void barrierLock(bool readOnly)
    {
        lastReadOnly = readOnly;
        locked = true;
    }

    bool tryBarrierLock(bool readOnly)
    {
        lastReadOnly = readOnly;
        locked = tryLockResult;
        return tryLockResult;
    }

    void unlock()
    {
        locked = false;
    }

    bool lastReadOnly = false;
    bool locked = false;
    bool tryLockResult = true;
};

class KisImageBarrierLockContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void adapterForwardsWritableLockLifecycle();
    void adapterForwardsReadOnlyTryResult();
    void rawLocksUseTheAdapterContract();
};

void KisImageBarrierLockContractTest::adapterForwardsWritableLockLifecycle()
{
    KisImage image;
    KisImageBarrierLockAdapterRaw adapter(&image);

    adapter.lock();
    QVERIFY(!image.lastReadOnly);
    QVERIFY(image.locked);

    adapter.unlock();
    QVERIFY(!image.locked);
}

void KisImageBarrierLockContractTest::adapterForwardsReadOnlyTryResult()
{
    KisImage image;
    KisImageReadOnlyBarrierLockAdapterRaw adapter(&image);

    image.tryLockResult = false;
    QVERIFY(!adapter.try_lock());
    QVERIFY(image.lastReadOnly);
    QVERIFY(!image.locked);

    image.tryLockResult = true;
    QVERIFY(adapter.try_lock());
    QVERIFY(image.lastReadOnly);
    QVERIFY(image.locked);

    adapter.unlock();
}

void KisImageBarrierLockContractTest::rawLocksUseTheAdapterContract()
{
    KisImage writableImage;
    {
        KisImageBarrierLockRaw lock(&writableImage);
        QVERIFY(lock.owns_lock());
        QVERIFY(!writableImage.lastReadOnly);
    }
    QVERIFY(!writableImage.locked);

    KisImage readOnlyImage;
    {
        KisImageReadOnlyBarrierLockRaw lock(&readOnlyImage, std::try_to_lock);
        QVERIFY(lock.owns_lock());
        QVERIFY(readOnlyImage.lastReadOnly);
    }
    QVERIFY(!readOnlyImage.locked);
}

QTEST_GUILESS_MAIN(KisImageBarrierLockContractTest)

#include "KisImageBarrierLockContractTest.moc"
