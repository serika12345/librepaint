/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QHash>
#include <QObject>
#include <QSet>
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

enum Direction {
    Next,
    Previous,
};

QList<Direction> directions;
QList<bool> siblingsOnlyValues;
KisNodeSP previouslyActiveNodeValue;
KisNodeSP activeNodeValue;
QSet<const KisNode *> nodesWithParent;
QSet<const KisNode *> nodesWithChildren;
QSet<const KisNode *> hiddenNodes;
QHash<const KisNode *, KisNodeSP> nextSiblings;
QHash<const KisNode *, KisNodeSP> previousSiblings;
QHash<const KisNode *, KisNodeSP> firstChildren;
QHash<const KisNode *, KisNodeSP> lastChildren;
QHash<const KisNode *, KisNodeSP> parentNodes;
KisNodeList activatedNodes;

template<typename T>
T *token(quintptr id)
{
    return reinterpret_cast<T *>(id * 32 + 1);
}

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(token<KisNode>(id));
}

void clearTraversalState()
{
    activeNodeValue.clear();
    nodesWithChildren.clear();
    hiddenNodes.clear();
    nextSiblings.clear();
    previousSiblings.clear();
    firstChildren.clear();
    lastChildren.clear();
    parentNodes.clear();
    activatedNodes.clear();
}

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

void KisNodeManager::NavigationAccess::activateNextNode(KisNodeManager *, bool siblingsOnly)
{
    directions.append(Next);
    siblingsOnlyValues.append(siblingsOnly);
}

void KisNodeManager::NavigationAccess::activatePreviousNode(KisNodeManager *, bool siblingsOnly)
{
    directions.append(Previous);
    siblingsOnlyValues.append(siblingsOnly);
}

KisNodeSP KisNodeManager::NavigationAccess::previouslyActiveNode(KisNodeManager *)
{
    return previouslyActiveNodeValue;
}

bool KisNodeManager::NavigationAccess::hasParent(KisNodeSP node)
{
    return nodesWithParent.contains(node.data());
}

void KisNodeManager::NavigationAccess::activateNode(KisNodeManager *, KisNodeSP node)
{
    activatedNodes.append(node);
}

KisNodeSP KisNodeManager::NavigationAccess::activeNode(KisNodeManager *)
{
    return activeNodeValue;
}

KisNodeSP KisNodeManager::NavigationAccess::nextSibling(KisNodeSP node)
{
    return nextSiblings.value(node.data());
}

KisNodeSP KisNodeManager::NavigationAccess::previousSibling(KisNodeSP node)
{
    return previousSiblings.value(node.data());
}

bool KisNodeManager::NavigationAccess::hasChildren(KisNodeSP node)
{
    return nodesWithChildren.contains(node.data());
}

KisNodeSP KisNodeManager::NavigationAccess::firstChild(KisNodeSP node)
{
    return firstChildren.value(node.data());
}

KisNodeSP KisNodeManager::NavigationAccess::lastChild(KisNodeSP node)
{
    return lastChildren.value(node.data());
}

KisNodeSP KisNodeManager::NavigationAccess::parentNode(KisNodeSP node)
{
    return parentNodes.value(node.data());
}

bool KisNodeManager::NavigationAccess::isHidden(KisNodeManager *, KisNodeSP node)
{
    return hiddenNodes.contains(node.data());
}

class KisNodeManagerNavigationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void siblingNavigationKeepsDirectionAndScope();
    void previousNodeRequiresAParent();
    void nextNavigationTraversesVisibleHierarchy();
    void previousNavigationTraversesVisibleHierarchy();
};

void KisNodeManagerNavigationContractTest::init()
{
    directions.clear();
    siblingsOnlyValues.clear();
    previouslyActiveNodeValue.clear();
    nodesWithParent.clear();
    clearTraversalState();
}

void KisNodeManagerNavigationContractTest::siblingNavigationKeepsDirectionAndScope()
{
    KisNodeManager manager(nullptr);

    manager.activateNextSiblingNode();
    manager.activatePreviousSiblingNode();

    QCOMPARE(directions, (QList<Direction>{Next, Previous}));
    QCOMPARE(siblingsOnlyValues, (QList<bool>{true, true}));
}

