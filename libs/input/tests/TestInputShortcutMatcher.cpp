/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QMouseEvent>
#include <QTest>

#include <KisInputAction.h>
#include <KisInputActionGroup.h>
#include <KisTouchGestureType.h>
#include <kis_shortcut_matcher.h>
#include <kis_stroke_shortcut.h>

class RecordingInputAction final : public KisInputAction
{
public:
    void activate(int shortcut) override
    {
        events.append(QStringLiteral("activate:%1").arg(shortcut));
    }

    void deactivate(int shortcut) override
    {
        events.append(QStringLiteral("deactivate:%1").arg(shortcut));
    }

    void begin(int shortcut, QEvent *) override
    {
        events.append(QStringLiteral("begin:%1").arg(shortcut));
    }

    void end(QEvent *) override
    {
        events.append(QStringLiteral("end"));
    }

    void inputEvent(QEvent *) override
    {
        events.append(QStringLiteral("input"));
    }

    bool supportsHiResInputEvents(int) const override
    {
        return true;
    }

    KisInputActionGroup inputActionGroup(int) const override
    {
        return ModifyingActionGroup;
    }

    int priority() const override
    {
        return 0;
    }

    bool canIgnoreModifiers() const override
    {
        return false;
    }

    bool isAvailable() const override
    {
        return true;
    }

    QStringList events;
};

class InputActionGroupsMaskState final
    : public KisInputActionGroupsMaskInterface
{
public:
    KisInputActionGroupsMask inputActionGroupsMask() const override
    {
        return mask;
    }

    void setInputActionGroupsMask(KisInputActionGroupsMask value) override
    {
        mask = value;
    }

    KisInputActionGroupsMask mask {AllActionGroup};
};

class TestInputShortcutMatcher : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void replaysMouseStroke();
    void cancelsIncompleteMouseStrokeOnFocusLoss();
    void restoresInputActionMask();
};

static KisStrokeShortcut *createStrokeShortcut(KisInputAction *action)
{
    auto *shortcut = new KisStrokeShortcut(action, 7);
    shortcut->setButtons({Qt::Key_Shift}, {Qt::LeftButton});
    return shortcut;
}

void TestInputShortcutMatcher::replaysMouseStroke()
{
    RecordingInputAction action;
    KisShortcutMatcher matcher;
    matcher.addShortcut(createStrokeShortcut(&action));
    matcher.enterEvent();

    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(10.0, 20.0),
                      QPointF(10.0, 20.0),
                      QPointF(10.0, 20.0),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::ShiftModifier);
    QMouseEvent move(QEvent::MouseMove,
                     QPointF(12.0, 24.0),
                     QPointF(12.0, 24.0),
                     QPointF(12.0, 24.0),
                     Qt::NoButton,
                     Qt::LeftButton,
                     Qt::ShiftModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(12.0, 24.0),
                        QPointF(12.0, 24.0),
                        QPointF(12.0, 24.0),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::ShiftModifier);

    QVERIFY(!matcher.keyPressed(Qt::Key_Shift));
    QVERIFY(matcher.buttonPressed(Qt::LeftButton, &press));
    QVERIFY(matcher.pointerMoved(&move));
    QVERIFY(matcher.buttonReleased(Qt::LeftButton, &release));
    QVERIFY(!matcher.keyReleased(Qt::Key_Shift));

    QCOMPARE(action.events,
             QStringList({QStringLiteral("activate:7"),
                          QStringLiteral("begin:7"),
                          QStringLiteral("input"),
                          QStringLiteral("end"),
                          QStringLiteral("deactivate:7"),
                          QStringLiteral("activate:7"),
                          QStringLiteral("deactivate:7")}));
}

void TestInputShortcutMatcher::cancelsIncompleteMouseStrokeOnFocusLoss()
{
    RecordingInputAction action;
    KisShortcutMatcher matcher;
    matcher.addShortcut(createStrokeShortcut(&action));
    matcher.enterEvent();

    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(10.0, 20.0),
                      QPointF(10.0, 20.0),
                      QPointF(10.0, 20.0),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::ShiftModifier);

    matcher.keyPressed(Qt::Key_Shift);
    QVERIFY(matcher.buttonPressed(Qt::LeftButton, &press));
    QVERIFY(matcher.hasRunningShortcut());

    matcher.lostFocusEvent(QPointF(14.0, 28.0));

    QVERIFY(!matcher.hasRunningShortcut());
    QCOMPARE(action.events.count(QStringLiteral("end")), 1);
    QCOMPARE(action.events.count(QStringLiteral("deactivate:7")), 1);
}

void TestInputShortcutMatcher::restoresInputActionMask()
{
    const KisInputActionGroupsMaskInterface::SharedInterface state =
        QSharedPointer<InputActionGroupsMaskState>::create();

    {
        KisInputActionGroupsMaskGuard guard(state, ViewTransformActionGroup);
        QCOMPARE(state->inputActionGroupsMask(),
                 KisInputActionGroupsMask(ViewTransformActionGroup));
    }

    QCOMPARE(state->inputActionGroupsMask(),
             KisInputActionGroupsMask(AllActionGroup));
}

QTEST_MAIN(TestInputShortcutMatcher)

#include "TestInputShortcutMatcher.moc"
