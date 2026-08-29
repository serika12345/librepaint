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

enum TreeEffect {
    MoveAt,
    Move,
    Copy,
    Add,
    AddUndoable,
    Duplicate,
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
int selectedNodesRequests = 0;
int activeNodeRequests = 0;
QList<TreeEffect> effects;
QList<int> nodeCounts;
KisNodeList nodes;
KisNodeList parents;
KisNodeList aboveNodes;
KisNodeList activeNodes;
QList<int> indices;

void recordNodes(TreeEffect effect, const KisNodeList &operationNodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    effects.append(effect);
    nodeCounts.append(operationNodes.size());
    nodes.append(operationNodes);
    parents.append(parent);
    aboveNodes.append(aboveThis);
}

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeList KisNodeManager::TreeOperationAccess::selectedNodes(KisNodeManager *)
{
    ++selectedNodesRequests;
    return selectedNodesValue;
}

KisNodeSP KisNodeManager::TreeOperationAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    return activeNodeValue;
}

void KisNodeManager::TreeOperationAccess::moveNodeAt(KisNodeManager *, KisNodeSP node, KisNodeSP parent, int index)
{
    recordNodes(MoveAt, KisNodeList{node}, parent, KisNodeSP());
    indices.append(index);
}

void KisNodeManager::TreeOperationAccess::moveNodes(KisNodeManager *,
                                                    const KisNodeList &operationNodes,
                                                    KisNodeSP parent,
                                                    KisNodeSP aboveThis,
                                                    KisNodeSP activeNode)
{
    recordNodes(Move, operationNodes, parent, aboveThis);
    activeNodes.append(activeNode);
}

void KisNodeManager::TreeOperationAccess::copyNodes(KisNodeManager *,
                                                    const KisNodeList &operationNodes,
                                                    KisNodeSP parent,
                                                    KisNodeSP aboveThis,
                                                    KisNodeSP activeNode)
{
    recordNodes(Copy, operationNodes, parent, aboveThis);
    activeNodes.append(activeNode);
}

void KisNodeManager::TreeOperationAccess::addNodes(KisNodeManager *,
                                                   const KisNodeList &operationNodes,
                                                   KisNodeSP parent,
                                                   KisNodeSP aboveThis,
                                                   KisNodeSP activeNode)
{
    recordNodes(Add, operationNodes, parent, aboveThis);
    activeNodes.append(activeNode);
}

void KisNodeManager::TreeOperationAccess::addNodeUndoable(KisNodeManager *,
                                                          KisNodeSP node,
                                                          KisNodeSP parent,
                                                          KisNodeSP aboveThis)
{
    recordNodes(AddUndoable, KisNodeList{node}, parent, aboveThis);
}

void KisNodeManager::TreeOperationAccess::duplicateNodes(KisNodeManager *,
                                                         const KisNodeList &operationNodes,
                                                         KisNodeSP activeNode)
{
    recordNodes(Duplicate, operationNodes, KisNodeSP(), KisNodeSP());
    activeNodes.append(activeNode);
}

class KisNodeManagerTreeOperationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void treeOperationsPreserveInputsAndCurrentState();
};

void KisNodeManagerTreeOperationContractTest::init()
{
    selectedNodesValue.clear();
    activeNodeValue.clear();
    selectedNodesRequests = 0;
    activeNodeRequests = 0;
    effects.clear();
    nodeCounts.clear();
    nodes.clear();
    parents.clear();
    aboveNodes.clear();
    activeNodes.clear();
    indices.clear();
}

void KisNodeManagerTreeOperationContractTest::treeOperationsPreserveInputsAndCurrentState()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP first = nodeToken(1);
    const KisNodeSP second = nodeToken(2);
    const KisNodeSP parent = nodeToken(3);
    const KisNodeSP above = nodeToken(4);
    const KisNodeSP active = nodeToken(5);
    const KisNodeList operationNodes{first, second};
    selectedNodesValue = operationNodes;
    activeNodeValue = active;

    manager.moveNodeAt(first, parent, 7);
    manager.moveNodesDirect(operationNodes, parent, above);
    manager.copyNodesDirect(operationNodes, parent, above);
    manager.addNodesDirect(operationNodes, parent, above);
    manager.addNodeUndoable(first, parent, above);
    manager.duplicateActiveNode();

    QCOMPARE(effects, (QList<TreeEffect>{MoveAt, Move, Copy, Add, AddUndoable, Duplicate}));
    QCOMPARE(nodeCounts, (QList<int>{1, 2, 2, 2, 1, 2}));
    QCOMPARE(nodes, (KisNodeList{first, first, second, first, second, first, second, first, first, second}));
    QCOMPARE(parents, (KisNodeList{parent, parent, parent, parent, parent, KisNodeSP()}));
    QCOMPARE(aboveNodes, (KisNodeList{KisNodeSP(), above, above, above, above, KisNodeSP()}));
    QCOMPARE(activeNodes, (KisNodeList{active, active, active, active}));
    QCOMPARE(indices, (QList<int>{7}));
    QCOMPARE(selectedNodesRequests, 1);
    QCOMPARE(activeNodeRequests, 4);
}

QTEST_GUILESS_MAIN(KisNodeManagerTreeOperationContractTest)

#include "KisNodeManagerTreeOperationContractTest.moc"
