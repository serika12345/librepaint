/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRecentFilesManager.h"
#include "krecentfilesaction.h"

#include <QActionGroup>
#include <QCoreApplication>
#include <QIcon>
#include <QMenu>
#include <QPixmap>
#include <QPointer>
#include <QSignalSpy>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTest>

class KRecentFilesActionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanup();

    void constructorsPreserveParentPresentationAndVirtualLifetime();
    void addActionAssociatesUrlAndEmitsSelection();
    void addActionIgnoresRequestedDisplayName();
    void removeActionReturnsTheLiveActionAndDropsSelection();
    void recentFilesModelPopulatesIconsLazily();

private:
    KisRecentFilesManager *manager() const;
    void drainPendingSave() const;
};

void KRecentFilesActionContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("LibrePaintContractTests"));
    QCoreApplication::setApplicationName(QStringLiteral("KRecentFilesActionContractTest"));

    manager()->clear();
    drainPendingSave();
}

void KRecentFilesActionContractTest::cleanup()
{
    manager()->clear();
    drainPendingSave();
}

void KRecentFilesActionContractTest::constructorsPreserveParentPresentationAndVirtualLifetime()
{
    auto *parent = new QObject;
    QPointer<KRecentFilesAction> parentOnly = new KRecentFilesAction(parent);
    QPointer<KRecentFilesAction> withText = new KRecentFilesAction(QStringLiteral("&Recent paintings"), parent);

    QPixmap iconPixmap(3, 3);
    iconPixmap.fill(QColor(37, 91, 149));
    const QIcon icon(iconPixmap);
    QPointer<KRecentFilesAction> withIcon = new KRecentFilesAction(icon, QStringLiteral("Recent with icon"), parent);

    QCOMPARE(parentOnly->parent(), parent);
    QVERIFY(parentOnly->menu());
    QCOMPARE(parentOnly->toolBarMode(), KSelectAction::MenuMode);
    QVERIFY(!parentOnly->isEnabled());
    QCOMPARE(parentOnly->menu()->actions().size(), 3);
    QCOMPARE(parentOnly->menu()->actions().at(0)->objectName(), QStringLiteral("no_entries"));
    QVERIFY(parentOnly->menu()->actions().at(0)->isVisible());
    QVERIFY(!parentOnly->menu()->actions().at(1)->isVisible());
    QVERIFY(!parentOnly->menu()->actions().at(2)->isVisible());

    QCOMPARE(withText->parent(), parent);
    QCOMPARE(withText->text(), QStringLiteral("&Recent paintings"));
    QCOMPARE(withIcon->parent(), parent);
    QCOMPARE(withIcon->text(), QStringLiteral("Recent with icon"));
    QCOMPARE(withIcon->icon().cacheKey(), icon.cacheKey());

    delete parent;
    QVERIFY(parentOnly.isNull());
    QVERIFY(withText.isNull());
    QVERIFY(withIcon.isNull());

    QPointer<KRecentFilesAction> polymorphicAction = new KRecentFilesAction(nullptr);
    KSelectAction *base = polymorphicAction.data();
    delete base;
    QVERIFY(polymorphicAction.isNull());
}

void KRecentFilesActionContractTest::addActionAssociatesUrlAndEmitsSelection()
{
    KRecentFilesAction recentAction(nullptr);
    QAction entry(QStringLiteral("Caller title"));
    entry.setActionGroup(recentAction.selectableActionGroup());
    const QUrl url(QStringLiteral("https://example.invalid/paintings/selected.kra"));
    QSignalSpy selectedSpy(&recentAction, &KRecentFilesAction::urlSelected);

    recentAction.addAction(&entry, url, QStringLiteral("Requested title"));

    QCOMPARE(recentAction.menu()->actions().first(), &entry);
    entry.trigger();
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(selectedSpy.at(0).at(0).toUrl(), url);
}

void KRecentFilesActionContractTest::addActionIgnoresRequestedDisplayName()
{
    KRecentFilesAction recentAction(nullptr);
    QAction entry(QStringLiteral("Caller title"));

    recentAction.addAction(&entry,
                           QUrl(QStringLiteral("https://example.invalid/paintings/name-is-ignored.kra")),
                           QStringLiteral("Requested display name"));

    QCOMPARE(entry.text(), QStringLiteral("Caller title"));
}

void KRecentFilesActionContractTest::removeActionReturnsTheLiveActionAndDropsSelection()
{
    KRecentFilesAction recentAction(nullptr);
    auto *entry = new QAction(QStringLiteral("Removable entry"));
    QPointer<QAction> guardedEntry = entry;
    entry->setActionGroup(recentAction.selectableActionGroup());
    const QUrl url(QStringLiteral("https://example.invalid/paintings/removable.kra"));
    recentAction.addAction(entry, url, QStringLiteral("Removable entry"));
    QSignalSpy selectedSpy(&recentAction, &KRecentFilesAction::urlSelected);

    QAction *removed = recentAction.removeAction(entry);

    QCOMPARE(removed, entry);
    QVERIFY(guardedEntry);
    QVERIFY(!recentAction.menu()->actions().contains(entry));
    entry->trigger();
    QCOMPARE(selectedSpy.count(), 0);

    delete entry;
    QVERIFY(guardedEntry.isNull());
}

void KRecentFilesActionContractTest::recentFilesModelPopulatesIconsLazily()
{
    const QUrl url(QStringLiteral("https://example.invalid/paintings/icon-model.kra"));
    manager()->add(url);
    KRecentFilesAction recentAction(nullptr);
    QCOMPARE(recentAction.selectableActionGroup()->actions().size(), 1);
    QAction *entry = recentAction.selectableActionGroup()->actions().constFirst();
    QVERIFY(entry->icon().isNull());

    QPixmap firstPixmap(4, 4);
    firstPixmap.fill(QColor(131, 47, 73));
    const QIcon firstIcon(firstPixmap);
    auto *item = new QStandardItem;
    item->setData(url);
    item->setIcon(firstIcon);
    QStandardItemModel model;
    model.appendRow(item);

    recentAction.setRecentFilesModel(&model);
    QVERIFY(entry->icon().isNull());

    QVERIFY(QMetaObject::invokeMethod(recentAction.menu(), "aboutToShow", Qt::DirectConnection));
    QCOMPARE(entry->icon().cacheKey(), firstIcon.cacheKey());

    QPixmap secondPixmap(4, 4);
    secondPixmap.fill(QColor(41, 137, 89));
    const QIcon secondIcon(secondPixmap);
    item->setIcon(secondIcon);
    QCOMPARE(entry->icon().cacheKey(), secondIcon.cacheKey());
}

KisRecentFilesManager *KRecentFilesActionContractTest::manager() const
{
    return KisRecentFilesManager::instance();
}

void KRecentFilesActionContractTest::drainPendingSave() const
{
    QCoreApplication::processEvents();
}

QTEST_MAIN(KRecentFilesActionContractTest)

#include "KRecentFilesActionContractTest.moc"
