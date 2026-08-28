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
    ShowLayerProperties,
    ShowMaskProperties,
    ChangeCloneSource,
    SetCurrentNode,
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

QSet<const KisNode *> layerNodes;
QSet<const KisNode *> maskNodes;
KisNodeSP currentNodeValue;
QList<Effect> effects;
KisNodeList assignedCurrentNodes;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

bool KisNodeManager::PropertyDialogAccess::isLayer(KisNodeSP node)
{
    return layerNodes.contains(node.data());
}

bool KisNodeManager::PropertyDialogAccess::isMask(KisNodeSP node)
{
    return maskNodes.contains(node.data());
}

void KisNodeManager::PropertyDialogAccess::showLayerProperties(KisNodeManager *)
{
    effects.append(ShowLayerProperties);
}

void KisNodeManager::PropertyDialogAccess::showMaskProperties(KisNodeManager *)
{
    effects.append(ShowMaskProperties);
}

KisNodeSP KisNodeManager::PropertyDialogAccess::currentNode(KisNodeManager *)
{
    return currentNodeValue;
}

void KisNodeManager::PropertyDialogAccess::setCurrentNode(KisNodeManager *, KisNodeSP node)
{
    effects.append(SetCurrentNode);
    assignedCurrentNodes.append(node);
    currentNodeValue = node;
}

void KisNodeManager::PropertyDialogAccess::changeCloneSource(KisNodeManager *)
{
    effects.append(ChangeCloneSource);
}

class KisNodeManagerPropertyDialogContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void propertiesRouteLayersBeforeMasks();
    void ignoredSelectionRestoresTheCurrentNode();
    void cloneSourceChangeDelegatesToLayerManager();
};

void KisNodeManagerPropertyDialogContractTest::init()
{
    layerNodes.clear();
    maskNodes.clear();
    currentNodeValue.clear();
    effects.clear();
    assignedCurrentNodes.clear();
}

void KisNodeManagerPropertyDialogContractTest::propertiesRouteLayersBeforeMasks()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP other = nodeToken(1);
    manager.nodeProperties(other);
    QVERIFY(effects.isEmpty());

    const KisNodeSP layer = nodeToken(2);
    layerNodes.insert(layer.data());
    manager.nodeProperties(layer);
    QCOMPARE(effects, QList<Effect>{ShowLayerProperties});

    effects.clear();
    const KisNodeSP mask = nodeToken(3);
    maskNodes.insert(mask.data());
    manager.nodeProperties(mask);
    QCOMPARE(effects, QList<Effect>{ShowMaskProperties});

    effects.clear();
    maskNodes.insert(layer.data());
    manager.nodeProperties(layer);
    QCOMPARE(effects, QList<Effect>{ShowLayerProperties});
}

void KisNodeManagerPropertyDialogContractTest::ignoredSelectionRestoresTheCurrentNode()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP original = nodeToken(1);
    const KisNodeSP requested = nodeToken(2);
    currentNodeValue = original;
    layerNodes.insert(requested.data());

    manager.nodePropertiesIgnoreSelection(requested);

    QCOMPARE(effects, (QList<Effect>{SetCurrentNode, ShowLayerProperties, SetCurrentNode}));
    QCOMPARE(assignedCurrentNodes, (KisNodeList{requested, original}));
    QCOMPARE(currentNodeValue, original);
}

void KisNodeManagerPropertyDialogContractTest::cloneSourceChangeDelegatesToLayerManager()
{
    KisNodeManager manager(nullptr);

    manager.changeCloneSource();

    QCOMPARE(effects, QList<Effect>{ChangeCloneSource});
}

QTEST_GUILESS_MAIN(KisNodeManagerPropertyDialogContractTest)

#include "KisNodeManagerPropertyDialogContractTest.moc"
