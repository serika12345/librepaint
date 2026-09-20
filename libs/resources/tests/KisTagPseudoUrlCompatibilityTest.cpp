/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceCacheDb.h>
#include <KisResourceTypes.h>
#include <KisTagModel.h>

#include <simpletest.h>

#include <QSqlDatabase>
#include <QTemporaryDir>

class KisTagPseudoUrlCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void savedPseudoTagUrlsResolveToFilters_data();
    void savedPseudoTagUrlsResolveToFilters();
    void cleanupTestCase();

private:
    QTemporaryDir m_databaseDirectory;
};

void KisTagPseudoUrlCompatibilityTest::initTestCase()
{
    QVERIFY2(m_databaseDirectory.isValid(), "Could not create the temporary resource-cache directory");
    QVERIFY2(KisResourceCacheDb::initialize(m_databaseDirectory.path()),
             qPrintable(KisResourceCacheDb::lastError()));
}

void KisTagPseudoUrlCompatibilityTest::savedPseudoTagUrlsResolveToFilters_data()
{
    QTest::addColumn<QString>("savedUrl");
    QTest::addColumn<QString>("expectedName");

    QTest::newRow("all-resources") << QStringLiteral("All") << QStringLiteral("All");
    QTest::newRow("all-untagged-resources") << QStringLiteral("All untagged") << QStringLiteral("All untagged");
}

// Compatibility requirement: Existing SelectedTags settings depend on the All and All untagged pseudo-tag URLs.
void KisTagPseudoUrlCompatibilityTest::savedPseudoTagUrlsResolveToFilters()
{
    // Consumer: Users reopening LibrePaint with a saved tag selection.
    // Operation: The tag chooser resolves the URL saved in the SelectedTags configuration group.
    // Observable result: The saved pseudo-tag URL selects the all-resources or all-untagged-resources view.
    // Failure impact: The chooser cannot restore the user's saved resource filter after an update.
    QFETCH(QString, savedUrl);
    QFETCH(QString, expectedName);

    KisTagModel model(ResourceType::Brushes);
    const KisTagSP tag = model.tagForUrl(savedUrl);

    QVERIFY(tag);
    QVERIFY(tag->valid());
    QVERIFY(tag->active());
    QCOMPARE(tag->resourceType(), ResourceType::Brushes);
    QCOMPARE(tag->url(), savedUrl);
    QCOMPARE(tag->name(), expectedName);
}

void KisTagPseudoUrlCompatibilityTest::cleanupTestCase()
{
    const QString connectionName = QSqlDatabase::defaultConnection;
    {
        QSqlDatabase database = QSqlDatabase::database(connectionName);
        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

SIMPLE_TEST_MAIN(KisTagPseudoUrlCompatibilityTest)

#include "KisTagPseudoUrlCompatibilityTest.moc"