void KisNodeManagerNavigationContractTest::previousNodeRequiresAParent()
{
    KisNodeManager manager(nullptr);

    manager.switchToPreviouslyActiveNode();

    QVERIFY(activatedNodes.isEmpty());

    const KisNodeSP previous = nodeToken(1);
    previouslyActiveNodeValue = previous;

    manager.switchToPreviouslyActiveNode();

    QVERIFY(activatedNodes.isEmpty());

    nodesWithParent.insert(previous.data());

    manager.switchToPreviouslyActiveNode();

    QCOMPARE(activatedNodes, KisNodeList{previous});
}

void KisNodeManagerNavigationContractTest::nextNavigationTraversesVisibleHierarchy()
{
    KisNodeManager manager(nullptr);

    manager.activateNextNode();
    QVERIFY(activatedNodes.isEmpty());

    const KisNodeSP active = nodeToken(1);
    const KisNodeSP group = nodeToken(2);
    const KisNodeSP nested = nodeToken(3);
    const KisNodeSP leaf = nodeToken(4);
    activeNodeValue = active;
    nextSiblings.insert(active.data(), group);
    nodesWithChildren.insert(group.data());
    firstChildren.insert(group.data(), nested);
    nodesWithChildren.insert(nested.data());
    firstChildren.insert(nested.data(), leaf);
    parentNodes.insert(leaf.data(), nested);

    manager.activateNextNode();
    QCOMPARE(activatedNodes, KisNodeList{leaf});

    clearTraversalState();
    const KisNodeSP hidden = nodeToken(5);
    const KisNodeSP visible = nodeToken(6);
    const KisNodeSP root = nodeToken(7);
    activeNodeValue = active;
    nextSiblings.insert(active.data(), hidden);
    nextSiblings.insert(hidden.data(), visible);
    hiddenNodes.insert(hidden.data());
    parentNodes.insert(visible.data(), root);

    manager.activateNextNode(true);
    QCOMPARE(activatedNodes, KisNodeList{visible});

    clearTraversalState();
    activeNodeValue = active;
    parentNodes.insert(active.data(), group);
    parentNodes.insert(group.data(), root);

    manager.activateNextNode();
    QCOMPARE(activatedNodes, KisNodeList{group});

    clearTraversalState();
    activeNodeValue = active;
    nextSiblings.insert(active.data(), root);

    manager.activateNextNode(true);
    QVERIFY(activatedNodes.isEmpty());
}

void KisNodeManagerNavigationContractTest::previousNavigationTraversesVisibleHierarchy()
{
    KisNodeManager manager(nullptr);

    manager.activatePreviousNode();
    QVERIFY(activatedNodes.isEmpty());

    const KisNodeSP active = nodeToken(1);
    const KisNodeSP child = nodeToken(2);
    activeNodeValue = active;
    nodesWithChildren.insert(active.data());
    lastChildren.insert(active.data(), child);
    parentNodes.insert(child.data(), active);

    manager.activatePreviousNode();
    QCOMPARE(activatedNodes, KisNodeList{child});

    clearTraversalState();
    const KisNodeSP hidden = nodeToken(3);
    const KisNodeSP visible = nodeToken(4);
    const KisNodeSP root = nodeToken(5);
    activeNodeValue = active;
    previousSiblings.insert(active.data(), hidden);
    previousSiblings.insert(hidden.data(), visible);
    hiddenNodes.insert(hidden.data());
    parentNodes.insert(visible.data(), root);

    manager.activatePreviousNode(true);
    QCOMPARE(activatedNodes, KisNodeList{visible});

    clearTraversalState();
    const KisNodeSP group = nodeToken(6);
    const KisNodeSP uncle = nodeToken(7);
    activeNodeValue = active;
    parentNodes.insert(active.data(), group);
    previousSiblings.insert(group.data(), uncle);
    parentNodes.insert(uncle.data(), root);

    manager.activatePreviousNode();
    QCOMPARE(activatedNodes, KisNodeList{uncle});

    clearTraversalState();
    activeNodeValue = active;
    previousSiblings.insert(active.data(), root);

    manager.activatePreviousNode(true);
    QVERIFY(activatedNodes.isEmpty());
}

QTEST_GUILESS_MAIN(KisNodeManagerNavigationContractTest)

#include "KisNodeManagerNavigationContractTest.moc"
