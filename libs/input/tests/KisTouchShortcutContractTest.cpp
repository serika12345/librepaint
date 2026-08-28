/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisInputAction.h>
#include <kis_touch_shortcut.h>

#include <QEventPoint>
#include <QTest>
#include <QTouchEvent>

#include <memory>

class TouchInputAction final : public KisInputAction
{
public:
    void activate(int) override { }
    void deactivate(int) override { }
    void begin(int, QEvent *) override { }
    void end(QEvent *) override { }
    void inputEvent(QEvent *) override { }

    bool supportsHiResInputEvents(int) const override { return false; }
    KisInputActionGroup inputActionGroup(int) const override { return ViewTransformActionGroup; }
    int priority() const override { return priorityValue; }
    bool canIgnoreModifiers() const override { return false; }
    bool isAvailable() const override { return true; }

    int priorityValue {19};
};

namespace {

QTouchEvent touchEvent(int pointCount)
{
    QList<QEventPoint> points;
    for (int i = 0; i < pointCount; ++i) {
        points.append(QEventPoint(i));
    }
    return QTouchEvent(QEvent::TouchUpdate, nullptr, Qt::NoModifier, points);
}

void configurePointRange(KisTouchShortcut &shortcut, int minimum, int maximum)
{
    shortcut.setMinimumTouchPoints(minimum);
    shortcut.setMaximumTouchPoints(maximum);
}

}

class KisTouchShortcutContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesPolymorphicLifetimeAndActionPriority();
    void reportsHoldGestureType();
    void gesturePredicatesMatchOnlyTheirConfiguredType();
    void touchPointRangeIsInclusive();
    void touchPaintingStateCanDisableMatching();
};

void KisTouchShortcutContractTest::preservesPolymorphicLifetimeAndActionPriority()
{
    TouchInputAction action;
    std::unique_ptr<KisAbstractShortcut> shortcut(
        new KisTouchShortcut(&action, 4, KisTouchGestureType::Tap));

    QCOMPARE(shortcut->priority(), action.priorityValue);
}

void KisTouchShortcutContractTest::reportsHoldGestureType()
{
    TouchInputAction action;
    KisTouchShortcut hold(&action, 1, KisTouchGestureType::Hold);
    KisTouchShortcut tap(&action, 2, KisTouchGestureType::Tap);

    QVERIFY(hold.isHoldType());
    QVERIFY(!tap.isHoldType());
}

void KisTouchShortcutContractTest::gesturePredicatesMatchOnlyTheirConfiguredType()
{
    TouchInputAction action;
    KisTouchShortcut tap(&action, 1, KisTouchGestureType::Tap);
    KisTouchShortcut drag(&action, 2, KisTouchGestureType::Drag);
    KisTouchShortcut hold(&action, 3, KisTouchGestureType::Hold);
    configurePointRange(tap, 2, 2);
    configurePointRange(drag, 2, 2);
    configurePointRange(hold, 2, 2);
    QTouchEvent event = touchEvent(2);

    QVERIFY(tap.matchTapType(&event));
    QVERIFY(!tap.matchDragType(&event));
    QVERIFY(!tap.matchHoldType(&event));

    QVERIFY(!drag.matchTapType(&event));
    QVERIFY(drag.matchDragType(&event));
    QVERIFY(!drag.matchHoldType(&event));

    QVERIFY(!hold.matchTapType(&event));
    QVERIFY(!hold.matchDragType(&event));
    QVERIFY(hold.matchHoldType(&event));
}

void KisTouchShortcutContractTest::touchPointRangeIsInclusive()
{
    TouchInputAction action;
    KisTouchShortcut shortcut(&action, 5, KisTouchGestureType::Tap);
    configurePointRange(shortcut, 2, 3);
    QTouchEvent onePoint = touchEvent(1);
    QTouchEvent twoPoints = touchEvent(2);
    QTouchEvent threePoints = touchEvent(3);
    QTouchEvent fourPoints = touchEvent(4);

    QVERIFY(!shortcut.matchTouchPoint(&onePoint));
    QVERIFY(shortcut.matchTouchPoint(&twoPoints));
    QVERIFY(shortcut.matchTouchPoint(&threePoints));
    QVERIFY(!shortcut.matchTouchPoint(&fourPoints));
}

void KisTouchShortcutContractTest::touchPaintingStateCanDisableMatching()
{
    TouchInputAction action;
    KisTouchShortcut shortcut(&action, 6, KisTouchGestureType::Tap);
    configurePointRange(shortcut, 1, 1);
    bool paintingActive = false;
    shortcut.setDisabledWhenTouchPaintingActive(true);
    shortcut.setTouchPaintingActiveCallback([&paintingActive]() { return paintingActive; });
    QTouchEvent event = touchEvent(1);

    QVERIFY(shortcut.matchTouchPoint(&event));

    paintingActive = true;
    QVERIFY(!shortcut.matchTouchPoint(&event));

    shortcut.setTouchPaintingActiveCallback({});
    QVERIFY(shortcut.matchTouchPoint(&event));

    shortcut.setDisabledWhenTouchPaintingActive(false);
    QVERIFY(shortcut.matchTouchPoint(&event));
}

QTEST_GUILESS_MAIN(KisTouchShortcutContractTest)

#include "KisTouchShortcutContractTest.moc"
