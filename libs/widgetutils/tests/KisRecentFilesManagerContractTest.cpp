/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRecentFilesManager.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QThread>

class KisRecentFilesManagerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanup();

    void singletonAndEntriesPreserveValues();
    void nonGuiThreadCannotAccessSingleton();
    void clearRenewsAnEmptyList();
    void addPreservesOrderNamesAndLatestFirstView();
    void duplicateMovesTheEntryAndReportsReplacement();
    void removeReportsOnlyExistingEntries();

private:
    KisRecentFilesManager *manager() const;
    void drainPendingSave() const;
};

void KisRecentFilesManagerContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("LibrePaintContractTests"));
    QCoreApplication::setApplicationName(QStringLiteral("KisRecentFilesManagerContractTest"));

    manager()->clear();
    drainPendingSave();
}

void KisRecentFilesManagerContractTest::cleanup()
{
    manager()->clear();
    drainPendingSave();
}

void KisRecentFilesManagerContractTest::singletonAndEntriesPreserveValues()
{
    QCOMPARE(KisRecentFilesManager::instance(), manager());

    const QUrl url(QStringLiteral("https://example.invalid/paintings/value-entry.kra"));
    const QString displayName(QStringLiteral("Value entry"));
    const KisRecentFilesEntry entry{url, displayName};

    QCOMPARE(entry.m_url, url);
    QCOMPARE(entry.m_displayName, displayName);
}

void KisRecentFilesManagerContractTest::nonGuiThreadCannotAccessSingleton()
{
    KisRecentFilesManager *threadResult = manager();
    QThread *thread = QThread::create([&threadResult]() {
        threadResult = KisRecentFilesManager::instance();
    });

    QTest::ignoreMessage(QtWarningMsg, "KisRecentFilesManager::instance() called from non-GUI thread!");
    thread->start();
    QVERIFY(thread->wait());
    delete thread;

    QVERIFY(!threadResult);
}

void KisRecentFilesManagerContractTest::clearRenewsAnEmptyList()
{
    manager()->add(QUrl(QStringLiteral("https://example.invalid/paintings/to-clear.kra")));
    QSignalSpy renewedSpy(manager(), &KisRecentFilesManager::listRenewed);

    manager()->clear();

    QCOMPARE(renewedSpy.count(), 1);
    QVERIFY(manager()->recentFiles().isEmpty());
    QVERIFY(manager()->recentUrlsLatestFirst().isEmpty());
}

void KisRecentFilesManagerContractTest::addPreservesOrderNamesAndLatestFirstView()
{
    const QUrl first(QStringLiteral("https://example.invalid/paintings/first-study.kra"));
    const QUrl second(QStringLiteral("https://example.invalid/paintings/second-study.kra"));
    QSignalSpy addedSpy(manager(), &KisRecentFilesManager::fileAdded);

    manager()->add(first);
    manager()->add(second);

    QCOMPARE(addedSpy.count(), 2);
    QCOMPARE(addedSpy.at(0).at(0).toUrl(), first);
    QCOMPARE(addedSpy.at(1).at(0).toUrl(), second);

    const QVector<KisRecentFilesEntry> entries = manager()->recentFiles();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).m_url, first);
    QCOMPARE(entries.at(0).m_displayName, QStringLiteral("first-study.kra"));
    QCOMPARE(entries.at(1).m_url, second);
    QCOMPARE(entries.at(1).m_displayName, QStringLiteral("second-study.kra"));
    QCOMPARE(manager()->recentUrlsLatestFirst(), QList<QUrl>({second, first}));
}

void KisRecentFilesManagerContractTest::duplicateMovesTheEntryAndReportsReplacement()
{
    const QUrl first(QStringLiteral("https://example.invalid/paintings/duplicate-first.kra"));
    const QUrl second(QStringLiteral("https://example.invalid/paintings/duplicate-second.kra"));
    manager()->add(first);
    manager()->add(second);

    QStringList notifications;
    const QMetaObject::Connection removedConnection =
        connect(manager(), &KisRecentFilesManager::fileRemoved, this, [&notifications](const QUrl &url) {
            notifications.append(QStringLiteral("removed:") + url.fileName());
        });
    const QMetaObject::Connection addedConnection =
        connect(manager(), &KisRecentFilesManager::fileAdded, this, [&notifications](const QUrl &url) {
            notifications.append(QStringLiteral("added:") + url.fileName());
        });

    manager()->add(first);

    QCOMPARE(notifications,
             QStringList({QStringLiteral("removed:duplicate-first.kra"), QStringLiteral("added:duplicate-first.kra")}));
    const QVector<KisRecentFilesEntry> entries = manager()->recentFiles();
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).m_url, second);
    QCOMPARE(entries.at(1).m_url, first);

    disconnect(removedConnection);
    disconnect(addedConnection);
}

void KisRecentFilesManagerContractTest::removeReportsOnlyExistingEntries()
{
    const QUrl existing(QStringLiteral("https://example.invalid/paintings/existing.kra"));
    const QUrl retained(QStringLiteral("https://example.invalid/paintings/retained.kra"));
    const QUrl absent(QStringLiteral("https://example.invalid/paintings/absent.kra"));
    manager()->add(existing);
    manager()->add(retained);
    QSignalSpy removedSpy(manager(), &KisRecentFilesManager::fileRemoved);

    manager()->remove(existing);
    manager()->remove(absent);

    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.at(0).at(0).toUrl(), existing);
    const QVector<KisRecentFilesEntry> entries = manager()->recentFiles();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.at(0).m_url, retained);
}

KisRecentFilesManager *KisRecentFilesManagerContractTest::manager() const
{
    return KisRecentFilesManager::instance();
}

void KisRecentFilesManagerContractTest::drainPendingSave() const
{
    QCoreApplication::processEvents();
}

QTEST_MAIN(KisRecentFilesManagerContractTest)

#include "KisRecentFilesManagerContractTest.moc"
