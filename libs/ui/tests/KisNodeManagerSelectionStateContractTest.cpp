/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_manager.h"
#pragma pop_macro("Q_OBJECT")

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

namespace
{

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(token<KisNode>(id));
}

KisNodeList storedSelectedNodes;
int selectionStateWriteCount;
KisNodeList notifiedSelectedNodes;
int selectionNotificationCount;
bool stateMatchedAtNotification;
KisNodeSP activatedNodeValue;
int activationCount;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

void KisNodeManager::SelectionStateAccess::setSelectedNodes(KisNodeManager *, const KisNodeList &nodes)
{
    storedSelectedNodes = nodes;
    selectionStateWriteCount++;
}

void KisNodeManager::slotNonUiActivatedNode(KisNodeSP node)
{
    activatedNodeValue = node;
    activationCount++;
}

void KisNodeManager::sigUiNeedChangeSelectedNodes(const QList<KisNodeSP> &nodes)
{
    notifiedSelectedNodes = nodes;
    selectionNotificationCount++;
    stateMatchedAtNotification = storedSelectedNodes == nodes;
}

class KisNodeManagerSelectionStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void selectionUpdatesStateBeforeNotification();
    void imageReselectionSkipsEmptyInputs();
};

void KisNodeManagerSelectionStateContractTest::init()
{
    storedSelectedNodes.clear();
    selectionStateWriteCount = 0;
    notifiedSelectedNodes.clear();
    selectionNotificationCount = 0;
    stateMatchedAtNotification = false;
    activatedNodeValue.clear();
    activationCount = 0;
}

void KisNodeManagerSelectionStateContractTest::selectionUpdatesStateBeforeNotification()
{
    KisNodeManager manager(nullptr);
    const KisNodeList selectedNodes{nodeToken(1), nodeToken(2)};
    manager.slotSetSelectedNodes(selectedNodes);
    QCOMPARE(selectionStateWriteCount, 1);
    QCOMPARE(storedSelectedNodes, selectedNodes);
    QCOMPARE(selectionNotificationCount, 1);
    QCOMPARE(notifiedSelectedNodes, selectedNodes);
    QVERIFY(stateMatchedAtNotification);

    manager.slotSetSelectedNodes(KisNodeList());
    QCOMPARE(selectionStateWriteCount, 2);
    QVERIFY(storedSelectedNodes.isEmpty());
    QCOMPARE(selectionNotificationCount, 2);
    QVERIFY(notifiedSelectedNodes.isEmpty());
    QVERIFY(stateMatchedAtNotification);
}

void KisNodeManagerSelectionStateContractTest::imageReselectionSkipsEmptyInputs()
{
    KisNodeManager manager(nullptr);
    manager.slotImageRequestNodeReselection(KisNodeSP(), KisNodeList());
    QCOMPARE(activationCount, 0);
    QCOMPARE(selectionStateWriteCount, 0);

    const KisNodeSP activeNode = nodeToken(1);
    manager.slotImageRequestNodeReselection(activeNode, KisNodeList());
    QCOMPARE(activationCount, 1);
    QCOMPARE(activatedNodeValue, activeNode);
    QCOMPARE(selectionStateWriteCount, 0);

    const KisNodeList selectedNodes{nodeToken(2), nodeToken(3)};
    manager.slotImageRequestNodeReselection(KisNodeSP(), selectedNodes);
    QCOMPARE(activationCount, 1);
    QCOMPARE(selectionStateWriteCount, 1);
    QCOMPARE(storedSelectedNodes, selectedNodes);
}

QTEST_GUILESS_MAIN(KisNodeManagerSelectionStateContractTest)

#include "KisNodeManagerSelectionStateContractTest.moc"
