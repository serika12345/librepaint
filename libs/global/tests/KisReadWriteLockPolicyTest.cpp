/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisReadWriteLockPolicy.h"

#include <QTest>

#include <type_traits>

class KisReadWriteLockPolicyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalLockersHoldRequestedMode();
    void upgradeReadLockerLeavesLockAvailable();
    void noLockLockersLeaveLockAvailable();
};

void KisReadWriteLockPolicyTest::normalLockersHoldRequestedMode()
{
    QVERIFY((std::is_same_v<NormalLockPolicy::ReadLocker, QReadLocker>));
    QVERIFY((std::is_same_v<NormalLockPolicy::WriteLocker, QWriteLocker>));

    QReadWriteLock lock;
    {
        NormalLockPolicy::ReadLocker locker(&lock);
        QVERIFY(!lock.tryLockForWrite());
    }
    {
        NormalLockPolicy::WriteLocker locker(&lock);
        QVERIFY(!lock.tryLockForRead());
    }

    QVERIFY(lock.tryLockForWrite());
    lock.unlock();
}

void KisReadWriteLockPolicyTest::upgradeReadLockerLeavesLockAvailable()
{
    QVERIFY((std::is_same_v<UpgradeLockPolicy::ReadLocker, UpgradeLockPolicy::FakeLocker>));
    QVERIFY((std::is_same_v<UpgradeLockPolicy::WriteLocker, KisUpgradeToWriteLocker>));
    QVERIFY(!std::is_copy_constructible_v<UpgradeLockPolicy::FakeLocker>);

    QReadWriteLock lock;
    UpgradeLockPolicy::ReadLocker locker(&lock);

    QVERIFY(lock.tryLockForWrite());
    lock.unlock();
}

void KisReadWriteLockPolicyTest::noLockLockersLeaveLockAvailable()
{
    QVERIFY((std::is_same_v<NoLockPolicy::ReadLocker, NoLockPolicy::FakeLocker>));
    QVERIFY((std::is_same_v<NoLockPolicy::WriteLocker, NoLockPolicy::FakeLocker>));
    QVERIFY(!std::is_copy_constructible_v<NoLockPolicy::FakeLocker>);

    QReadWriteLock lock;
    NoLockPolicy::ReadLocker readLocker(&lock);
    NoLockPolicy::WriteLocker writeLocker(&lock);

    QVERIFY(lock.tryLockForWrite());
    lock.unlock();
}

QTEST_GUILESS_MAIN(KisReadWriteLockPolicyTest)

#include "KisReadWriteLockPolicyTest.moc"
