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

#include <type_traits>

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
    void exceptionValuesPreserveDiagnostics();
    void inlineMultiStatementExecutesInOrderAndReportsFailures();
    void inlineSingleStatementBindsAndExecutes();
    void batchExecutesBoundValueLists();
    void fileConstructorsLoadScriptsAndReportOpenFailures();
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

void KisSqlQueryLoaderContractTest::exceptionValuesPreserveDiagnostics()
{
    static_assert(std::is_empty_v<KisSqlQueryLoader::single_statement_mode_t>);
    const KisSqlQueryLoader::single_statement_mode_t mode = KisSqlQueryLoader::single_statement_mode;
    Q_UNUSED(mode);

    const KisSqlQueryLoader::FileException fileException(QStringLiteral("file message"),
                                                         QStringLiteral("/queries/file.sql"),
                                                         QStringLiteral("permission denied"));
    QCOMPARE(fileException.message, QStringLiteral("file message"));
    QCOMPARE(fileException.filePath, QStringLiteral("/queries/file.sql"));
    QCOMPARE(fileException.fileErrorString, QStringLiteral("permission denied"));

    const QSqlError sqlError(QStringLiteral("driver"),
                             QStringLiteral("database"),
                             QSqlError::StatementError,
                             QStringLiteral("19"));
    const KisSqlQueryLoader::SQLException sqlException(QStringLiteral("sql message"),
                                                       QStringLiteral("inline://query"),
                                                       3,
                                                       sqlError);
    QCOMPARE(sqlException.message, QStringLiteral("sql message"));
    QCOMPARE(sqlException.filePath, QStringLiteral("inline://query"));
    QCOMPARE(sqlException.statementIndex, 3);
    QCOMPARE(sqlException.sqlError.driverText(), QStringLiteral("driver"));
    QCOMPARE(sqlException.sqlError.databaseText(), QStringLiteral("database"));
    QCOMPARE(sqlException.sqlError.type(), QSqlError::StatementError);
    QCOMPARE(sqlException.sqlError.nativeErrorCode(), QStringLiteral("19"));
}

void KisSqlQueryLoaderContractTest::inlineMultiStatementExecutesInOrderAndReportsFailures()
{
    {
        KisSqlQueryLoader loader(QStringLiteral("inline://ordered"),
                                 QStringLiteral("-- ignored comment\n"
                                                "INSERT INTO records(value) VALUES (11);\n"
                                                "INSERT INTO records(value) VALUES (12);"));
        QCOMPARE(&loader.query(), &loader.query());
        loader.exec();
    }
    QCOMPARE(recordValues(), QList<int>({11, 12}));

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
    QCOMPARE(recordValues(), QList<int>({11, 12, 13}));
}

void KisSqlQueryLoaderContractTest::inlineSingleStatementBindsAndExecutes()
{
    KisSqlQueryLoader loader(QStringLiteral("inline://bound-insert"),
                             QStringLiteral("INSERT INTO records(value) VALUES (:value)"),
                             KisSqlQueryLoader::single_statement_mode);
    QSqlQuery &query = loader.query();
    query.bindValue(QStringLiteral(":value"), 21);
    loader.exec();
    QCOMPARE(recordValues(), QList<int>({21}));

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
    KisSqlQueryLoader loader(QStringLiteral("inline://batch-insert"),
                             QStringLiteral("INSERT INTO records(value) VALUES (?)"),
                             KisSqlQueryLoader::single_statement_mode);
    loader.query().addBindValue(QVariantList({31, 32, 33}));
    loader.execBatch();

    QCOMPARE(recordValues(), QList<int>({31, 32, 33}));
}

void KisSqlQueryLoaderContractTest::fileConstructorsLoadScriptsAndReportOpenFailures()
{
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
