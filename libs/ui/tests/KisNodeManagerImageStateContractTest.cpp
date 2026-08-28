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

void kisSharedPtrAddReference(KisLayer *)
{
}

bool kisSharedPtrRelease(KisLayer *)
{
    return true;
}

void kisSharedPtrAddReference(KisMask *)
{
}

bool kisSharedPtrRelease(KisMask *)
{
    return true;
}

void kisSharedPtrAddReference(KisSelection *)
{
}

bool kisSharedPtrRelease(KisSelection *)
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

KisLayerSP layerToken(quintptr id)
{
    return KisLayerSP(token<KisLayer>(id));
}

KisMaskSP maskToken(quintptr id)
{
    return KisMaskSP(token<KisMask>(id));
}

KisSelectionSP selectionToken(quintptr id)
{
    return KisSelectionSP(token<KisSelection>(id));
}

KisNodeSP activeNodeValue;
KisNodeSP activeLayerValue;
KisNodeSP nearestNodeValue;
QSet<const KisNode *> animatedNodes;
QHash<const KisNode *, KisNodeSP> selectionMasks;
QHash<const KisNode *, bool> editableSelectionMasks;
QHash<const KisNode *, KisLayerSP> layers;
QHash<const KisNode *, KisMaskSP> masks;
QHash<const KisNode *, KisNodeSP> parents;
QHash<const KisNode *, KisSelectionSP> selections;

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

KisNodeSP KisNodeManager::ImageStateAccess::nearestNodeAfterRemoval(KisNodeSP)
{
    return nearestNodeValue;
}

bool KisNodeManager::ImageStateAccess::isAnimated(KisNodeSP node)
{
    return animatedNodes.contains(node.data());
}

KisNodeSP KisNodeManager::ImageStateAccess::activeLayerNode(KisNodeManager *)
{
    return activeLayerValue;
}

KisNodeSP KisNodeManager::ImageStateAccess::selectionMaskNode(KisNodeSP layer)
{
    return selectionMasks.value(layer.data());
}

bool KisNodeManager::ImageStateAccess::isEditable(KisNodeSP mask)
{
    return editableSelectionMasks.value(mask.data());
}

void KisNodeManager::ImageStateAccess::updateImageNodeSettings(KisImageWSP)
{
}

void KisNodeManager::ImageStateAccess::createNodeActivationActions(KisImageWSP,
                                                                   KisKActionCollection *,
                                                                   KisNodeManager *)
{
}

bool KisNodeManager::ImageStateAccess::isLayer(KisNodeSP node)
{
    return layers.contains(node.data());
}

bool KisNodeManager::ImageStateAccess::isMask(KisNodeSP node)
{
    return masks.contains(node.data());
}

KisNodeSP KisNodeManager::ImageStateAccess::parentNode(KisNodeSP mask)
{
    return parents.value(mask.data());
}

KisLayerSP KisNodeManager::ImageStateAccess::toLayer(KisNodeSP node)
{
    return layers.value(node.data());
}

KisMaskSP KisNodeManager::ImageStateAccess::toMask(KisNodeSP node)
{
    return masks.value(node.data());
}

KisSelectionSP KisNodeManager::ImageStateAccess::selection(KisNodeSP layer)
{
    return selections.value(layer.data());
}

KisSelectionSP KisNodeManager::ImageStateAccess::globalSelection(KisImageWSP)
{
    return KisSelectionSP();
}

class KisNodeManagerImageStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void removalAndNodeKindsFollowTheImageGraph();
    void activeAnimationAndSelectionEditabilityFollowNodeState();
    void selectionUsesTheOwningLayer();
};

void KisNodeManagerImageStateContractTest::init()
{
    activeNodeValue.clear();
    activeLayerValue.clear();
    nearestNodeValue.clear();
    animatedNodes.clear();
    selectionMasks.clear();
    editableSelectionMasks.clear();
    layers.clear();
    masks.clear();
    parents.clear();
    selections.clear();
}

void KisNodeManagerImageStateContractTest::removalAndNodeKindsFollowTheImageGraph()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP removedNode = nodeToken(1);
    const KisNodeSP nearestNode = nodeToken(2);
    nearestNodeValue = nearestNode;
    QCOMPARE(manager.nearestNodeAfterRemoval(removedNode), nearestNode);

    const KisNodeSP layerNode = nodeToken(3);
    const KisLayerSP layer = layerToken(3);
    layers.insert(layerNode.data(), layer);
    QCOMPARE(manager.layerForNode(layerNode), layer);
    QVERIFY(manager.maskForNode(layerNode).isNull());

    const KisNodeSP maskNode = nodeToken(4);
    const KisMaskSP mask = maskToken(4);
    masks.insert(maskNode.data(), mask);
    parents.insert(maskNode.data(), layerNode);
    QCOMPARE(manager.maskForNode(maskNode), mask);
    QCOMPARE(manager.layerForNode(maskNode), layer);

    const KisNodeSP otherNode = nodeToken(5);
    QVERIFY(manager.layerForNode(otherNode).isNull());
    QVERIFY(manager.maskForNode(otherNode).isNull());
}

void KisNodeManagerImageStateContractTest::activeAnimationAndSelectionEditabilityFollowNodeState()
{
    KisNodeManager manager(nullptr);
    QVERIFY(!manager.activeNodeIsAnimated());
    QVERIFY(manager.activeSelectionIsEditable());

    activeNodeValue = nodeToken(1);
    QVERIFY(!manager.activeNodeIsAnimated());
    animatedNodes.insert(activeNodeValue.data());
    QVERIFY(manager.activeNodeIsAnimated());

    activeLayerValue = nodeToken(2);
    layers.insert(activeLayerValue.data(), layerToken(2));
    QVERIFY(manager.activeSelectionIsEditable());
    const KisNodeSP mask = nodeToken(3);
    selectionMasks.insert(activeLayerValue.data(), mask);
    editableSelectionMasks.insert(mask.data(), true);
    QVERIFY(manager.activeSelectionIsEditable());
    editableSelectionMasks.insert(mask.data(), false);
    QVERIFY(!manager.activeSelectionIsEditable());
}

void KisNodeManagerImageStateContractTest::selectionUsesTheOwningLayer()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP layerNode = nodeToken(1);
    const KisLayerSP layer = layerToken(1);
    const KisSelectionSP selection = selectionToken(2);
    layers.insert(layerNode.data(), layer);
    selections.insert(layerNode.data(), selection);
    QCOMPARE(manager.selectionForNode(layerNode, KisImageWSP()), selection);

    const KisNodeSP maskNode = nodeToken(3);
    const KisMaskSP mask = maskToken(3);
    masks.insert(maskNode.data(), mask);
    parents.insert(maskNode.data(), layerNode);
    QCOMPARE(manager.selectionForNode(maskNode, KisImageWSP()), selection);

    QVERIFY(manager.selectionForNode(nodeToken(4), KisImageWSP()).isNull());
}

QTEST_GUILESS_MAIN(KisNodeManagerImageStateContractTest)

#include "KisNodeManagerImageStateContractTest.moc"
