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

enum OrderingEffect {
    Raise,
    Lower,
};

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(token<KisNode>(id));
}

KisNodeList selectedNodesValue;
KisNodeSP activeNodeValue;
bool canMoveValue = false;
QList<int> checkedNodeCounts;
KisNodeList checkedNodes;
QList<OrderingEffect> effects;
QList<int> operationNodeCounts;
KisNodeList operationNodes;
KisNodeList operationActiveNodes;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeList KisNodeManager::OrderingAccess::selectedNodes(KisNodeManager *)
{
    return selectedNodesValue;
}

bool KisNodeManager::OrderingAccess::canMoveLayers(KisNodeManager *, const KisNodeList &nodes)
{
    checkedNodeCounts.append(nodes.size());
    checkedNodes.append(nodes);
    return canMoveValue;
}

KisNodeSP KisNodeManager::OrderingAccess::activeNode(KisNodeManager *)
{
    return activeNodeValue;
}

void KisNodeManager::OrderingAccess::raiseNodes(KisNodeManager *, const KisNodeList &nodes, KisNodeSP activeNode)
{
    effects.append(Raise);
    operationNodeCounts.append(nodes.size());
    operationNodes.append(nodes);
    operationActiveNodes.append(activeNode);
}

void KisNodeManager::OrderingAccess::lowerNodes(KisNodeManager *, const KisNodeList &nodes, KisNodeSP activeNode)
{
    effects.append(Lower);
    operationNodeCounts.append(nodes.size());
    operationNodes.append(nodes);
    operationActiveNodes.append(activeNode);
}

class KisNodeManagerOrderingContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void orderingRequiresMovableSelection();
};

void KisNodeManagerOrderingContractTest::init()
{
    selectedNodesValue.clear();
    activeNodeValue.clear();
    canMoveValue = false;
    checkedNodeCounts.clear();
    checkedNodes.clear();
    effects.clear();
    operationNodeCounts.clear();
    operationNodes.clear();
    operationActiveNodes.clear();
}

void KisNodeManagerOrderingContractTest::orderingRequiresMovableSelection()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP first = nodeToken(1);
    const KisNodeSP second = nodeToken(2);
    selectedNodesValue = {first, second};
    activeNodeValue = second;

    manager.raiseNode();
    manager.lowerNode();

    QCOMPARE(checkedNodeCounts, (QList<int>{2, 2}));
    QCOMPARE(checkedNodes, (KisNodeList{first, second, first, second}));
    QVERIFY(effects.isEmpty());

    checkedNodeCounts.clear();
    checkedNodes.clear();
    canMoveValue = true;

    manager.raiseNode();
    manager.lowerNode();

    QCOMPARE(checkedNodeCounts, (QList<int>{2, 2}));
    QCOMPARE(checkedNodes, (KisNodeList{first, second, first, second}));
    QCOMPARE(effects, (QList<OrderingEffect>{Raise, Lower}));
    QCOMPARE(operationNodeCounts, (QList<int>{2, 2}));
    QCOMPARE(operationNodes, (KisNodeList{first, second, first, second}));
    QCOMPARE(operationActiveNodes, (KisNodeList{second, second}));
}

QTEST_GUILESS_MAIN(KisNodeManagerOrderingContractTest)

#include "KisNodeManagerOrderingContractTest.moc"
