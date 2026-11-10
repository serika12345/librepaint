/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include "../canvas/kis_canvas_frame_update_gate.h"

class KisCanvasFrameUpdateGateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void disabledDispatchesEveryRequest();
    void requestsWaitForThePaintInFlight();
    void frameSwappedAllowsThePendingDispatch();
    void refusesToDispatchBeforeThePreviousPaintFinishes();
    void resetForgetsTheFrame();
};

void KisCanvasFrameUpdateGateContractTest::disabledDispatchesEveryRequest()
{
    KisCanvasFrameUpdateGate gate;
    QVERIFY(!gate.isEnabled());
    QVERIFY(gate.requestUpdate());
    QVERIFY(gate.requestUpdate());
    QVERIFY(!gate.isInFlight());
    QVERIFY(!gate.hasPendingUpdate());
}

void KisCanvasFrameUpdateGateContractTest::requestsWaitForThePaintInFlight()
{
    KisCanvasFrameUpdateGate gate;
    gate.setEnabled(true);

    QVERIFY(gate.requestUpdate());
    QVERIFY(gate.isInFlight());

    QVERIFY(!gate.requestUpdate());
    QVERIFY(gate.isInFlight());
    QVERIFY(gate.hasPendingUpdate());
}

void KisCanvasFrameUpdateGateContractTest::frameSwappedAllowsThePendingDispatch()
{
    KisCanvasFrameUpdateGate gate;
    gate.setEnabled(true);

    QVERIFY(gate.requestUpdate());
    QVERIFY(!gate.requestUpdate());
    QVERIFY(!gate.requestUpdate());

    gate.frameSwapped();
    QVERIFY(!gate.isInFlight());
    QVERIFY(gate.hasPendingUpdate());

    QVERIFY(gate.takePendingUpdateForDispatch());
    QVERIFY(gate.isInFlight());
    QVERIFY(!gate.hasPendingUpdate());

    gate.frameSwapped();
    QVERIFY(!gate.takePendingUpdateForDispatch());
    QVERIFY(!gate.isInFlight());
}

void KisCanvasFrameUpdateGateContractTest::refusesToDispatchBeforeThePreviousPaintFinishes()
{
    KisCanvasFrameUpdateGate gate;
    gate.setEnabled(true);

    QVERIFY(gate.requestUpdate());
    QVERIFY(!gate.requestUpdate());

    QVERIFY(!gate.takePendingUpdateForDispatch());
    QVERIFY(gate.isInFlight());
    QVERIFY(gate.hasPendingUpdate());
}

void KisCanvasFrameUpdateGateContractTest::resetForgetsTheFrame()
{
    KisCanvasFrameUpdateGate gate;
    gate.setEnabled(true);

    QVERIFY(gate.requestUpdate());
    QVERIFY(!gate.requestUpdate());
    gate.reset();

    QVERIFY(!gate.isInFlight());
    QVERIFY(!gate.hasPendingUpdate());
    QVERIFY(gate.requestUpdate());
}

QTEST_GUILESS_MAIN(KisCanvasFrameUpdateGateContractTest)

#include "KisCanvasFrameUpdateGateContractTest.moc"
