/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImageSignals.h"

#include <QTest>

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

class KisImageSignalsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void signalTypesPreserveValues();
    void sizeChangesPreserveStillPoints();
    void nodeReselectionPreservesBeforeAndAfterState();
    void signalWrappersPreservePayloadAndOrder();
};

void KisImageSignalsContractTest::signalTypesPreserveValues()
{
    QCOMPARE(static_cast<int>(LayersChangedSignal), 0);
    QCOMPARE(static_cast<int>(ModifiedWithoutUndoSignal), 1);
    QCOMPARE(static_cast<int>(SizeChangedSignal), 2);
    QCOMPARE(static_cast<int>(ProfileChangedSignal), 3);
    QCOMPARE(static_cast<int>(ColorSpaceChangedSignal), 4);
    QCOMPARE(static_cast<int>(ResolutionChangedSignal), 5);
    QCOMPARE(static_cast<int>(NodeReselectionRequestSignal), 6);
}

void KisImageSignalsContractTest::sizeChangesPreserveStillPoints()
{
    const ComplexSizeChangedSignal emptySignal;
    QCOMPARE(emptySignal.oldStillPoint, QPointF());
    QCOMPARE(emptySignal.newStillPoint, QPointF());

    const ComplexSizeChangedSignal explicitSignal(QPointF(1.5, 2.5), QPointF(7.5, 8.5));
    QCOMPARE(explicitSignal.oldStillPoint, QPointF(1.5, 2.5));
    QCOMPARE(explicitSignal.newStillPoint, QPointF(7.5, 8.5));

    const ComplexSizeChangedSignal invertedSignal = explicitSignal.inverted();
    QCOMPARE(invertedSignal.oldStillPoint, explicitSignal.newStillPoint);
    QCOMPARE(invertedSignal.newStillPoint, explicitSignal.oldStillPoint);

    const ComplexSizeChangedSignal resizedSignal(QRect(10, 20, 30, 40), QSize(100, 60));
    QCOMPARE(resizedSignal.oldStillPoint, QPointF(25.0, 40.0));
    QCOMPARE(resizedSignal.newStillPoint, QPointF(50.0, 30.0));
}

void KisImageSignalsContractTest::nodeReselectionPreservesBeforeAndAfterState()
{
    alignas(void *) unsigned char newNodeStorage = 0;
    alignas(void *) unsigned char oldNodeStorage = 0;
    KisNodeSP newNode(reinterpret_cast<KisNode *>(&newNodeStorage));
    KisNodeSP oldNode(reinterpret_cast<KisNode *>(&oldNodeStorage));

    const ComplexNodeReselectionSignal emptySignal;
    QVERIFY(emptySignal.newActiveNode.isNull());
    QVERIFY(emptySignal.newSelectedNodes.isEmpty());
    QVERIFY(emptySignal.oldActiveNode.isNull());
    QVERIFY(emptySignal.oldSelectedNodes.isEmpty());

    const KisNodeList newSelection{newNode, oldNode};
    const KisNodeList oldSelection{oldNode};
    const ComplexNodeReselectionSignal signal(newNode, newSelection, oldNode, oldSelection);
    QCOMPARE(signal.newActiveNode.data(), newNode.data());
    QCOMPARE(signal.newSelectedNodes, newSelection);
    QCOMPARE(signal.oldActiveNode.data(), oldNode.data());
    QCOMPARE(signal.oldSelectedNodes, oldSelection);

    const ComplexNodeReselectionSignal invertedSignal = signal.inverted();
    QCOMPARE(invertedSignal.newActiveNode.data(), oldNode.data());
    QCOMPARE(invertedSignal.newSelectedNodes, oldSelection);
    QCOMPARE(invertedSignal.oldActiveNode.data(), newNode.data());
    QCOMPARE(invertedSignal.oldSelectedNodes, newSelection);
}

void KisImageSignalsContractTest::signalWrappersPreservePayloadAndOrder()
{
    KisImageSignalType defaultSignal;
    QCOMPARE(defaultSignal.sizeChangedSignal.oldStillPoint, QPointF());
    QVERIFY(defaultSignal.nodeReselectionSignal.newActiveNode.isNull());

    const KisImageSignalType profileSignal(ProfileChangedSignal);
    QCOMPARE(profileSignal.id, ProfileChangedSignal);

    const ComplexSizeChangedSignal sizeChange(QPointF(2.0, 3.0), QPointF(5.0, 7.0));
    const KisImageSignalType sizeSignal(sizeChange);
    QCOMPARE(sizeSignal.id, SizeChangedSignal);
    QCOMPARE(sizeSignal.sizeChangedSignal.oldStillPoint, sizeChange.oldStillPoint);
    QCOMPARE(sizeSignal.sizeChangedSignal.newStillPoint, sizeChange.newStillPoint);

    const ComplexNodeReselectionSignal nodeChange;
    const KisImageSignalType nodeSignal(nodeChange);
    QCOMPARE(nodeSignal.id, NodeReselectionRequestSignal);
    QVERIFY(nodeSignal.nodeReselectionSignal.newActiveNode.isNull());

    const KisImageSignalType invertedSizeSignal = sizeSignal.inverted();
    QCOMPARE(invertedSizeSignal.id, SizeChangedSignal);
    QCOMPARE(invertedSizeSignal.sizeChangedSignal.oldStillPoint, sizeChange.newStillPoint);
    QCOMPARE(invertedSizeSignal.sizeChangedSignal.newStillPoint, sizeChange.oldStillPoint);

    const KisImageSignalVector signals{profileSignal, sizeSignal, nodeSignal};
    QCOMPARE(signals.size(), 3);
    QCOMPARE(signals.at(0).id, ProfileChangedSignal);
    QCOMPARE(signals.at(1).id, SizeChangedSignal);
    QCOMPARE(signals.at(2).id, NodeReselectionRequestSignal);
}

QTEST_GUILESS_MAIN(KisImageSignalsContractTest)

#include "KisImageSignalsContractTest.moc"
