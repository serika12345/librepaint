/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QEventPoint>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPointingDevice>
#include <QTest>
#include <QTouchEvent>

#include <KisInputAction.h>
#include <KisInputActionGroup.h>
#include <KisTouchGestureType.h>
#include <kis_native_gesture_shortcut.h>
#include <kis_shortcut_matcher.h>
#include <kis_single_action_shortcut.h>
#include <kis_stroke_shortcut.h>
#include <kis_touch_shortcut.h>

#include <memory>

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
        return supportsHiRes;
    }

    KisInputActionGroup inputActionGroup(int) const override
    {
        return group;
    }

    int priority() const override
    {
        return priorityValue;
    }

    bool canIgnoreModifiers() const override
    {
        return ignoresModifiers;
    }

    bool isAvailable() const override
    {
        return available;
    }

    QStringList events;
    KisInputActionGroup group {ModifyingActionGroup};
    bool available {true};
    bool supportsHiRes {true};
    bool ignoresModifiers {false};
    int priorityValue {0};
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
    void dispatchesAtomicKeyAndWheelActionsWithSuppressions();
    void synchronizesAndReinitializesModifierState();
    void reinitializesTrackedButtons();
    void toolActivationReevaluatesSuppressionAndMaskGatesRunning();
    void replaysTouchHoldLifecycle();
    void cancelsTouchHoldLifecycle();
    void replaysNativeGestureLifecycle();
};

static KisStrokeShortcut *createStrokeShortcut(KisInputAction *action)
{
    auto *shortcut = new KisStrokeShortcut(action, 7);
    shortcut->setButtons({Qt::Key_Shift}, {Qt::LeftButton});
    return shortcut;
}

static QTouchEvent touchEvent(QEvent::Type type, int pointCount)
{
    QList<QEventPoint> points;
    for (int i = 0; i < pointCount; ++i) {
        points.append(QEventPoint(i));
    }
    return QTouchEvent(type, nullptr, Qt::NoModifier, points);
}

