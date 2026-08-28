/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_native_gesture_shortcut.h>

#include <QNativeGestureEvent>
#include <QPointingDevice>
#include <QTest>

#include <memory>

namespace {

QNativeGestureEvent gestureEvent(Qt::NativeGestureType type)
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

}

class KisNativeGestureShortcutContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void retainsNeutralPriorityThroughPolymorphicLifetime();
    void matchesOnlyTheConfiguredGestureType();
};

void KisNativeGestureShortcutContractTest::retainsNeutralPriorityThroughPolymorphicLifetime()
{
    std::unique_ptr<KisAbstractShortcut> shortcut(
        new KisNativeGestureShortcut(nullptr, 7, Qt::ZoomNativeGesture));

    QCOMPARE(shortcut->priority(), 0);
}

void KisNativeGestureShortcutContractTest::matchesOnlyTheConfiguredGestureType()
{
    KisNativeGestureShortcut shortcut(nullptr, 3, Qt::ZoomNativeGesture);
    QNativeGestureEvent zoom = gestureEvent(Qt::ZoomNativeGesture);
    QNativeGestureEvent rotate = gestureEvent(Qt::RotateNativeGesture);

    QVERIFY(shortcut.match(&zoom));
    QVERIFY(!shortcut.match(&rotate));
}

QTEST_GUILESS_MAIN(KisNativeGestureShortcutContractTest)

#include "KisNativeGestureShortcutContractTest.moc"
