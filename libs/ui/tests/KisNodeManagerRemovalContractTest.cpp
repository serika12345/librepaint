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

KisNodeList selectedNodesValue;
KisNodeSP parentedNodeValue;
KisNodeSP parentNodeValue;
KisNodeSP activeNodeValue;
bool canModifyValue = false;
KisNodeList parentRequests;
QList<int> checkedNodeCounts;
KisNodeList checkedNodes;
QList<int> removedNodeCounts;
KisNodeList removedNodes;
KisNodeList removalActiveNodes;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeList KisNodeManager::RemovalAccess::selectedNodes(KisNodeManager *)
{
    return selectedNodesValue;
}

KisNodeSP KisNodeManager::RemovalAccess::parentNode(KisNodeSP node)
{
    parentRequests.append(node);
    return node == parentedNodeValue ? parentNodeValue : KisNodeSP();
}

bool KisNodeManager::RemovalAccess::canModifyLayers(KisNodeManager *, const KisNodeList &nodes)
{
    checkedNodeCounts.append(nodes.size());
    checkedNodes.append(nodes);
    return canModifyValue;
}

KisNodeSP KisNodeManager::RemovalAccess::activeNode(KisNodeManager *)
{
    return activeNodeValue;
}

void KisNodeManager::RemovalAccess::removeNodes(KisNodeManager *, const KisNodeList &nodes, KisNodeSP activeNode)
{
    removedNodeCounts.append(nodes.size());
    removedNodes.append(nodes);
    removalActiveNodes.append(activeNode);
}

class KisNodeManagerRemovalContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void singleRemovalRequiresParentAndPermission();
    void selectedRemovalUsesCurrentSelection();
};

void KisNodeManagerRemovalContractTest::init()
{
    selectedNodesValue.clear();
    parentedNodeValue.clear();
    parentNodeValue.clear();
    activeNodeValue.clear();
    canModifyValue = false;
    parentRequests.clear();
    checkedNodeCounts.clear();
    checkedNodes.clear();
    removedNodeCounts.clear();
    removedNodes.clear();
    removalActiveNodes.clear();
}

void KisNodeManagerRemovalContractTest::singleRemovalRequiresParentAndPermission()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP root = nodeToken(1);
    const KisNodeSP child = nodeToken(2);
    const KisNodeSP active = nodeToken(3);
    parentedNodeValue = child;
    parentNodeValue = root;
    activeNodeValue = active;

    manager.removeSingleNode(KisNodeSP());
    manager.removeSingleNode(root);
    manager.removeSingleNode(child);

    QCOMPARE(parentRequests, (KisNodeList{root, child}));
    QCOMPARE(checkedNodeCounts, (QList<int>{1}));
    QCOMPARE(checkedNodes, (KisNodeList{child}));
    QVERIFY(removedNodes.isEmpty());
    QVERIFY(removalActiveNodes.isEmpty());

    canModifyValue = true;
    manager.removeSingleNode(child);

    QCOMPARE(parentRequests, (KisNodeList{root, child, child}));
    QCOMPARE(checkedNodeCounts, (QList<int>{1, 1}));
    QCOMPARE(checkedNodes, (KisNodeList{child, child}));
    QCOMPARE(removedNodeCounts, (QList<int>{1}));
    QCOMPARE(removedNodes, (KisNodeList{child}));
    QCOMPARE(removalActiveNodes, (KisNodeList{active}));
}

void KisNodeManagerRemovalContractTest::selectedRemovalUsesCurrentSelection()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP first = nodeToken(1);
    const KisNodeSP second = nodeToken(2);
    const KisNodeSP active = nodeToken(3);
    selectedNodesValue = {first, second};
    activeNodeValue = active;

    manager.removeNode();

    QCOMPARE(checkedNodeCounts, (QList<int>{2}));
    QCOMPARE(checkedNodes, (KisNodeList{first, second}));
    QVERIFY(removedNodes.isEmpty());

    canModifyValue = true;
    manager.removeNode();

    QCOMPARE(checkedNodeCounts, (QList<int>{2, 2}));
    QCOMPARE(checkedNodes, (KisNodeList{first, second, first, second}));
    QCOMPARE(removedNodeCounts, (QList<int>{2}));
    QCOMPARE(removedNodes, (KisNodeList{first, second}));
    QCOMPARE(removalActiveNodes, (KisNodeList{active}));
}

QTEST_GUILESS_MAIN(KisNodeManagerRemovalContractTest)

#include "KisNodeManagerRemovalContractTest.moc"
