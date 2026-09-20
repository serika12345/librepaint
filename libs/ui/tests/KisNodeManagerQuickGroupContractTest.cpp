/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QHash>
#include <QObject>
#include <QTest>

#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#include "nodes/kis_node_manager.h"
#pragma pop_macro("Q_OBJECT")

#include <kundo2magicstring.h>

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

KUndo2MagicString::KUndo2MagicString() = default;

KUndo2MagicString::KUndo2MagicString(const QString &text)
    : m_text(text)
{
}

QString KUndo2MagicString::toString() const
{
    const int separator = m_text.indexOf(QLatin1Char('\n'));
    return separator > 0 ? m_text.left(separator) : m_text;
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

KisNodeOperationBatch *batchToken(quintptr id)
{
    return token<KisNodeOperationBatch>(id);
}

KisNodeSP activeNodeValue;
bool canMoveResult = false;
bool canModifyResult = false;
QString nextLayerNameValue;
KisNodeList selectedNodesValue;
bool createGroupResult = false;
KisNodeSP createdGroupValue;
KisNodeSP createdLastChildValue;
bool ungroupResult = true;
KisNodeSP incompatibleNodeValue;
KisNodeSP destinationParentValue;
KisNodeList nodesWithParents;
QHash<const KisNode *, QString> nodeNames;
KisNodeOperationBatch *operationBatchValue = nullptr;
QStringList operationActionNames;
KisNodeList canMoveRequests;
QStringList nextLayerNameDefaults;
QList<KisNodeList> createGroupNodes;
KisNodeList createGroupActiveNodes;
QStringList createGroupNames;
QList<KisNodeOperationBatch *> createGroupBatches;
QList<KisNodeOperationBatch *> clippingMaskBatches;
KisNodeList clippingMaskParents;
KisNodeList clippingMaskAboveNodes;
QStringList clippingMaskNames;
KisNodeList canModifyRequests;
QList<KisNodeOperationBatch *> ungroupBatches;
QList<KisNodeList> ungroupNodeLists;
KisNodeList ungroupActiveNodes;
QStringList floatingMessages;
int activeNodeRequests = 0;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeOperationBatch *KisNodeManager::QuickGroupAccess::operationBatch(KisNodeManager *,
                                                                        const KUndo2MagicString &actionName)
{
    operationActionNames.append(actionName.toString());
    return operationBatchValue;
}

KisNodeSP KisNodeManager::QuickGroupAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    return activeNodeValue;
}

bool KisNodeManager::QuickGroupAccess::canMoveLayer(KisNodeManager *, KisNodeSP node)
{
    canMoveRequests.append(node);
    return canMoveResult;
}

QString KisNodeManager::QuickGroupAccess::nextLayerName(KisNodeManager *, const QString &defaultName)
{
    nextLayerNameDefaults.append(defaultName);
    return nextLayerNameValue;
}

KisNodeList KisNodeManager::QuickGroupAccess::selectedNodes(KisNodeManager *)
{
    return selectedNodesValue;
}

bool KisNodeManager::QuickGroupAccess::createGroup(KisNodeOperationBatch *batch,
                                                   const KisNodeList &nodes,
                                                   KisNodeSP activeNode,
                                                   const QString &groupName,
                                                   KisNodeSP *newGroup,
                                                   KisNodeSP *newLastChild)
{
    createGroupBatches.append(batch);
    createGroupNodes.append(nodes);
    createGroupActiveNodes.append(activeNode);
    createGroupNames.append(groupName);
    *newGroup = createdGroupValue;
    *newLastChild = createdLastChildValue;
    return createGroupResult;
}

void KisNodeManager::QuickGroupAccess::addClippingMask(KisNodeManager *,
                                                       KisNodeOperationBatch *batch,
                                                       KisNodeSP parent,
                                                       KisNodeSP above,
                                                       const QString &maskName)
{
    clippingMaskBatches.append(batch);
    clippingMaskParents.append(parent);
    clippingMaskAboveNodes.append(above);
    clippingMaskNames.append(maskName);
}

bool KisNodeManager::QuickGroupAccess::canModifyLayer(KisNodeManager *, KisNodeSP node)
{
    canModifyRequests.append(node);
    return canModifyResult;
}

bool KisNodeManager::QuickGroupAccess::ungroupNodes(KisNodeOperationBatch *batch,
                                                    const KisNodeList &nodes,
                                                    KisNodeSP activeNode,
                                                    KisNodeSP *incompatibleNode,
                                                    KisNodeSP *destinationParent)
{
    ungroupBatches.append(batch);
    ungroupNodeLists.append(nodes);
    ungroupActiveNodes.append(activeNode);
    *incompatibleNode = incompatibleNodeValue;
    *destinationParent = destinationParentValue;
    return ungroupResult;
}

KisNodeSP KisNodeManager::QuickGroupAccess::parentNode(KisNodeSP node)
{
    return nodesWithParents.contains(node) ? nodeToken(99) : KisNodeSP();
}

QString KisNodeManager::QuickGroupAccess::nodeName(KisNodeSP node)
{
    return nodeNames.value(node.data());
}

void KisNodeManager::QuickGroupAccess::showFloatingMessage(KisNodeManager *, const QString &message)
{
    floatingMessages.append(message);
}

class KisNodeManagerQuickGroupContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void quickGroupRequiresActiveMovableNode();
    void quickGroupUsesSelectionAndGeneratedName();
    void clippingGroupAddsMaskAfterGroupCreation();
    void quickUngroupRequiresModifiableNodeAndReportsFailure();
};

