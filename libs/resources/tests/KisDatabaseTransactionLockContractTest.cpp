/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDatabaseTransactionLock.h>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

namespace
{
bool insertValue(int value)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT INTO records (value) VALUES (:value)"));
    query.bindValue(QStringLiteral(":value"), value);
    return query.exec();
}

int recordCount()
{
    QSqlQuery query;
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM records")) || !query.first()) {
        return -1;
    }
    return query.value(0).toInt();
}
}

class KisDatabaseTransactionLockContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void scopeExitCancelsUncommittedCacheChanges();
    void commitPersistsAllCacheChanges();
    void explicitRollbackCancelsCacheChanges();
    void cleanupTestCase();
};

void KisDatabaseTransactionLockContractTest::initTestCase()
{
    QVERIFY(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    database.setDatabaseName(QStringLiteral(":memory:"));
    QVERIFY(database.open());

    QSqlQuery query;
    QVERIFY(query.exec(QStringLiteral("CREATE TABLE records (value INTEGER NOT NULL)")));
}

void KisDatabaseTransactionLockContractTest::init()
{
    QSqlQuery query;
    QVERIFY(query.exec(QStringLiteral("DELETE FROM records")));
}

void KisDatabaseTransactionLockContractTest::scopeExitCancelsUncommittedCacheChanges()
{
    // Consumer: Resource-cache initialization, synchronization, and cleanup operations.
    // Operation: An operation writes several cache records but ends before it is accepted.
    // Observable result: Scope exit removes every uncommitted record.
    // Failure impact: A failed resource update can leave a partial cache that shows missing or stale resources.
    {
        KisDatabaseTransactionLock lock(QSqlDatabase::database());
        QVERIFY(insertValue(31));
        QVERIFY(insertValue(32));
    }

    QCOMPARE(recordCount(), 0);
}

void KisDatabaseTransactionLockContractTest::commitPersistsAllCacheChanges()
{
    // Consumer: Resource-cache initialization, synchronization, and cleanup operations.
    // Operation: An operation writes several cache records and commits after every step succeeds.
    // Observable result: All records remain available after the transaction object leaves scope.
    // Failure impact: A successful resource update can disappear, leaving the resource chooser out of date.
    {
        KisDatabaseTransactionLock lock(QSqlDatabase::database());
        QVERIFY(insertValue(37));
        QVERIFY(insertValue(38));
        lock.commit();
    }

    QCOMPARE(recordCount(), 2);
}

void KisDatabaseTransactionLockContractTest::explicitRollbackCancelsCacheChanges()
{
    // Consumer: Resource-cache initialization, synchronization, and cleanup operations that detect an error.
    // Operation: An operation writes cache records and explicitly cancels the transaction.
    // Observable result: The written records are unavailable immediately after cancellation.
    // Failure impact: A rejected update can be presented as a valid resource-cache state.
    KisDatabaseTransactionLock lock(QSqlDatabase::database());
    QVERIFY(insertValue(41));
    QVERIFY(insertValue(42));

    lock.rollback();
    QCOMPARE(recordCount(), 0);
}

void KisDatabaseTransactionLockContractTest::cleanupTestCase()
{
    {
        QSqlDatabase database = QSqlDatabase::database();
        database.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

QTEST_GUILESS_MAIN(KisDatabaseTransactionLockContractTest)

#include "KisDatabaseTransactionLockContractTest.moc"
