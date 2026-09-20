/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only
 */

#include <kstandardaction.h>

#include <QAction>
#include <QSet>
#include <QTest>

// Compatibility requirement: LibrePaint action XML and plugins depend on KStandardAction::name() identifiers.

class ActionReceiver : public QObject
{
    Q_OBJECT

public:
    bool triggered {false};

public Q_SLOTS:
    void receiveAction()
    {
        triggered = true;
    }
};

class KStandardActionCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void registeredActionsExposeUniqueIdentifiers();
    void createdActionKeepsIdentifierAndDispatchesActivation();
};

void KStandardActionCompatibilityTest::registeredActionsExposeUniqueIdentifiers()
{
    const QList<KStandardAction::StandardAction> actionIds = KStandardAction::actionIds();
    QVERIFY(!actionIds.isEmpty());
    QVERIFY(!actionIds.contains(KStandardAction::ActionNone));

    QSet<QString> names;
    for (const KStandardAction::StandardAction actionId : actionIds) {
        const char *actionName = KStandardAction::name(actionId);
        QVERIFY(actionName);
        QVERIFY(*actionName != '\0');

        const QString name = QString::fromLatin1(actionName);
        QVERIFY(!names.contains(name));
        names.insert(name);
    }

    QCOMPARE(QString::fromLatin1(KStandardAction::name(KStandardAction::New)),
             QStringLiteral("file_new"));
    QCOMPARE(QString::fromLatin1(KStandardAction::name(KStandardAction::Open)),
             QStringLiteral("file_open"));
    QCOMPARE(QString::fromLatin1(KStandardAction::name(KStandardAction::Undo)),
             QStringLiteral("edit_undo"));
    QCOMPARE(QString::fromLatin1(KStandardAction::name(KStandardAction::Redo)),
             QStringLiteral("edit_redo"));
    QCOMPARE(QString::fromLatin1(KStandardAction::name(KStandardAction::ZoomIn)),
             QStringLiteral("view_zoom_in"));
}

void KStandardActionCompatibilityTest::createdActionKeepsIdentifierAndDispatchesActivation()
{
    ActionReceiver receiver;
    QAction *action = KStandardAction::create(KStandardAction::New,
                                                &receiver,
                                                SLOT(receiveAction()),
                                                &receiver);

    QVERIFY(action);
    QCOMPARE(action->objectName(), QStringLiteral("file_new"));
    QVERIFY(!action->text().isEmpty());

    action->trigger();

    QVERIFY(receiver.triggered);
}

QTEST_MAIN(KStandardActionCompatibilityTest)

#include "KStandardActionCompatibilityTest.moc"
