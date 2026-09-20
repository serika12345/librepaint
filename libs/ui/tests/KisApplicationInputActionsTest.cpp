/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <actions/input/KisApplicationInputActions.h>
#include <input/ui/kis_abstract_input_action.h>
#include <simpletest.h>

class KisApplicationInputActionsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void applicationActionsPreserveProfileIdentifiers();
};

void KisApplicationInputActionsTest::applicationActionsPreserveProfileIdentifiers()
{
    const QList<KisAbstractInputAction *> actions = createApplicationInputActions();
    QStringList actualActionIds;
    for (const KisAbstractInputAction *action : actions) {
        actualActionIds.append(action->id());
    }

    const QStringList expectedActionIds {
        QStringLiteral("Tool Invocation"),
        QStringLiteral("Alternate Invocation"),
        QStringLiteral("Change Primary Setting"),
        QStringLiteral("Pan Canvas"),
        QStringLiteral("Rotate Canvas"),
        QStringLiteral("Zoom Canvas"),
        QStringLiteral("Show Popup Widget"),
        QStringLiteral("Select Layer"),
        QStringLiteral("Exposure or Gamma"),
        QStringLiteral("Switch Time"),
        QStringLiteral("Zoom and Rotate Canvas"),
        QStringLiteral("Touch Gestures"),
    };
    QCOMPARE(actualActionIds, expectedActionIds);

    qDeleteAll(actions);
}

SIMPLE_TEST_MAIN(KisApplicationInputActionsTest)

#include "KisApplicationInputActionsTest.moc"
