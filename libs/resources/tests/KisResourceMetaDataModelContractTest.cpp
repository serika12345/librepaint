/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceMetaDataModel.h>

#include <QDataStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTest>

namespace
{
QByteArray encodedVariant(const QVariant &value)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << value;
    return data.toBase64();
}

bool createMetadataTable()
{
    QSqlQuery query;
    return query.exec(QStringLiteral(
        "CREATE TABLE metadata ("
        "foreign_id INTEGER NOT NULL, "
        "table_name TEXT NOT NULL, "
        "key TEXT NOT NULL, "
        "value BLOB)"));
}

bool insertMetadata(int resourceId,
                    const QString &tableName,
                    const QString &key,
                    const QByteArray &value)
{
    QSqlQuery query;
    if (!query.prepare(QStringLiteral(
            "INSERT INTO metadata (foreign_id, table_name, key, value) "
            "VALUES (:id, :table, :key, :value)"))) {
        return false;
    }

    query.bindValue(QStringLiteral(":id"), resourceId);
    query.bindValue(QStringLiteral(":table"), tableName);
    query.bindValue(QStringLiteral(":key"), key);
    query.bindValue(QStringLiteral(":value"), value);
    return query.exec();
}
}

class KisResourceMetaDataModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void filtersSerializedValuesByTableResourceAndKey();
    void returnsInvalidValueForMissingOrEmptyData();
    void releasesPreparedQueryBeforeSchemaReplacement();
    void cleanupTestCase();
};

void KisResourceMetaDataModelContractTest::initTestCase()
{
    QVERIFY(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    database.setDatabaseName(QStringLiteral(":memory:"));
    QVERIFY(database.open());
    QVERIFY(createMetadataTable());

    QVERIFY(insertMetadata(17,
                           QStringLiteral("resources"),
                           QStringLiteral("answer"),
                           encodedVariant(42)));
    QVERIFY(insertMetadata(17,
                           QStringLiteral("resources"),
                           QStringLiteral("label"),
                           encodedVariant(QStringLiteral("Brush"))));
    QVERIFY(insertMetadata(23,
                           QStringLiteral("resources"),
                           QStringLiteral("answer"),
                           encodedVariant(84)));
    QVERIFY(insertMetadata(17,
                           QStringLiteral("storages"),
                           QStringLiteral("answer"),
                           encodedVariant(126)));
    QVERIFY(insertMetadata(17,
                           QStringLiteral("resources"),
                           QStringLiteral("empty"),
                           QByteArray()));
}

void KisResourceMetaDataModelContractTest::filtersSerializedValuesByTableResourceAndKey()
{
    KisResourceMetaDataModel resources(QStringLiteral("resources"));
    KisResourceMetaDataModel storages(QStringLiteral("storages"));

    QCOMPARE(resources.metaDataValue(17, QStringLiteral("answer")), QVariant(42));
    QCOMPARE(resources.metaDataValue(17, QStringLiteral("label")),
             QVariant(QStringLiteral("Brush")));
    QCOMPARE(resources.metaDataValue(23, QStringLiteral("answer")), QVariant(84));
    QCOMPARE(storages.metaDataValue(17, QStringLiteral("answer")), QVariant(126));
}

void KisResourceMetaDataModelContractTest::returnsInvalidValueForMissingOrEmptyData()
{
    KisResourceMetaDataModel resources(QStringLiteral("resources"));
    KisResourceMetaDataModel unknownTable(QStringLiteral("unknown"));

    QVERIFY(!resources.metaDataValue(99, QStringLiteral("answer")).isValid());
    QVERIFY(!resources.metaDataValue(17, QStringLiteral("missing")).isValid());
    QVERIFY(!resources.metaDataValue(17, QStringLiteral("empty")).isValid());
    QVERIFY(!unknownTable.metaDataValue(17, QStringLiteral("answer")).isValid());
}

void KisResourceMetaDataModelContractTest::releasesPreparedQueryBeforeSchemaReplacement()
{
    {
        KisResourceMetaDataModel resources(QStringLiteral("resources"));
        QCOMPARE(resources.metaDataValue(17, QStringLiteral("answer")), QVariant(42));
    }

    QSqlQuery query;
    QVERIFY(query.exec(QStringLiteral("DROP TABLE metadata")));
    QVERIFY(createMetadataTable());
}

void KisResourceMetaDataModelContractTest::cleanupTestCase()
{
    {
        QSqlDatabase database = QSqlDatabase::database();
        database.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

QTEST_GUILESS_MAIN(KisResourceMetaDataModelContractTest)

#include "KisResourceMetaDataModelContractTest.moc"
