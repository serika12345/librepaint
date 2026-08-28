/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisInputEventSuppressor.h>

class TestInputEventSuppressor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void replaysTabletMouseSuppression();
    void suppressesOneDelayedMousePress();
    void appliesSyntheticMousePolicy();
    void preservesSecondaryButtonWorkaround();
    void replaysTouchSuppression();
};

using EventType = KisInputEventSuppressor::EventType;
using Button = KisInputEventSuppressor::Button;
using Reason = KisInputEventSuppressor::SuppressionReason;

void TestInputEventSuppressor::replaysTabletMouseSuppression()
{
    KisInputEventSuppressor suppressor(false, true);

    QCOMPARE(suppressor.filter({EventType::MouseMove}), Reason::None);
    suppressor.startBlockingMouseEvents();
    QVERIFY(suppressor.isBlockingMouseEvents());
    QCOMPARE(suppressor.filter({EventType::MouseMove}), Reason::MouseEvent);
    QCOMPARE(suppressor.filter({EventType::MousePress, Button::Left}), Reason::MouseEvent);
    QCOMPARE(suppressor.filter({EventType::MouseRelease, Button::Left}), Reason::MouseEvent);
    QCOMPARE(suppressor.filter({EventType::MouseDoubleClick, Button::Left}), Reason::MouseEvent);

    suppressor.stopBlockingMouseEvents();
    QVERIFY(!suppressor.isBlockingMouseEvents());
    QCOMPARE(suppressor.filter({EventType::MouseMove}), Reason::None);
}

void TestInputEventSuppressor::suppressesOneDelayedMousePress()
{
    KisInputEventSuppressor suppressor(false, true);
    suppressor.suppressNextLeftMousePress();

    QCOMPARE(suppressor.filter({EventType::MousePress, Button::Other}), Reason::None);
    QCOMPARE(suppressor.filter({EventType::MouseMove}), Reason::None);
    QCOMPARE(suppressor.filter({EventType::MousePress, Button::Left}), Reason::DelayedMousePress);
    QCOMPARE(suppressor.filter({EventType::MousePress, Button::Left}), Reason::None);
}

void TestInputEventSuppressor::appliesSyntheticMousePolicy()
{
    KisInputEventSuppressor supported(false, true);
    supported.setSuppressSyntheticMouseEvents(true);
    QCOMPARE(supported.filter({EventType::MouseMove, Button::None, true}), Reason::MouseEvent);
    QCOMPARE(supported.filter({EventType::MouseMove, Button::None, false}), Reason::None);

    KisInputEventSuppressor unsupported(false, false);
    unsupported.setSuppressSyntheticMouseEvents(true);
    QCOMPARE(unsupported.filter({EventType::MouseMove, Button::None, true}), Reason::None);
}

void TestInputEventSuppressor::preservesSecondaryButtonWorkaround()
{
    KisInputEventSuppressor suppressor(true, true);
    suppressor.startBlockingMouseEvents();

    QCOMPARE(suppressor.filter({EventType::MousePress, Button::Other}), Reason::None);
    QCOMPARE(suppressor.filter({EventType::MouseRelease, Button::Other}), Reason::None);
    QCOMPARE(suppressor.filter({EventType::MouseDoubleClick, Button::Other}), Reason::None);
    QCOMPARE(suppressor.filter({EventType::MousePress, Button::Left}), Reason::MouseEvent);
    QCOMPARE(suppressor.filter({EventType::TabletPress, Button::Other}), Reason::SecondaryTabletButton);
    QCOMPARE(suppressor.filter({EventType::TabletRelease, Button::Other}), Reason::SecondaryTabletButton);
    QCOMPARE(suppressor.filter({EventType::TabletPress, Button::Left}), Reason::None);
}

void TestInputEventSuppressor::replaysTouchSuppression()
{
    KisInputEventSuppressor suppressor(false, true);

    QCOMPARE(suppressor.filter({EventType::TouchBegin}), Reason::None);
    suppressor.startBlockingTouchEvents();
    QCOMPARE(suppressor.filter({EventType::TouchBegin}), Reason::TouchBegin);
    QCOMPARE(suppressor.filter({EventType::Other}), Reason::None);
    suppressor.stopBlockingTouchEvents();
    QCOMPARE(suppressor.filter({EventType::TouchBegin}), Reason::None);
}

QTEST_GUILESS_MAIN(TestInputEventSuppressor)

#include "TestInputEventSuppressor.moc"
