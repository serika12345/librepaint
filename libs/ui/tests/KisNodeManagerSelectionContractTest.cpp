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

QList<KisNodeList> findResults;
KisNodeList selectedNodesValue;
bool sameNodesValue = false;
QList<int> requestedProperties;
QList<bool> requestedValues;
int selectedNodesRequests = 0;
int comparisonRequests = 0;
QList<int> reselectionNodeCounts;
KisNodeList reselectionNodes;
KisNodeList reselectionActiveNodes;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeList KisNodeManager::SelectionAccess::findNodes(KisNodeManager *, SelectionProperty property, bool value)
{
    requestedProperties.append(static_cast<int>(property));
    requestedValues.append(value);
    return findResults.takeFirst();
}

KisNodeList KisNodeManager::SelectionAccess::selectedNodes(KisNodeManager *)
{
    ++selectedNodesRequests;
    return selectedNodesValue;
}

bool KisNodeManager::SelectionAccess::sameNodesUnordered(const KisNodeList &, const KisNodeList &)
{
    ++comparisonRequests;
    return sameNodesValue;
}

void KisNodeManager::SelectionAccess::reselectNodes(KisNodeManager *, KisNodeSP activeNode, const KisNodeList &nodes)
{
    reselectionNodeCounts.append(nodes.size());
    reselectionNodes.append(nodes);
    reselectionActiveNodes.append(activeNode);
}

class KisNodeManagerSelectionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void selectionFiltersToggleToTheirInverse();
    void selectionUsesPrimaryAndSkipsEmptyResults();
};

void KisNodeManagerSelectionContractTest::init()
{
    findResults.clear();
    selectedNodesValue.clear();
    sameNodesValue = false;
    requestedProperties.clear();
    requestedValues.clear();
    selectedNodesRequests = 0;
    comparisonRequests = 0;
    reselectionNodeCounts.clear();
    reselectionNodes.clear();
    reselectionActiveNodes.clear();
}

void KisNodeManagerSelectionContractTest::selectionFiltersToggleToTheirInverse()
{
    KisNodeManager manager(nullptr);
    selectedNodesValue = {nodeToken(20)};
    sameNodesValue = true;
    for (quintptr id = 1; id <= 10; ++id) {
        findResults.append(KisNodeList{nodeToken(id)});
    }

    manager.selectAllNodes();
    manager.selectVisibleNodes();
    manager.selectLockedNodes();
    manager.selectInvisibleNodes();
    manager.selectUnlockedNodes();

    QCOMPARE(requestedProperties, (QList<int>{0, 0, 1, 1, 2, 2, 1, 1, 2, 2}));
    QCOMPARE(requestedValues, (QList<bool>{true, true, true, false, true, false, false, true, false, true}));
    QCOMPARE(selectedNodesRequests, 5);
    QCOMPARE(comparisonRequests, 5);
    QCOMPARE(reselectionNodeCounts, (QList<int>{1, 1, 1, 1, 1}));
    QCOMPARE(reselectionNodes, (KisNodeList{nodeToken(2), nodeToken(4), nodeToken(6), nodeToken(8), nodeToken(10)}));
    QCOMPARE(reselectionActiveNodes, reselectionNodes);
}

void KisNodeManagerSelectionContractTest::selectionUsesPrimaryAndSkipsEmptyResults()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP first = nodeToken(1);
    const KisNodeSP second = nodeToken(2);
    findResults = {KisNodeList{first, second}, KisNodeList{}};

    manager.selectVisibleNodes();
    manager.selectVisibleNodes();

    QCOMPARE(requestedProperties, (QList<int>{1, 1}));
    QCOMPARE(requestedValues, (QList<bool>{true, true}));
    QCOMPARE(selectedNodesRequests, 2);
    QCOMPARE(comparisonRequests, 2);
    QCOMPARE(reselectionNodeCounts, (QList<int>{2}));
    QCOMPARE(reselectionNodes, (KisNodeList{first, second}));
    QCOMPARE(reselectionActiveNodes, (KisNodeList{second}));
}

QTEST_GUILESS_MAIN(KisNodeManagerSelectionContractTest)

#include "KisNodeManagerSelectionContractTest.moc"
