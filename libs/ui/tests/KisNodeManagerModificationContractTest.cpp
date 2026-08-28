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

QSet<const KisNode *> editableNodes;
QHash<const KisNode *, QString> names;
QHash<const KisNode *, KisNodeSP> parents;
QStringList warnings;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

bool KisNodeManager::ModificationAccess::isEditable(KisNodeSP node)
{
    return editableNodes.contains(node.data());
}

QString KisNodeManager::ModificationAccess::name(KisNodeSP node)
{
    return names.value(node.data());
}

KisNodeSP KisNodeManager::ModificationAccess::parentNode(KisNodeSP node)
{
    return parents.value(node.data());
}

void KisNodeManager::ModificationAccess::showWarning(KisNodeManager *, const QString &message)
{
    warnings.append(message);
}

class KisNodeManagerModificationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void multipleNodesStopAtFirstLockedNode();
    void singleNodeDelegatesAndCanSuppressWarning();
    void movingNodesChecksTheirFirstLockedParent();
    void movingOneNodeDelegatesAndCanSuppressWarning();
};

void KisNodeManagerModificationContractTest::init()
{
    editableNodes.clear();
    names.clear();
    parents.clear();
    warnings.clear();
}

void KisNodeManagerModificationContractTest::multipleNodesStopAtFirstLockedNode()
{
    KisNodeManager manager(nullptr);
    QVERIFY(manager.canModifyLayers({}));

    const KisNodeSP editable = nodeToken(1);
    const KisNodeSP firstLocked = nodeToken(2);
    const KisNodeSP secondLocked = nodeToken(3);
    editableNodes.insert(editable.data());
    names.insert(firstLocked.data(), QStringLiteral("First locked"));
    names.insert(secondLocked.data(), QStringLiteral("Second locked"));
    QVERIFY(manager.canModifyLayers({editable}));
    QVERIFY(warnings.isEmpty());

    QVERIFY(!manager.canModifyLayers({editable, firstLocked, secondLocked}));
    QCOMPARE(warnings, QStringList({QStringLiteral("Layer \"First locked\" is locked")}));
}

void KisNodeManagerModificationContractTest::singleNodeDelegatesAndCanSuppressWarning()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP editable = nodeToken(1);
    const KisNodeSP locked = nodeToken(2);
    editableNodes.insert(editable.data());
    QVERIFY(manager.canModifyLayer(editable));
    QVERIFY(warnings.isEmpty());

    QVERIFY(!manager.canModifyLayer(locked));
    QCOMPARE(warnings, QStringList({QStringLiteral("Layer is locked")}));

    warnings.clear();
    QVERIFY(!manager.canModifyLayer(locked, false));
    QVERIFY(warnings.isEmpty());
}

void KisNodeManagerModificationContractTest::movingNodesChecksTheirFirstLockedParent()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP rootNode = nodeToken(1);
    const KisNodeSP movableNode = nodeToken(2);
    const KisNodeSP editableParent = nodeToken(3);
    const KisNodeSP blockedNode = nodeToken(4);
    const KisNodeSP lockedParent = nodeToken(5);
    parents.insert(movableNode.data(), editableParent);
    parents.insert(blockedNode.data(), lockedParent);
    editableNodes.insert(editableParent.data());
    names.insert(lockedParent.data(), QStringLiteral("Locked group"));

    QVERIFY(manager.canMoveLayers({rootNode, movableNode}));
    QVERIFY(warnings.isEmpty());
    QVERIFY(!manager.canMoveLayers({rootNode, movableNode, blockedNode}));
    QCOMPARE(warnings, QStringList({QStringLiteral("Layer \"Locked group\" is locked")}));
}

void KisNodeManagerModificationContractTest::movingOneNodeDelegatesAndCanSuppressWarning()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(1);
    const KisNodeSP lockedParent = nodeToken(2);
    parents.insert(node.data(), lockedParent);
    names.insert(lockedParent.data(), QStringLiteral("Locked parent"));

    QVERIFY(!manager.canMoveLayer(node));
    QCOMPARE(warnings, QStringList({QStringLiteral("Layer \"Locked parent\" is locked")}));

    warnings.clear();
    QVERIFY(!manager.canMoveLayer(node, false));
    QVERIFY(warnings.isEmpty());
}

QTEST_GUILESS_MAIN(KisNodeManagerModificationContractTest)

#include "KisNodeManagerModificationContractTest.moc"
