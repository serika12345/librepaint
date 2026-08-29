/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
QSet<const KisNode *> nodesWithParent;
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

class KisNodeManagerNavigationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void siblingNavigationKeepsDirectionAndScope();
    void previousNodeRequiresAParent();
};

void KisNodeManagerNavigationContractTest::init()
{
    directions.clear();
    siblingsOnlyValues.clear();
    previouslyActiveNodeValue.clear();
    nodesWithParent.clear();
    activatedNodes.clear();
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

QTEST_GUILESS_MAIN(KisNodeManagerNavigationContractTest)

#include "KisNodeManagerNavigationContractTest.moc"
