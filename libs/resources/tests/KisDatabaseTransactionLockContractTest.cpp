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
    void adapterStartsRollsBackAndCommitsTransactions();
    void lockRollsBackOnScopeExit();
    void commitPersistsChanges();
    void rollbackCancelsAndRelinquishesOwnership();
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

void KisDatabaseTransactionLockContractTest::adapterStartsRollsBackAndCommitsTransactions()
{
    detail::KisDatabaseTransactionLockAdapter adapter(QSqlDatabase::database());

    adapter.lock();
    QVERIFY(insertValue(17));
    adapter.unlock();
    QCOMPARE(recordCount(), 0);
    adapter.unlock();

    adapter.lock();
    QVERIFY(insertValue(23));
    adapter.commit();
    QCOMPARE(recordCount(), 1);
    adapter.commit();
}

void KisDatabaseTransactionLockContractTest::lockRollsBackOnScopeExit()
{
    {
        KisDatabaseTransactionLock lock(QSqlDatabase::database());
        QVERIFY(lock.owns_lock());
        QVERIFY(insertValue(31));
    }

    QCOMPARE(recordCount(), 0);
}

void KisDatabaseTransactionLockContractTest::commitPersistsChanges()
{
    {
        KisDatabaseTransactionLock lock(QSqlDatabase::database());
        QVERIFY(insertValue(37));
        lock.commit();
        QVERIFY(lock.owns_lock());
    }

    QCOMPARE(recordCount(), 1);
}

void KisDatabaseTransactionLockContractTest::rollbackCancelsAndRelinquishesOwnership()
{
    KisDatabaseTransactionLock lock(QSqlDatabase::database());
    QVERIFY(insertValue(41));

    lock.rollback();
    QVERIFY(!lock.owns_lock());
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
