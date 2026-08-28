/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisTemporaryResourceStorageLock.h>

#include <QSet>
#include <QTest>

namespace
{
QSet<QString> storageLocations;
}

namespace KisTemporaryResourceStorageLocator
{
bool hasStorage(const QString &location)
{
    return storageLocations.contains(location);
}

void addMemoryStorage(const QString &location)
{
    storageLocations.insert(location);
}

void removeStorage(const QString &location)
{
    storageLocations.remove(location);
}
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class KisTemporaryResourceStorageLockContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void adapterRegistersAndRemovesStorage();
    void adapterAvoidsExistingStorageNames();
    void tryLockRegistersAUniqueStorage();
    void wrapperOwnsStorageForItsScope();
};

void KisTemporaryResourceStorageLockContractTest::init()
{
    storageLocations.clear();
}

void KisTemporaryResourceStorageLockContractTest::adapterRegistersAndRemovesStorage()
{
    KisTemporaryResourceStorageLockAdapter adapter(QStringLiteral("temporary"));
    QVERIFY(adapter.storageLocation().isEmpty());

    adapter.lock();
    QCOMPARE(adapter.storageLocation(), QStringLiteral("temporary"));
    QVERIFY(storageLocations.contains(QStringLiteral("temporary")));

    adapter.unlock();
    QVERIFY(adapter.storageLocation().isEmpty());
    QVERIFY(!storageLocations.contains(QStringLiteral("temporary")));
}

void KisTemporaryResourceStorageLockContractTest::adapterAvoidsExistingStorageNames()
{
    storageLocations.insert(QStringLiteral("temporary"));
    storageLocations.insert(QStringLiteral("temporary_0"));
    KisTemporaryResourceStorageLockAdapter adapter(QStringLiteral("temporary"));

    adapter.lock();
    QCOMPARE(adapter.storageLocation(), QStringLiteral("temporary_1"));
    QVERIFY(storageLocations.contains(QStringLiteral("temporary_1")));
    adapter.unlock();
}

void KisTemporaryResourceStorageLockContractTest::tryLockRegistersAUniqueStorage()
{
    storageLocations.insert(QStringLiteral("temporary"));
    KisTemporaryResourceStorageLockAdapter adapter(QStringLiteral("temporary"));

    QVERIFY(adapter.try_lock());
    QCOMPARE(adapter.storageLocation(), QStringLiteral("temporary_0"));
    adapter.unlock();
}

void KisTemporaryResourceStorageLockContractTest::wrapperOwnsStorageForItsScope()
{
    QString location;
    {
        KisTemporaryResourceStorageLock lock(QStringLiteral("scope"));
        QVERIFY(lock.owns_lock());
        location = lock.storageLocation();
        QCOMPARE(location, QStringLiteral("scope"));
        QVERIFY(storageLocations.contains(location));
    }

    QVERIFY(!storageLocations.contains(location));
}

QTEST_GUILESS_MAIN(KisTemporaryResourceStorageLockContractTest)

#include "KisTemporaryResourceStorageLockContractTest.moc"