static QNativeGestureEvent nativeGestureEvent(Qt::NativeGestureType type)
{
    return QNativeGestureEvent(type,
                               QPointingDevice::primaryPointingDevice(),
                               2,
                               QPointF(),
                               QPointF(),
                               QPointF(),
                               1.0,
                               QPointF(),
                               1);
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

void TestInputShortcutMatcher::dispatchesAtomicKeyAndWheelActionsWithSuppressions()
{
    RecordingInputAction keyAction;
    KisShortcutMatcher keyMatcher;
    auto *keyShortcut = new KisSingleActionShortcut(&keyAction, 11);
    keyShortcut->setKey({}, Qt::Key_K);
    keyMatcher.addShortcut(keyShortcut);

    QVERIFY(keyMatcher.keyPressed(Qt::Key_K));
    QVERIFY(keyMatcher.autoRepeatedKeyPressed(Qt::Key_K));
    QVERIFY(!keyMatcher.keyReleased(Qt::Key_K));
    QCOMPARE(keyAction.events.count(QStringLiteral("begin:11")), 2);
    QCOMPARE(keyAction.events.count(QStringLiteral("end")), 2);

    keyMatcher.suppressAllKeyboardActions(true);
    QVERIFY(!keyMatcher.keyPressed(Qt::Key_K));
    keyMatcher.keyReleased(Qt::Key_K);

    keyMatcher.suppressAllKeyboardActions(false);
    keyMatcher.suppressConflictingKeyActions({QKeySequence(Qt::Key_K)});
    QVERIFY(!keyMatcher.keyPressed(Qt::Key_K));
    keyMatcher.keyReleased(Qt::Key_K);

    keyMatcher.suppressConflictingKeyActions({});
    keyMatcher.suppressAllActions(true);
    QVERIFY(!keyMatcher.keyPressed(Qt::Key_K));
    keyMatcher.keyReleased(Qt::Key_K);

    keyMatcher.suppressAllActions(false);
    QVERIFY(keyMatcher.keyPressed(Qt::Key_K));
    keyMatcher.keyReleased(Qt::Key_K);

    keyMatcher.clearShortcuts();
    QVERIFY(!keyMatcher.keyPressed(Qt::Key_K));
    keyMatcher.keyReleased(Qt::Key_K);

    RecordingInputAction wheelAction;
    KisShortcutMatcher wheelMatcher;
    auto *wheelShortcut = new KisSingleActionShortcut(&wheelAction, 12);
    wheelShortcut->setWheel({}, KisSingleActionShortcut::WheelUp);
    wheelMatcher.addShortcut(wheelShortcut);
    wheelMatcher.suppressAllKeyboardActions(true);

    QVERIFY(wheelMatcher.wheelEvent(KisSingleActionShortcut::WheelUp, nullptr));
    QVERIFY(!wheelMatcher.wheelEvent(KisSingleActionShortcut::WheelDown, nullptr));
    QCOMPARE(wheelAction.events,
             QStringList({QStringLiteral("begin:12"), QStringLiteral("end")}));
}

void TestInputShortcutMatcher::synchronizesAndReinitializesModifierState()
{
    KisShortcutMatcher matcher;

    matcher.keyPressed(Qt::Key_Shift);
    matcher.keyPressed(Qt::Key_Control);
    const QVector<Qt::Key> pressedKeys = matcher.debugPressedKeys();
    QCOMPARE(pressedKeys.size(), 2);
    QVERIFY(pressedKeys.contains(Qt::Key_Shift));
    QVERIFY(pressedKeys.contains(Qt::Key_Control));
    QVERIFY(matcher.sanityCheckModifiersCorrectness(Qt::ShiftModifier | Qt::ControlModifier));
    QVERIFY(!matcher.sanityCheckModifiersCorrectness(Qt::ShiftModifier));

    matcher.reinitialize();
    QVERIFY(matcher.debugPressedKeys().isEmpty());
    QVERIFY(matcher.sanityCheckModifiersCorrectness(Qt::NoModifier));

    matcher.handlePolledKeys({Qt::Key_Alt, Qt::Key_Meta});
    QVERIFY(matcher.hasPolledKeys());
    const QVector<Qt::Key> polledKeys = matcher.debugPressedKeys();
    QCOMPARE(polledKeys.size(), 2);
    QVERIFY(polledKeys.contains(Qt::Key_Alt));
    QVERIFY(polledKeys.contains(Qt::Key_Meta));

    matcher.handlePolledKeys({});
    QVERIFY(!matcher.hasPolledKeys());
    QVERIFY(matcher.debugPressedKeys().isEmpty());
}

void TestInputShortcutMatcher::reinitializesTrackedButtons()
{
    RecordingInputAction action;
    KisShortcutMatcher matcher;
    auto *shortcut = new KisStrokeShortcut(&action, 13);
    shortcut->setButtons({}, {Qt::LeftButton, Qt::RightButton});
    matcher.addShortcut(shortcut);
    matcher.enterEvent();
    QMouseEvent leftPress(QEvent::MouseButtonPress,
                          QPointF(),
                          QPointF(),
                          QPointF(),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
    QMouseEvent rightPress(QEvent::MouseButtonPress,
                           QPointF(),
                           QPointF(),
                           QPointF(),
                           Qt::RightButton,
                           Qt::RightButton,
                           Qt::NoModifier);

    QVERIFY(!matcher.buttonPressed(Qt::LeftButton, &leftPress));
    QCOMPARE(action.events, QStringList({QStringLiteral("activate:13")}));

    matcher.reinitializeButtons();
    QCOMPARE(action.events,
             QStringList({QStringLiteral("activate:13"), QStringLiteral("deactivate:13")}));

    QVERIFY(!matcher.buttonPressed(Qt::RightButton, &rightPress));
    QVERIFY(!matcher.hasRunningShortcut());
}

void TestInputShortcutMatcher::toolActivationReevaluatesSuppressionAndMaskGatesRunning()
{
    RecordingInputAction action;
    KisShortcutMatcher matcher;
    KisInputActionGroupsMask mask = AllActionGroup;
    matcher.setInputActionGroupsMaskCallback([&mask]() { return mask; });
    matcher.addShortcut(createStrokeShortcut(&action));
    matcher.suppressAllActions(true);
    matcher.enterEvent();
    matcher.keyPressed(Qt::Key_Shift);

    QCOMPARE(action.events, QStringList());

    matcher.suppressAllActions(false);
    matcher.toolHasBeenActivated();
    QCOMPARE(action.events, QStringList({QStringLiteral("activate:7")}));

    matcher.leaveEvent();
    QCOMPARE(action.events,
             QStringList({QStringLiteral("activate:7"), QStringLiteral("deactivate:7")}));

    mask = NoActionGroup;
    matcher.enterEvent();
    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(),
                      QPointF(),
                      QPointF(),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::ShiftModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(),
                        QPointF(),
                        QPointF(),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::ShiftModifier);
    const int beginCount = action.events.count(QStringLiteral("begin:7"));

    QVERIFY(!matcher.buttonPressed(Qt::LeftButton, &press));
    QCOMPARE(action.events.count(QStringLiteral("begin:7")), beginCount);
    matcher.buttonReleased(Qt::LeftButton, &release);

    mask = AllActionGroup;
    QVERIFY(matcher.buttonPressed(Qt::LeftButton, &press));
    QCOMPARE(action.events.count(QStringLiteral("begin:7")), beginCount + 1);
    QVERIFY(matcher.buttonReleased(Qt::LeftButton, &release));
    matcher.keyReleased(Qt::Key_Shift);
}

void TestInputShortcutMatcher::replaysTouchHoldLifecycle()
{
    QCOMPARE(KisShortcutMatcher::TOUCH_SLOP_SQUARED, 256);
    RecordingInputAction action;
    action.group = ViewTransformActionGroup;
    KisShortcutMatcher matcher;
    QVERIFY(!matcher.hasTouchHoldShortcut());
    auto *shortcut = new KisTouchShortcut(&action, 21, KisTouchGestureType::Hold);
    shortcut->setMinimumTouchPoints(1);
    shortcut->setMaximumTouchPoints(1);
    matcher.addShortcut(shortcut);
    QVERIFY(matcher.hasTouchHoldShortcut());
    matcher.enterEvent();
    QTouchEvent begin = touchEvent(QEvent::TouchBegin, 1);
    QTouchEvent update = touchEvent(QEvent::TouchUpdate, 1);
    QTouchEvent end = touchEvent(QEvent::TouchEnd, 1);

    QVERIFY(matcher.touchBeginEvent(&begin));
    QVERIFY(matcher.touchHoldBeginEvent(&begin));
    QVERIFY(matcher.hasRunningShortcut());
    QVERIFY(matcher.supportsHiResInputEvents());
    QVERIFY(matcher.touchUpdateEvent(&update));
    QVERIFY(matcher.touchEndEvent(&end));
    QVERIFY(!matcher.hasRunningShortcut());
    QCOMPARE(action.events,
             QStringList({QStringLiteral("activate:21"),
                          QStringLiteral("begin:21"),
                          QStringLiteral("input"),
                          QStringLiteral("end"),
                          QStringLiteral("deactivate:21")}));
}

void TestInputShortcutMatcher::cancelsTouchHoldLifecycle()
{
    RecordingInputAction action;
    KisShortcutMatcher matcher;
    auto *shortcut = new KisTouchShortcut(&action, 22, KisTouchGestureType::Hold);
    shortcut->setMinimumTouchPoints(1);
    shortcut->setMaximumTouchPoints(1);
    matcher.addShortcut(shortcut);
    matcher.enterEvent();
    QTouchEvent begin = touchEvent(QEvent::TouchBegin, 1);
    QTouchEvent cancel = touchEvent(QEvent::TouchCancel, 0);

    QVERIFY(matcher.touchBeginEvent(&begin));
    QVERIFY(matcher.touchHoldBeginEvent(&begin));
    matcher.touchCancelEvent(&cancel);

    QVERIFY(!matcher.hasRunningShortcut());
    QCOMPARE(action.events.count(QStringLiteral("end")), 1);
    QCOMPARE(action.events.count(QStringLiteral("deactivate:22")), 1);
}

void TestInputShortcutMatcher::replaysNativeGestureLifecycle()
{
    RecordingInputAction action;
    std::unique_ptr<KisNativeGestureShortcut> shortcut(
        new KisNativeGestureShortcut(&action, 23, Qt::ZoomNativeGesture));
    KisShortcutMatcher matcher;
    matcher.addShortcut(shortcut.get());
    matcher.enterEvent();
    QNativeGestureEvent begin = nativeGestureEvent(Qt::BeginNativeGesture);
    QNativeGestureEvent zoom = nativeGestureEvent(Qt::ZoomNativeGesture);
    QNativeGestureEvent rotate = nativeGestureEvent(Qt::RotateNativeGesture);

    QVERIFY(matcher.nativeGestureBeginEvent(&begin));
    QVERIFY(!matcher.supportsHiResInputEvents());
    QVERIFY(matcher.nativeGestureEvent(&zoom));
    QVERIFY(matcher.hasRunningShortcut());
    QVERIFY(matcher.supportsHiResInputEvents());
    QVERIFY(matcher.nativeGestureEvent(&zoom));
    QVERIFY(matcher.nativeGestureEndEvent(&rotate));
    QVERIFY(!matcher.hasRunningShortcut());
    QVERIFY(!matcher.supportsHiResInputEvents());
    QCOMPARE(action.events,
             QStringList({QStringLiteral("activate:23"),
                          QStringLiteral("begin:23"),
                          QStringLiteral("input"),
                          QStringLiteral("end"),
                          QStringLiteral("deactivate:23")}));
}

QTEST_GUILESS_MAIN(TestInputShortcutMatcher)

#include "TestInputShortcutMatcher.moc"
