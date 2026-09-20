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
KisNodeSP activeNodeValue;
KisNodeList supportedNodes;
bool propertyStateValue = false;
int selectedNodesRequests = 0;
int activeNodeRequests = 0;
QList<int> supportProperties;
KisNodeList supportNodes;
QList<int> stateProperties;
KisNodeList stateNodes;
QList<int> appliedProperties;
KisNodeList appliedNodes;
QList<bool> appliedValues;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeList KisNodeManager::ToggleAccess::selectedNodes(KisNodeManager *)
{
    ++selectedNodesRequests;
    return selectedNodesValue;
}

KisNodeSP KisNodeManager::ToggleAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    return activeNodeValue;
}

bool KisNodeManager::ToggleAccess::supportsProperty(KisNodeSP node, ToggleProperty property)
{
    supportProperties.append(static_cast<int>(property));
    supportNodes.append(node);
    return supportedNodes.contains(node);
}

bool KisNodeManager::ToggleAccess::propertyState(KisNodeSP node, ToggleProperty property)
{
    stateProperties.append(static_cast<int>(property));
    stateNodes.append(node);
    return propertyStateValue;
}

void KisNodeManager::ToggleAccess::setProperty(KisNodeManager *, KisNodeSP node, ToggleProperty property, bool value)
{
    appliedProperties.append(static_cast<int>(property));
    appliedNodes.append(node);
    appliedValues.append(value);
}

class KisNodeManagerToggleContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void toggleRequiresSelectionAndActiveNode();
    void lockAndVisibilityUseActiveStateForAllNodes();
    void alphaLockRequiresPaintLayers();
    void inheritAlphaRequiresLayers();
};

void KisNodeManagerToggleContractTest::init()
{
    selectedNodesValue.clear();
    activeNodeValue.clear();
    supportedNodes.clear();
    propertyStateValue = false;
    selectedNodesRequests = 0;
    activeNodeRequests = 0;
    supportProperties.clear();
    supportNodes.clear();
    stateProperties.clear();
    stateNodes.clear();
    appliedProperties.clear();
    appliedNodes.clear();
    appliedValues.clear();
}

void KisNodeManagerToggleContractTest::toggleRequiresSelectionAndActiveNode()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(1);

    manager.toggleLock();
    selectedNodesValue = {node};
    manager.toggleLock();

    QCOMPARE(selectedNodesRequests, 2);
    QCOMPARE(activeNodeRequests, 2);
    QVERIFY(supportNodes.isEmpty());
    QVERIFY(stateNodes.isEmpty());
    QVERIFY(appliedNodes.isEmpty());
}

void KisNodeManagerToggleContractTest::lockAndVisibilityUseActiveStateForAllNodes()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP first = nodeToken(1);
    const KisNodeSP second = nodeToken(2);
    selectedNodesValue = {first, second};
    activeNodeValue = second;
    supportedNodes = selectedNodesValue;
    propertyStateValue = true;

    manager.toggleLock();
    propertyStateValue = false;
    manager.toggleVisibility();

    QCOMPARE(supportProperties, (QList<int>{0, 0, 0, 1, 1, 1}));
    QCOMPARE(supportNodes, (KisNodeList{second, first, second, second, first, second}));
    QCOMPARE(stateProperties, (QList<int>{0, 1}));
    QCOMPARE(stateNodes, (KisNodeList{second, second}));
    QCOMPARE(appliedProperties, (QList<int>{0, 0, 1, 1}));
    QCOMPARE(appliedNodes, (KisNodeList{first, second, first, second}));
    QCOMPARE(appliedValues, (QList<bool>{false, false, true, true}));
}

void KisNodeManagerToggleContractTest::alphaLockRequiresPaintLayers()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP paintLayer = nodeToken(1);
    const KisNodeSP other = nodeToken(2);
    selectedNodesValue = {paintLayer, other};
    activeNodeValue = other;
    supportedNodes = {paintLayer};

    manager.toggleAlphaLock();
    QVERIFY(stateNodes.isEmpty());
    QVERIFY(appliedNodes.isEmpty());

    activeNodeValue = paintLayer;
    propertyStateValue = true;
    manager.toggleAlphaLock();

    QCOMPARE(stateProperties, (QList<int>{2}));
    QCOMPARE(stateNodes, (KisNodeList{paintLayer}));
    QCOMPARE(appliedProperties, (QList<int>{2}));
    QCOMPARE(appliedNodes, (KisNodeList{paintLayer}));
    QCOMPARE(appliedValues, (QList<bool>{false}));
}

void KisNodeManagerToggleContractTest::inheritAlphaRequiresLayers()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP layer = nodeToken(1);
    const KisNodeSP other = nodeToken(2);
    selectedNodesValue = {layer, other};
    activeNodeValue = layer;
    supportedNodes = {layer};

    manager.toggleInheritAlpha();

    QCOMPARE(stateProperties, (QList<int>{3}));
    QCOMPARE(stateNodes, (KisNodeList{layer}));
    QCOMPARE(appliedProperties, (QList<int>{3}));
    QCOMPARE(appliedNodes, (KisNodeList{layer}));
    QCOMPARE(appliedValues, (QList<bool>{true}));
}

QTEST_GUILESS_MAIN(KisNodeManagerToggleContractTest)

#include "KisNodeManagerToggleContractTest.moc"
