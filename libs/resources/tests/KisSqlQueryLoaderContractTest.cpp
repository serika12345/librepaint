/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisSqlQueryLoader.h>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

namespace
{
QList<int> recordValues()
{
    QList<int> values;
    QSqlQuery query;
    if (!query.exec(QStringLiteral("SELECT value FROM records ORDER BY sequence"))) {
        return values;
    }

    while (query.next()) {
        values.append(query.value(0).toInt());
    }
    return values;
}

QString writeScript(QTemporaryFile *file, const QByteArray &script)
{
    if (!file->open() || file->write(script) != script.size()) {
        return {};
    }
    file->close();
    return file->fileName();
}
} // namespace

class KisSqlQueryLoaderContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void multiStatementScriptsApplyEveryStatement();
    void failingScriptsReportTheirLocation();
    void inlineSingleStatementBindsAndExecutes();
    void failingBoundStatementReportsItsLocation();
    void batchExecutesBoundValueLists();
    void fileScriptsExecuteWithBoundValues();
    void missingScriptReportsItsPath();
    void cleanupTestCase();
};

void KisSqlQueryLoaderContractTest::initTestCase()
{
    QVERIFY(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    database.setDatabaseName(QStringLiteral(":memory:"));
    QVERIFY(database.open());

    QSqlQuery query;
    QVERIFY(
        query.exec(QStringLiteral("CREATE TABLE records ("
                                  "sequence INTEGER PRIMARY KEY AUTOINCREMENT, "
                                  "value INTEGER NOT NULL UNIQUE)")));
}

void KisSqlQueryLoaderContractTest::init()
{
    QSqlQuery query;
    QVERIFY(query.exec(QStringLiteral("DELETE FROM records")));
}

void KisSqlQueryLoaderContractTest::multiStatementScriptsApplyEveryStatement()
{
    // Consumer: Resource-cache initialization and database migration steps with SQL resource scripts.
    // Operation: A script contains comments and several statements that add cache records.
    // Observable result: Every executable statement runs in source order.
    // Failure impact: A resource-cache migration can omit part of its schema or data update.
    KisSqlQueryLoader loader(QStringLiteral("inline://ordered"),
                             QStringLiteral("-- ignored comment\n"
                                            "INSERT INTO records(value) VALUES (11);\n"
                                            "INSERT INTO records(value) VALUES (12);"));
    loader.exec();
    QCOMPARE(recordValues(), QList<int>({11, 12}));
}

void KisSqlQueryLoaderContractTest::failingScriptsReportTheirLocation()
{
    // Consumer: Resource-cache migration and synchronization error reporting.
    // Operation: A multi-statement script succeeds once and then executes an invalid statement.
    // Observable result: The reported diagnostic identifies the script and failing statement while earlier changes remain inspectable.
    // Failure impact: A failed cache update cannot be diagnosed or recovered because its responsible SQL resource is unknown.
    try {
        KisSqlQueryLoader loader(QStringLiteral("inline://failure"),
                                 QStringLiteral("INSERT INTO records(value) VALUES (13);"
                                                "INSERT INTO missing_records(value) VALUES (14);"));
        loader.exec();
        QFAIL("multi-statement execution should report the failing statement");
    } catch (const KisSqlQueryLoader::SQLException &exception) {
        QCOMPARE(exception.message, QStringLiteral("Failed to execute sql from file"));
        QCOMPARE(exception.filePath, QStringLiteral("inline://failure"));
        QCOMPARE(exception.statementIndex, 1);
        QVERIFY(exception.sqlError.isValid());
    }
    QCOMPARE(recordValues(), QList<int>({13}));
}

void KisSqlQueryLoaderContractTest::inlineSingleStatementBindsAndExecutes()
{
    // Consumer: Resource-cache queries that bind resource IDs, locations, and metadata keys.
    // Operation: A single-statement loader receives a named value before execution.
    // Observable result: The bound value is stored by the prepared statement.
    // Failure impact: Resource synchronization can update the wrong row or fail to save its selected resource.
    KisSqlQueryLoader loader(QStringLiteral("inline://bound-insert"),
                             QStringLiteral("INSERT INTO records(value) VALUES (:value)"),
                             KisSqlQueryLoader::single_statement_mode);
    QSqlQuery &query = loader.query();
    query.bindValue(QStringLiteral(":value"), 21);
    loader.exec();
    QCOMPARE(recordValues(), QList<int>({21}));
}

void KisSqlQueryLoaderContractTest::failingBoundStatementReportsItsLocation()
{
    // Consumer: Resource-cache query error reporting for statements with bound values.
    // Operation: A bound statement violates a database constraint.
    // Observable result: The failure reports the source name, first statement, and database error.
    // Failure impact: A resource update can fail without identifying the SQL operation that needs repair.
    KisSqlQueryLoader failingLoader(QStringLiteral("inline://null-insert"),
                                    QStringLiteral("INSERT INTO records(value) VALUES (:value)"),
                                    KisSqlQueryLoader::single_statement_mode);
    failingLoader.query().bindValue(QStringLiteral(":value"), QVariant());
    try {
        failingLoader.exec();
        QFAIL("single-statement execution should report constraint failures");
    } catch (const KisSqlQueryLoader::SQLException &exception) {
        QCOMPARE(exception.filePath, QStringLiteral("inline://null-insert"));
        QCOMPARE(exception.statementIndex, 0);
        QVERIFY(exception.sqlError.isValid());
    }
}

void KisSqlQueryLoaderContractTest::batchExecutesBoundValueLists()
{
    // Consumer: Resource-cache synchronization that stores several selected values at once.
    // Operation: A prepared statement receives a list of bound values for batch execution.
    // Observable result: Each supplied value becomes a stored record.
    // Failure impact: Resource metadata or associations can be missing after a bulk update.
    KisSqlQueryLoader loader(QStringLiteral("inline://batch-insert"),
                             QStringLiteral("INSERT INTO records(value) VALUES (?)"),
                             KisSqlQueryLoader::single_statement_mode);
    loader.query().addBindValue(QVariantList({31, 32, 33}));
    loader.execBatch();

    QCOMPARE(recordValues(), QList<int>({31, 32, 33}));
}

void KisSqlQueryLoaderContractTest::fileScriptsExecuteWithBoundValues()
{
    // Consumer: Resource-cache initialization and migration code that loads bundled SQL files.
    // Operation: It executes a multi-statement file and binds a value into a single-statement file.
    // Observable result: Both file-backed scripts apply their records.
    // Failure impact: A bundled resource database cannot be created or upgraded correctly.
    QTemporaryFile multiStatementFile;
    const QString multiStatementPath = writeScript(&multiStatementFile,
                                                   QByteArrayLiteral("INSERT INTO records(value) VALUES (41);"
                                                                     "INSERT INTO records(value) VALUES (42);"));
    QVERIFY(!multiStatementPath.isEmpty());
    {
        KisSqlQueryLoader loader(multiStatementPath);
        loader.exec();
    }

    QTemporaryFile singleStatementFile;
    const QString singleStatementPath =
        writeScript(&singleStatementFile, QByteArrayLiteral("INSERT INTO records(value) VALUES (:value)"));
    QVERIFY(!singleStatementPath.isEmpty());
    {
        KisSqlQueryLoader loader(singleStatementPath, KisSqlQueryLoader::single_statement_mode);
        loader.query().bindValue(QStringLiteral(":value"), 43);
        loader.exec();
    }
    QCOMPARE(recordValues(), QList<int>({41, 42, 43}));
}

void KisSqlQueryLoaderContractTest::missingScriptReportsItsPath()
{
    // Consumer: Resource-cache initialization and migration error reporting.
    // Operation: It opens a SQL resource file that is unavailable.
    // Observable result: The file error identifies the unavailable path and its operating-system cause.
    // Failure impact: A broken installation cannot be diagnosed when the resource database fails to initialize.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString missingPath = directory.filePath(QStringLiteral("missing.sql"));
    try {
        KisSqlQueryLoader loader(missingPath);
        Q_UNUSED(loader);
        QFAIL("opening a missing script should report a file exception");
    } catch (const KisSqlQueryLoader::FileException &exception) {
        QCOMPARE(exception.message, QStringLiteral("Could not load SQL script file"));
        QCOMPARE(exception.filePath, missingPath);
        QVERIFY(!exception.fileErrorString.isEmpty());
    }
}

void KisSqlQueryLoaderContractTest::cleanupTestCase()
{
    {
        QSqlDatabase database = QSqlDatabase::database();
        database.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

QTEST_GUILESS_MAIN(KisSqlQueryLoaderContractTest)

#include "KisSqlQueryLoaderContractTest.moc"
