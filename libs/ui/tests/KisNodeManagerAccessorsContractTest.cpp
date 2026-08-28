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

KisNodeList selectedNodesValue;
KisNodeSelectionAdapter *nodeSelectionAdapterValue;
KisNodeInsertionAdapter *nodeInsertionAdapterValue;
KisNodeDisplayModeAdapter *nodeDisplayModeAdapterValue;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeList KisNodeManager::AccessorAccess::selectedNodes(KisNodeManager *)
{
    return selectedNodesValue;
}

KisNodeSelectionAdapter *KisNodeManager::AccessorAccess::nodeSelectionAdapter(const KisNodeManager *)
{
    return nodeSelectionAdapterValue;
}

KisNodeInsertionAdapter *KisNodeManager::AccessorAccess::nodeInsertionAdapter(const KisNodeManager *)
{
    return nodeInsertionAdapterValue;
}

KisNodeDisplayModeAdapter *KisNodeManager::AccessorAccess::nodeDisplayModeAdapter(const KisNodeManager *)
{
    return nodeDisplayModeAdapterValue;
}

class KisNodeManagerAccessorsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void selectedNodesPreserveStoredOrder();
    void adaptersReturnManagerOwnedObjects();
};

void KisNodeManagerAccessorsContractTest::init()
{
    selectedNodesValue.clear();
    nodeSelectionAdapterValue = nullptr;
    nodeInsertionAdapterValue = nullptr;
    nodeDisplayModeAdapterValue = nullptr;
}

void KisNodeManagerAccessorsContractTest::selectedNodesPreserveStoredOrder()
{
    KisNodeManager manager(nullptr);
    QVERIFY(manager.selectedNodes().isEmpty());
    selectedNodesValue << KisNodeSP(token<KisNode>(1)) << KisNodeSP(token<KisNode>(2));
    QCOMPARE(manager.selectedNodes(), selectedNodesValue);
}

void KisNodeManagerAccessorsContractTest::adaptersReturnManagerOwnedObjects()
{
    KisNodeManager manager(nullptr);
    QCOMPARE(manager.nodeSelectionAdapter(), nullptr);
    QCOMPARE(manager.nodeInsertionAdapter(), nullptr);
    QCOMPARE(manager.nodeDisplayModeAdapter(), nullptr);

    nodeSelectionAdapterValue = token<KisNodeSelectionAdapter>(1);
    nodeInsertionAdapterValue = token<KisNodeInsertionAdapter>(2);
    nodeDisplayModeAdapterValue = token<KisNodeDisplayModeAdapter>(3);
    QCOMPARE(manager.nodeSelectionAdapter(), nodeSelectionAdapterValue);
    QCOMPARE(manager.nodeInsertionAdapter(), nodeInsertionAdapterValue);
    QCOMPARE(manager.nodeDisplayModeAdapter(), nodeDisplayModeAdapterValue);
}

QTEST_GUILESS_MAIN(KisNodeManagerAccessorsContractTest)

#include "KisNodeManagerAccessorsContractTest.moc"
