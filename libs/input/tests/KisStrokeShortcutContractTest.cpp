/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisInputAction.h>
#include <kis_stroke_shortcut.h>

#include <QMouseEvent>
#include <QTest>

#include <memory>

class StrokeInputAction final : public KisInputAction
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
    bool canIgnoreModifiers() const override { return ignoresModifiers; }
    bool isAvailable() const override { return true; }

    int priorityValue {17};
    bool ignoresModifiers {false};
};

class KisStrokeShortcutContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesPolymorphicLifetimeAndActionPriority();
    void configuredButtonsDefineReadyAndBeginTransitions();
    void priorityCombinesConfigurationAndActionPriority();
    void emptyButtonConfigurationDoesNotReplaceTheActiveConfiguration();
    void fakeEndEventUsesTheConfiguredButtonAndPosition();
};

void KisStrokeShortcutContractTest::preservesPolymorphicLifetimeAndActionPriority()
{
    StrokeInputAction action;
    std::unique_ptr<KisAbstractShortcut> shortcut(new KisStrokeShortcut(&action, 4));

    QCOMPARE(shortcut->priority(), action.priorityValue);
}

void KisStrokeShortcutContractTest::configuredButtonsDefineReadyAndBeginTransitions()
{
    StrokeInputAction action;
    KisStrokeShortcut shortcut(&action, 2);
    shortcut.setButtons({Qt::Key_Shift}, {Qt::LeftButton, Qt::RightButton});

    QVERIFY(shortcut.matchReady({Qt::Key_Shift}, {Qt::LeftButton}));
    QVERIFY(shortcut.matchReady({Qt::Key_Shift}, {Qt::RightButton}));
    QVERIFY(!shortcut.matchReady({}, {Qt::LeftButton}));
    QVERIFY(!shortcut.matchReady({Qt::Key_Shift}, {Qt::MiddleButton}));
    QVERIFY(!shortcut.matchReady({Qt::Key_Shift}, {}));

    QVERIFY(shortcut.matchBegin(Qt::LeftButton));
    QVERIFY(shortcut.matchBegin(Qt::RightButton));
    QVERIFY(!shortcut.matchBegin(Qt::MiddleButton));
}

void KisStrokeShortcutContractTest::priorityCombinesConfigurationAndActionPriority()
{
    StrokeInputAction action;
    KisStrokeShortcut shortcut(&action, 5);
    shortcut.setButtons({Qt::Key_Shift}, {Qt::LeftButton});
    const int configuredPriority = shortcut.priority();

    QVERIFY(configuredPriority > action.priorityValue);

    action.priorityValue = 29;
    QCOMPARE(shortcut.priority(), configuredPriority + 12);
}

void KisStrokeShortcutContractTest::emptyButtonConfigurationDoesNotReplaceTheActiveConfiguration()
{
    StrokeInputAction action;
    KisStrokeShortcut shortcut(&action, 6);
    shortcut.setButtons({}, {Qt::LeftButton});

    shortcut.setButtons({Qt::Key_Control}, {});

    QVERIFY(shortcut.matchReady({}, {}));
    QVERIFY(shortcut.matchBegin(Qt::LeftButton));
    QVERIFY(!shortcut.matchBegin(Qt::RightButton));
}

void KisStrokeShortcutContractTest::fakeEndEventUsesTheConfiguredButtonAndPosition()
{
    StrokeInputAction action;
    KisStrokeShortcut shortcut(&action, 8);
    shortcut.setButtons({}, {Qt::LeftButton});
    const QPointF localPosition(12.5, 8.25);

    const QMouseEvent event = shortcut.fakeEndEvent(localPosition);

    QCOMPARE(event.type(), QEvent::MouseButtonRelease);
    QCOMPARE(event.position(), localPosition);
    QCOMPARE(event.scenePosition(), localPosition);
    QCOMPARE(event.globalPosition(), localPosition);
    QCOMPARE(event.button(), Qt::LeftButton);
    QCOMPARE(event.buttons(), Qt::NoButton);
    QCOMPARE(event.modifiers(), Qt::NoModifier);
}

QTEST_GUILESS_MAIN(KisStrokeShortcutContractTest)

#include "KisStrokeShortcutContractTest.moc"
