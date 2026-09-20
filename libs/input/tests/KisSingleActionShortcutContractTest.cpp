/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisInputAction.h>
#include <kis_single_action_shortcut.h>

#include <QKeySequence>
#include <QTest>

#include <memory>

class SingleInputAction final : public KisInputAction
{
public:
    void activate(int) override { }
    void deactivate(int) override { }
    void begin(int, QEvent *) override { }
    void end(QEvent *) override { }
    void inputEvent(QEvent *) override { }

    bool supportsHiResInputEvents(int) const override { return false; }
    KisInputActionGroup inputActionGroup(int) const override { return ModifyingActionGroup; }
    int priority() const override { return priorityValue; }
    bool canIgnoreModifiers() const override { return false; }
    bool isAvailable() const override { return true; }

    int priorityValue {23};
};

class KisSingleActionShortcutContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesPolymorphicLifetimeAndSpecificityPriority();
    void keyConfigurationMatchesExactKeysAndModifiers();
    void wheelValuesAndConfigurationRemainStable();
    void keyConflictsIncludePrefixesAndRejectInvalidSequences();
};

void KisSingleActionShortcutContractTest::preservesPolymorphicLifetimeAndSpecificityPriority()
{
    SingleInputAction action;
    auto *single = new KisSingleActionShortcut(&action, 4);
    single->setKey({Qt::Key_Control, Qt::Key_Shift}, Qt::Key_K);
    std::unique_ptr<KisAbstractShortcut> shortcut(single);

    QCOMPARE(shortcut->priority(), action.priorityValue + 5);
}

void KisSingleActionShortcutContractTest::keyConfigurationMatchesExactKeysAndModifiers()
{
    SingleInputAction action;
    KisSingleActionShortcut shortcut(&action, 2);
    shortcut.setKey({Qt::Key_Control}, Qt::Key_K);

    QVERIFY(shortcut.match({Qt::Key_Control}, Qt::Key_K));
    QVERIFY(!shortcut.match({}, Qt::Key_K));
    QVERIFY(!shortcut.match({Qt::Key_Control}, Qt::Key_L));

    shortcut.setWheel({Qt::Key_Control}, KisSingleActionShortcut::WheelUp);
    QVERIFY(!shortcut.match({Qt::Key_Control}, Qt::Key_K));
}

void KisSingleActionShortcutContractTest::wheelValuesAndConfigurationRemainStable()
{
    using WheelAction = KisSingleActionShortcut::WheelAction;
    QCOMPARE(static_cast<int>(WheelAction::WheelUp), 0);
    QCOMPARE(static_cast<int>(WheelAction::WheelDown), 1);
    QCOMPARE(static_cast<int>(WheelAction::WheelLeft), 2);
    QCOMPARE(static_cast<int>(WheelAction::WheelRight), 3);
    QCOMPARE(static_cast<int>(WheelAction::WheelTrackpad), 4);

    SingleInputAction action;
    KisSingleActionShortcut shortcut(&action, 3);
    const WheelAction wheelActions[] = {
        WheelAction::WheelUp,
        WheelAction::WheelDown,
        WheelAction::WheelLeft,
        WheelAction::WheelRight,
        WheelAction::WheelTrackpad,
    };

    for (WheelAction wheelAction : wheelActions) {
        shortcut.setWheel({Qt::Key_Shift}, wheelAction);
        QVERIFY(shortcut.match({Qt::Key_Shift}, wheelAction));
        QVERIFY(!shortcut.match({}, wheelAction));
    }

    shortcut.setWheel({}, WheelAction::WheelUp);
    QVERIFY(!shortcut.match({}, WheelAction::WheelDown));
}

void KisSingleActionShortcutContractTest::keyConflictsIncludePrefixesAndRejectInvalidSequences()
{
    SingleInputAction action;
    KisSingleActionShortcut shortcut(&action, 5);
    shortcut.setKey({Qt::Key_Control}, Qt::Key_K);

    QVERIFY(shortcut.conflictsWith(QKeySequence(Qt::CTRL | Qt::Key_K)));
    QVERIFY(shortcut.conflictsWith(QKeySequence(Qt::Key_K)));
    QVERIFY(!shortcut.conflictsWith(QKeySequence(Qt::CTRL | Qt::Key_L)));
    QVERIFY(!shortcut.conflictsWith(QKeySequence()));
    QVERIFY(!shortcut.conflictsWith(QKeySequence(Qt::CTRL)));
}

QTEST_GUILESS_MAIN(KisSingleActionShortcutContractTest)

#include "KisSingleActionShortcutContractTest.moc"
