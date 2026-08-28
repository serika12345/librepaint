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

const KoCompositeOp *compositeOpToken(quintptr id)
{
    return reinterpret_cast<const KoCompositeOp *>(id * 32 + 1);
}

KisNodeSP activeNodeValue;
QHash<const KisNode *, QString> names;
QHash<const KisNode *, qint32> opacities;
QHash<const KisNode *, const KoCompositeOp *> compositeOps;
int nameChangeCount;
int opacityChangeCount;
int compositeOpChangeCount;

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

QString KisNodeManager::NodeChangeAccess::name(KisNodeSP node)
{
    return names.value(node.data());
}

qint32 KisNodeManager::NodeChangeAccess::opacity(KisNodeSP node)
{
    return opacities.value(node.data());
}

const KoCompositeOp *KisNodeManager::NodeChangeAccess::compositeOp(KisNodeSP node)
{
    return compositeOps.value(node.data());
}

void KisNodeManager::NodeChangeAccess::setName(KisNodeManager *, KisNodeSP node, const QString &name)
{
    names.insert(node.data(), name);
    ++nameChangeCount;
}

void KisNodeManager::NodeChangeAccess::setOpacity(KisNodeManager *, KisNodeSP node, qint32 opacity)
{
    opacities.insert(node.data(), opacity);
    ++opacityChangeCount;
}

void KisNodeManager::NodeChangeAccess::setCompositeOp(KisNodeManager *,
                                                      KisNodeSP node,
                                                      const KoCompositeOp *compositeOp)
{
    compositeOps.insert(node.data(), compositeOp);
    ++compositeOpChangeCount;
}

class KisNodeManagerNodeChangeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void settersSkipMissingAndUnchangedNodes();
    void activeOpacityChangesUseCurrentNodeAndExistingScale();
    void activeCompositeOpChangesUseCurrentNode();
};

void KisNodeManagerNodeChangeContractTest::init()
{
    activeNodeValue.clear();
    names.clear();
    opacities.clear();
    compositeOps.clear();
    nameChangeCount = 0;
    opacityChangeCount = 0;
    compositeOpChangeCount = 0;
}

void KisNodeManagerNodeChangeContractTest::settersSkipMissingAndUnchangedNodes()
{
    KisNodeManager manager(nullptr);
    const KoCompositeOp *const initialCompositeOp = compositeOpToken(1);
    manager.setNodeName(KisNodeSP(), QStringLiteral("Missing"));
    manager.setNodeOpacity(KisNodeSP(), 64);
    manager.setNodeCompositeOp(KisNodeSP(), initialCompositeOp);
    QCOMPARE(nameChangeCount, 0);
    QCOMPARE(opacityChangeCount, 0);
    QCOMPARE(compositeOpChangeCount, 0);

    const KisNodeSP node = nodeToken(1);
    names.insert(node.data(), QStringLiteral("Layer"));
    opacities.insert(node.data(), 128);
    compositeOps.insert(node.data(), initialCompositeOp);
    manager.setNodeName(node, QStringLiteral("Layer"));
    manager.setNodeOpacity(node, 128);
    manager.setNodeCompositeOp(node, initialCompositeOp);
    QCOMPARE(nameChangeCount, 0);
    QCOMPARE(opacityChangeCount, 0);
    QCOMPARE(compositeOpChangeCount, 0);

    const KoCompositeOp *const newCompositeOp = compositeOpToken(2);
    manager.setNodeName(node, QStringLiteral("Renamed"));
    manager.setNodeOpacity(node, 192);
    manager.setNodeCompositeOp(node, newCompositeOp);
    QCOMPARE(names.value(node.data()), QStringLiteral("Renamed"));
    QCOMPARE(opacities.value(node.data()), 192);
    QCOMPARE(compositeOps.value(node.data()), newCompositeOp);
    QCOMPARE(nameChangeCount, 1);
    QCOMPARE(opacityChangeCount, 1);
    QCOMPARE(compositeOpChangeCount, 1);
}

void KisNodeManagerNodeChangeContractTest::activeOpacityChangesUseCurrentNodeAndExistingScale()
{
    KisNodeManager manager(nullptr);
    manager.nodeOpacityChanged(50.0);
    QCOMPARE(opacityChangeCount, 0);

    activeNodeValue = nodeToken(1);
    opacities.insert(activeNodeValue.data(), 0);
    manager.nodeOpacityChanged(0.0);
    QCOMPARE(opacityChangeCount, 0);

    manager.nodeOpacityChanged(50.0);
    QCOMPARE(opacities.value(activeNodeValue.data()), 127);
    manager.nodeOpacityChanged(100.0);
    QCOMPARE(opacities.value(activeNodeValue.data()), 255);
    manager.nodeOpacityChanged(200.0);
    QCOMPARE(opacityChangeCount, 2);
}

void KisNodeManagerNodeChangeContractTest::activeCompositeOpChangesUseCurrentNode()
{
    KisNodeManager manager(nullptr);
    const KoCompositeOp *const initialCompositeOp = compositeOpToken(1);
    manager.nodeCompositeOpChanged(initialCompositeOp);
    QCOMPARE(compositeOpChangeCount, 0);

    activeNodeValue = nodeToken(1);
    compositeOps.insert(activeNodeValue.data(), initialCompositeOp);
    manager.nodeCompositeOpChanged(initialCompositeOp);
    QCOMPARE(compositeOpChangeCount, 0);
    const KoCompositeOp *const newCompositeOp = compositeOpToken(2);
    manager.nodeCompositeOpChanged(newCompositeOp);
    QCOMPARE(compositeOps.value(activeNodeValue.data()), newCompositeOp);
    QCOMPARE(compositeOpChangeCount, 1);
}

QTEST_GUILESS_MAIN(KisNodeManagerNodeChangeContractTest)

#include "KisNodeManagerNodeChangeContractTest.moc"
