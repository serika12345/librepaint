/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceCacheDb.h>

#include <simpletest.h>

#include <array>
#include <utility>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

class KisResourceStorageTypeCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void persistedStorageTypesRemainReadable();
    void cleanupTestCase();

private:
    QTemporaryDir m_databaseDirectory;
};

void KisResourceStorageTypeCompatibilityTest::initTestCase()
{
    QVERIFY2(m_databaseDirectory.isValid(), "Could not create the temporary resource-cache directory");
}

// Compatibility requirement: Existing resourcecache.sqlite databases depend on stable storage_types IDs and names.
void KisResourceStorageTypeCompatibilityTest::persistedStorageTypesRemainReadable()
{
    // Consumer: Users updating LibrePaint with a resource-cache database from an earlier run.
    // Operation: LibrePaint initializes resourcecache.sqlite and reads its persisted storage-type lookup rows.
    // Observable result: Each persisted ID resolves to the established untranslated storage-type name.
    // Failure impact: Resources can be assigned to the wrong storage kind or temporary resources can survive cleanup.
    QVERIFY2(KisResourceCacheDb::initialize(m_databaseDirectory.path()),
             qPrintable(KisResourceCacheDb::lastError()));
    QVERIFY(KisResourceCacheDb::isValid());

    const std::array<std::pair<int, QString>, 7> storageTypes{{
        {1, QStringLiteral("Unknown")},
        {2, QStringLiteral("Folder")},
        {3, QStringLiteral("Bundle")},
        {4, QStringLiteral("Adobe Brush Library")},
        {5, QStringLiteral("Adobe Style Library")},
        {6, QStringLiteral("Memory")},
        {7, QStringLiteral("Font Storage")},
    }};

    QSqlQuery query;
    QVERIFY(query.exec(QStringLiteral("SELECT id, name FROM storage_types ORDER BY id")));
    QVERIFY(query.lastError() == QSqlError());

    for (const auto &[expectedId, expectedName] : storageTypes) {
        QVERIFY(query.next());
        QCOMPARE(query.value(QStringLiteral("id")).toInt(), expectedId);
        QCOMPARE(query.value(QStringLiteral("name")).toString(), expectedName);
    }
    QVERIFY(!query.next());
}

void KisResourceStorageTypeCompatibilityTest::cleanupTestCase()
{
    const QString connectionName = QSqlDatabase::defaultConnection;
    {
        QSqlDatabase database = QSqlDatabase::database(connectionName);
        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

SIMPLE_TEST_MAIN(KisResourceStorageTypeCompatibilityTest)

#include "KisResourceStorageTypeCompatibilityTest.moc"