void KisNodeManagerQuickGroupContractTest::init()
{
    activeNodeValue.clear();
    canMoveResult = false;
    canModifyResult = false;
    nextLayerNameValue.clear();
    selectedNodesValue.clear();
    createGroupResult = false;
    createdGroupValue.clear();
    createdLastChildValue.clear();
    ungroupResult = true;
    incompatibleNodeValue.clear();
    destinationParentValue.clear();
    nodesWithParents.clear();
    nodeNames.clear();
    operationBatchValue = batchToken(1);
    operationActionNames.clear();
    canMoveRequests.clear();
    nextLayerNameDefaults.clear();
    createGroupNodes.clear();
    createGroupActiveNodes.clear();
    createGroupNames.clear();
    createGroupBatches.clear();
    clippingMaskBatches.clear();
    clippingMaskParents.clear();
    clippingMaskAboveNodes.clear();
    clippingMaskNames.clear();
    canModifyRequests.clear();
    ungroupBatches.clear();
    ungroupNodeLists.clear();
    ungroupActiveNodes.clear();
    floatingMessages.clear();
    activeNodeRequests = 0;
}

void KisNodeManagerQuickGroupContractTest::quickGroupRequiresActiveMovableNode()
{
    KisNodeManager manager(nullptr);

    manager.createQuickGroup();

    QCOMPARE(operationActionNames, (QStringList{kundo2_i18n("Quick Group").toString()}));
    QCOMPARE(activeNodeRequests, 1);
    QVERIFY(canMoveRequests.isEmpty());
    QVERIFY(createGroupNodes.isEmpty());

    activeNodeValue = nodeToken(1);
    manager.createQuickGroup();

    QCOMPARE(canMoveRequests, (KisNodeList{activeNodeValue}));
    QVERIFY(createGroupNodes.isEmpty());
}

void KisNodeManagerQuickGroupContractTest::quickGroupUsesSelectionAndGeneratedName()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    canMoveResult = true;
    selectedNodesValue = {nodeToken(2), activeNodeValue};
    nextLayerNameValue = QStringLiteral("Group 2");

    manager.createQuickGroup();

    QCOMPARE(nextLayerNameDefaults, (QStringList{i18nc("A group of layers", "Group")}));
    QCOMPARE(createGroupBatches, (QList<KisNodeOperationBatch *>{operationBatchValue}));
    QCOMPARE(createGroupNodes, (QList<KisNodeList>{selectedNodesValue}));
    QCOMPARE(createGroupActiveNodes, (KisNodeList{activeNodeValue}));
    QCOMPARE(createGroupNames, (QStringList{nextLayerNameValue}));
}

void KisNodeManagerQuickGroupContractTest::clippingGroupAddsMaskAfterGroupCreation()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    canMoveResult = true;
    nextLayerNameValue = QStringLiteral("Clipping Group 2");
    createGroupResult = true;
    createdGroupValue = nodeToken(2);
    createdLastChildValue = nodeToken(3);

    manager.createQuickClippingGroup();

    QCOMPARE(operationActionNames, (QStringList{kundo2_i18n("Quick Clipping Group").toString()}));
    QCOMPARE(nextLayerNameDefaults, (QStringList{i18nc("default name for a clipping group layer", "Clipping Group")}));
    QCOMPARE(createGroupNames, (QStringList{nextLayerNameValue}));
    QCOMPARE(clippingMaskBatches, (QList<KisNodeOperationBatch *>{operationBatchValue}));
    QCOMPARE(clippingMaskParents, (KisNodeList{createdGroupValue}));
    QCOMPARE(clippingMaskAboveNodes, (KisNodeList{createdLastChildValue}));
    QCOMPARE(clippingMaskNames, (QStringList{i18nc("default name for quick clip group mask layer", "Mask Layer")}));
}

void KisNodeManagerQuickGroupContractTest::quickUngroupRequiresModifiableNodeAndReportsFailure()
{
    KisNodeManager manager(nullptr);

    manager.quickUngroup();
    QVERIFY(canModifyRequests.isEmpty());
    QVERIFY(operationActionNames.isEmpty());

    activeNodeValue = nodeToken(1);
    manager.quickUngroup();
    QCOMPARE(canModifyRequests, (KisNodeList{activeNodeValue}));
    QVERIFY(operationActionNames.isEmpty());

    canModifyResult = true;
    selectedNodesValue = {nodeToken(2), activeNodeValue};
    ungroupResult = false;
    incompatibleNodeValue = nodeToken(3);
    destinationParentValue = nodeToken(4);
    nodesWithParents = {destinationParentValue};
    nodeNames.insert(incompatibleNodeValue.data(), QStringLiteral("Blocked"));
    nodeNames.insert(destinationParentValue.data(), QStringLiteral("Destination"));

    manager.quickUngroup();

    QCOMPARE(operationActionNames, (QStringList{kundo2_i18n("Quick Ungroup").toString()}));
    QCOMPARE(ungroupBatches, (QList<KisNodeOperationBatch *>{operationBatchValue}));
    QCOMPARE(ungroupNodeLists, (QList<KisNodeList>{selectedNodesValue}));
    QCOMPARE(ungroupActiveNodes, (KisNodeList{activeNodeValue}));
    QCOMPARE(floatingMessages,
             (QStringList{i18n("Cannot move layer \"%1\" into new parent \"%2\"", "Blocked", "Destination")}));

    nodesWithParents.clear();
    floatingMessages.clear();
    manager.quickUngroup();

    QCOMPARE(floatingMessages, (QStringList{i18n("Cannot move layer \"%1\" into the root layer", "Blocked")}));
}

QTEST_GUILESS_MAIN(KisNodeManagerQuickGroupContractTest)

#include "KisNodeManagerQuickGroupContractTest.moc"
