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

enum Effect {
    UpdateLayers,
    UpdateMasks,
    UpdateView,
    NotifySelectionChanged,
    SetTimelinePinned,
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

KisNodeSP activeNodeValue;
KisNodeList selectedNodesValue;
QSet<const KisNode *> timelinePinnedNodes;
QList<Effect> effects;
QList<bool> assignedTimelinePinnedValues;
KisNodeList assignedTimelinePinnedNodes;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeSP KisNodeManager::NodeUpdateAccess::activeNode(KisNodeManager *)
{
    return activeNodeValue;
}

void KisNodeManager::NodeUpdateAccess::updateLayers(KisNodeManager *)
{
    effects.append(UpdateLayers);
}

void KisNodeManager::NodeUpdateAccess::updateMasks(KisNodeManager *)
{
    effects.append(UpdateMasks);
}

void KisNodeManager::NodeUpdateAccess::updateView(KisNodeManager *)
{
    effects.append(UpdateView);
}

void KisNodeManager::NodeUpdateAccess::notifySelectionChanged(KisNodeManager *)
{
    effects.append(NotifySelectionChanged);
}

bool KisNodeManager::NodeUpdateAccess::isPinnedToTimeline(KisNodeSP node)
{
    return timelinePinnedNodes.contains(node.data());
}

void KisNodeManager::NodeUpdateAccess::setTimelinePinned(KisNodeManager *, bool value)
{
    effects.append(SetTimelinePinned);
    assignedTimelinePinnedValues.append(value);
}

KisNodeList KisNodeManager::NodeUpdateAccess::selectedNodes(KisNodeManager *)
{
    return selectedNodesValue;
}

void KisNodeManager::NodeUpdateAccess::setNodePinnedToTimeline(KisNodeSP node, bool value)
{
    assignedTimelinePinnedNodes.append(node);
    assignedTimelinePinnedValues.append(value);
}

class KisNodeManagerNodeUpdateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void nodeUpdatesSynchronizeUiInOrder();
    void timelinePinningUpdatesEverySelectedNode();
};

void KisNodeManagerNodeUpdateContractTest::init()
{
    activeNodeValue.clear();
    selectedNodesValue.clear();
    timelinePinnedNodes.clear();
    effects.clear();
    assignedTimelinePinnedValues.clear();
    assignedTimelinePinnedNodes.clear();
}

void KisNodeManagerNodeUpdateContractTest::nodeUpdatesSynchronizeUiInOrder()
{
    KisNodeManager manager(nullptr);

    manager.nodesUpdated();

    QVERIFY(effects.isEmpty());

    const KisNodeSP node = nodeToken(1);
    activeNodeValue = node;

    manager.nodesUpdated();

    QCOMPARE(effects,
             (QList<Effect>{UpdateLayers, UpdateMasks, UpdateView, NotifySelectionChanged, SetTimelinePinned}));
    QCOMPARE(assignedTimelinePinnedValues, QList<bool>{false});

    effects.clear();
    assignedTimelinePinnedValues.clear();
    timelinePinnedNodes.insert(node.data());

    manager.nodesUpdated();

    QCOMPARE(effects,
             (QList<Effect>{UpdateLayers, UpdateMasks, UpdateView, NotifySelectionChanged, SetTimelinePinned}));
    QCOMPARE(assignedTimelinePinnedValues, QList<bool>{true});
}

void KisNodeManagerNodeUpdateContractTest::timelinePinningUpdatesEverySelectedNode()
{
    KisNodeManager manager(nullptr);

    manager.slotPinToTimeline(true);

    QVERIFY(assignedTimelinePinnedNodes.isEmpty());

    const KisNodeSP first = nodeToken(1);
    const KisNodeSP second = nodeToken(2);
    selectedNodesValue = {first, second};

    manager.slotPinToTimeline(true);

    QCOMPARE(assignedTimelinePinnedNodes, (KisNodeList{first, second}));
    QCOMPARE(assignedTimelinePinnedValues, (QList<bool>{true, true}));

    assignedTimelinePinnedNodes.clear();
    assignedTimelinePinnedValues.clear();

    manager.slotPinToTimeline(false);

    QCOMPARE(assignedTimelinePinnedNodes, (KisNodeList{first, second}));
    QCOMPARE(assignedTimelinePinnedValues, (QList<bool>{false, false}));
}

QTEST_GUILESS_MAIN(KisNodeManagerNodeUpdateContractTest)

#include "KisNodeManagerNodeUpdateContractTest.moc"
