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

KisNodeSP nodeToken(quintptr id)
{
    return KisNodeSP(reinterpret_cast<KisNode *>(id * 32 + 1));
}

KisNodeSP activeNodeValue;
QSet<const KisNode *> layerNodes;
QHash<const KisNode *, QSet<QString>> inheritedTypes;
QSet<const KisNode *> editableNodes;
QSet<const KisNode *> editablePaintDeviceNodes;
QSet<const KisNode *> fakeNodes;
QSet<const KisNode *> selectionMasks;
QHash<const KisNode *, KisNodeSP> parents;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeSP KisNodeManager::activeNode()
{
    return activeNodeValue;
}

bool KisNodeManager::NodeStateAccess::isLayer(KisNodeSP node)
{
    return layerNodes.contains(node.data());
}

bool KisNodeManager::NodeStateAccess::inheritsType(KisNodeSP node, const QString &type)
{
    return inheritedTypes.value(node.data()).contains(type);
}

bool KisNodeManager::NodeStateAccess::isEditable(KisNodeSP node)
{
    return editableNodes.contains(node.data());
}

bool KisNodeManager::NodeStateAccess::hasEditablePaintDevice(KisNodeSP node)
{
    return editablePaintDeviceNodes.contains(node.data());
}

bool KisNodeManager::NodeStateAccess::isFakeNode(KisNodeSP node)
{
    return fakeNodes.contains(node.data());
}

bool KisNodeManager::NodeStateAccess::isSelectionMask(KisNodeSP node)
{
    return selectionMasks.contains(node.data());
}

KisNodeSP KisNodeManager::NodeStateAccess::parentNode(KisNodeSP node)
{
    return parents.value(node.data());
}

class KisNodeManagerNodeStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void activeNodeClassificationsRequireCurrentNodeState();
    void hiddenNodesFollowFakeAndGlobalSelectionState();
};

void KisNodeManagerNodeStateContractTest::init()
{
    activeNodeValue.clear();
    layerNodes.clear();
    inheritedTypes.clear();
    editableNodes.clear();
    editablePaintDeviceNodes.clear();
    fakeNodes.clear();
    selectionMasks.clear();
    parents.clear();
}

void KisNodeManagerNodeStateContractTest::activeNodeClassificationsRequireCurrentNodeState()
{
    KisNodeManager manager(nullptr);
    QVERIFY(!manager.activeNodeIsLayer());
    QVERIFY(!manager.activeNodeInherits(QStringLiteral("PaintNode")));
    QVERIFY(!manager.activeNodeIsEditable());
    QVERIFY(!manager.activeNodeHasEditablePaintDevice());

    activeNodeValue = nodeToken(1);
    QVERIFY(!manager.activeNodeIsLayer());
    layerNodes.insert(activeNodeValue.data());
    QVERIFY(manager.activeNodeIsLayer());

    QVERIFY(!manager.activeNodeInherits(QStringLiteral("PaintNode")));
    inheritedTypes[activeNodeValue.data()].insert(QStringLiteral("PaintNode"));
    QVERIFY(manager.activeNodeInherits(QStringLiteral("PaintNode")));
    QVERIFY(!manager.activeNodeInherits(QStringLiteral("MaskNode")));

    QVERIFY(!manager.activeNodeIsEditable());
    editableNodes.insert(activeNodeValue.data());
    QVERIFY(manager.activeNodeIsEditable());

    QVERIFY(!manager.activeNodeHasEditablePaintDevice());
    editablePaintDeviceNodes.insert(activeNodeValue.data());
    QVERIFY(manager.activeNodeHasEditablePaintDevice());
}

void KisNodeManagerNodeStateContractTest::hiddenNodesFollowFakeAndGlobalSelectionState()
{
    QVERIFY(!KisNodeManager::isNodeHidden(KisNodeSP(), false));
    QVERIFY(!KisNodeManager::isNodeHidden(KisNodeSP(), true));

    const KisNodeSP fakeNode = nodeToken(1);
    fakeNodes.insert(fakeNode.data());
    QVERIFY(KisNodeManager::isNodeHidden(fakeNode, false));

    const KisNodeSP selectionMask = nodeToken(2);
    selectionMasks.insert(selectionMask.data());
    QVERIFY(!KisNodeManager::isNodeHidden(selectionMask, false));
    QVERIFY(KisNodeManager::isNodeHidden(selectionMask, true));

    const KisNodeSP rootLayer = nodeToken(3);
    parents.insert(selectionMask.data(), rootLayer);
    QVERIFY(KisNodeManager::isNodeHidden(selectionMask, true));

    const KisNodeSP rootGroup = nodeToken(4);
    parents.insert(rootLayer.data(), rootGroup);
    QVERIFY(!KisNodeManager::isNodeHidden(selectionMask, true));

    QVERIFY(!KisNodeManager::isNodeHidden(nodeToken(5), true));
}

QTEST_GUILESS_MAIN(KisNodeManagerNodeStateContractTest)

#include "KisNodeManagerNodeStateContractTest.moc"
