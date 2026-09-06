/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisActionsSnapshot.h>
#include <config/khelpclient.h>
#include <katecommandbar.h>
#include <xmlgui/kundoactions.h>

#include <QTest>

#include <type_traits>

class KisActionUtilitySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionsSnapshotTypeLifetimeAndConstructionSchemaRemainStable();
    void actionsSnapshotAccessSchemaRemainsStable();
    void commandBarTypeAndConstructionSchemaRemainStable();
    void commandBarUpdateSchemaRemainsStable();
    void helpAndUndoActionFactorySchemaRemainStable();
};

void KisActionUtilitySchemaContractTest::actionsSnapshotTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisActionsSnapshot>);
    static_assert(std::is_default_constructible_v<KisActionsSnapshot>);
    static_assert(std::is_destructible_v<KisActionsSnapshot>);
}

void KisActionUtilitySchemaContractTest::actionsSnapshotAccessSchemaRemainsStable()
{
    using AddActionSignature = void (KisActionsSnapshot::*)(const QString &, QAction *);
    using ActionCollectionsSignature = QMap<QString, KisKActionCollection *> (KisActionsSnapshot::*)();

    static_assert(std::is_same_v<decltype(&KisActionsSnapshot::addAction), AddActionSignature>);
    static_assert(std::is_same_v<decltype(&KisActionsSnapshot::actionCollections), ActionCollectionsSignature>);
}

void KisActionUtilitySchemaContractTest::commandBarTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KateCommandBar>);
    static_assert(std::is_constructible_v<KateCommandBar, QWidget *>);
}

void KisActionUtilitySchemaContractTest::commandBarUpdateSchemaRemainsStable()
{
    using UpdateBarSignature = void (KateCommandBar::*)(const QList<KisKActionCollection *> &, int);
    using UpdateViewGeometrySignature = void (KateCommandBar::*)();

    static_assert(std::is_same_v<decltype(&KateCommandBar::updateBar), UpdateBarSignature>);
    static_assert(std::is_same_v<decltype(&KateCommandBar::updateViewGeometry), UpdateViewGeometrySignature>);
}

void KisActionUtilitySchemaContractTest::helpAndUndoActionFactorySchemaRemainStable()
{
    using InvokeHelpSignature = void (*)(const QString &, const QString &);
    using UndoActionFactorySignature = QAction *(*)(QUndoStack *, KisKActionCollection *, const QString &);

    static_assert(std::is_same_v<decltype(&KHelpClient::invokeHelp), InvokeHelpSignature>);
    static_assert(std::is_same_v<decltype(&KisKUndoActions::createRedoAction), UndoActionFactorySignature>);
    static_assert(std::is_same_v<decltype(&KisKUndoActions::createUndoAction), UndoActionFactorySignature>);
}

QTEST_GUILESS_MAIN(KisActionUtilitySchemaContractTest)

#include "KisActionUtilitySchemaContractTest.moc"
