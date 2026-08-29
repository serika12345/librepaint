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

void *privateStateValue = token<void>(1);
QList<void *> destroyedStates;
QList<bool> imageViewAvailability;
KisNodeList currentNodeValues;
KisNodeSP lastActivatedNodeValue;
KisNodeSP lastRootChildValue;
bool graphListenerValue = true;
KisNodeList graphListenerRequests;
KisNodeList activationRequests;
KisNodeList resourceProviderNotifications;
QStringList effects;

bool takeImageViewAvailability()
{
    return imageViewAvailability.isEmpty() ? false : imageViewAvailability.takeFirst();
}

KisNodeSP takeCurrentNode()
{
    return currentNodeValues.isEmpty() ? KisNodeSP() : currentNodeValues.takeFirst();
}

} // namespace

void *KisNodeManager::LifecycleAccess::createPrivateState(KisNodeManager *, KisViewManager *)
{
    effects.append(QStringLiteral("create"));
    return privateStateValue;
}

void KisNodeManager::LifecycleAccess::connectReselectionOutput(KisNodeManager *)
{
    effects.append(QStringLiteral("connect-output"));
}

void KisNodeManager::LifecycleAccess::destroyPrivateState(void *state)
{
    destroyedStates.append(state);
    effects.append(QStringLiteral("destroy"));
}

void KisNodeManager::LifecycleAccess::setMaskView(KisNodeManager *, QPointer<KisView>)
{
    effects.append(QStringLiteral("mask-view"));
}

void KisNodeManager::LifecycleAccess::setLayerView(KisNodeManager *, QPointer<KisView>)
{
    effects.append(QStringLiteral("layer-view"));
}

bool KisNodeManager::LifecycleAccess::hasImageView(KisNodeManager *)
{
    effects.append(QStringLiteral("has-view"));
    return takeImageViewAvailability();
}

void KisNodeManager::LifecycleAccess::disconnectNodeActivation(KisNodeManager *)
{
    effects.append(QStringLiteral("disconnect-activation"));
}

void KisNodeManager::LifecycleAccess::disconnectImageSignals(KisNodeManager *)
{
    effects.append(QStringLiteral("disconnect-image"));
}

void KisNodeManager::LifecycleAccess::disconnectReselectionInput(KisNodeManager *)
{
    effects.append(QStringLiteral("disconnect-reselection"));
}

void KisNodeManager::LifecycleAccess::assignImageView(KisNodeManager *, QPointer<KisView>)
{
    effects.append(QStringLiteral("assign-view"));
}

void KisNodeManager::LifecycleAccess::assignCommandImage(KisNodeManager *)
{
    effects.append(QStringLiteral("command-image"));
}

void KisNodeManager::LifecycleAccess::connectNodeActivation(KisNodeManager *)
{
    effects.append(QStringLiteral("connect-activation"));
}

KisNodeSP KisNodeManager::LifecycleAccess::currentNode(KisNodeManager *)
{
    effects.append(QStringLiteral("current"));
    return takeCurrentNode();
}

KisNodeSP KisNodeManager::LifecycleAccess::lastActivatedNode(KisNodeManager *)
{
    effects.append(QStringLiteral("last"));
    return lastActivatedNodeValue;
}

bool KisNodeManager::LifecycleAccess::hasGraphListener(KisNodeSP node)
{
    graphListenerRequests.append(node);
    effects.append(QStringLiteral("graph"));
    return graphListenerValue;
}

KisNodeSP KisNodeManager::LifecycleAccess::lastRootChild(KisNodeManager *)
{
    effects.append(QStringLiteral("root-last"));
    return lastRootChildValue;
}

void KisNodeManager::LifecycleAccess::activateNode(KisNodeManager *, KisNodeSP node)
{
    activationRequests.append(node);
    effects.append(QStringLiteral("activate"));
}

void KisNodeManager::LifecycleAccess::connectReselectionInput(KisNodeManager *)
{
    effects.append(QStringLiteral("connect-reselection"));
}

void KisNodeManager::LifecycleAccess::notifyResourceProvider(KisNodeManager *, KisNodeSP node)
{
    resourceProviderNotifications.append(node);
    effects.append(QStringLiteral("resource"));
}

void KisNodeManager::LifecycleAccess::connectIsolation(KisNodeManager *)
{
    effects.append(QStringLiteral("connect-isolation"));
}

void KisNodeManager::LifecycleAccess::updateLayerGui(KisNodeManager *)
{
    effects.append(QStringLiteral("update-layer"));
}

void KisNodeManager::LifecycleAccess::updateMaskGui(KisNodeManager *)
{
    effects.append(QStringLiteral("update-mask"));
}

class KisNodeManagerLifecycleContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void constructionOwnsStateUntilDestruction();
    void replacingViewRewiresConnectionsInOrder();
    void emptyViewRestoresLastActivatedNode();
    void detachedCurrentNodeFallsBackToLastRootChild();
    void clearingViewLeavesExternalConnectionsDetached();
    void guiUpdateDelegatesToLayerAndMaskManagers();
};

void KisNodeManagerLifecycleContractTest::init()
{
    privateStateValue = token<void>(1);
    destroyedStates.clear();
    imageViewAvailability.clear();
    currentNodeValues.clear();
    lastActivatedNodeValue.clear();
    lastRootChildValue.clear();
    graphListenerValue = true;
    graphListenerRequests.clear();
    activationRequests.clear();
    resourceProviderNotifications.clear();
    effects.clear();
}

void KisNodeManagerLifecycleContractTest::constructionOwnsStateUntilDestruction()
{
    auto *manager = new KisNodeManager(token<KisViewManager>(2));

    QCOMPARE(effects, QStringList({QStringLiteral("create"), QStringLiteral("connect-output")}));
    QVERIFY(destroyedStates.isEmpty());

    delete manager;

    QCOMPARE(destroyedStates, QList<void *>({privateStateValue}));
    QCOMPARE(effects,
             QStringList({QStringLiteral("create"), QStringLiteral("connect-output"), QStringLiteral("destroy")}));
}

void KisNodeManagerLifecycleContractTest::replacingViewRewiresConnectionsInOrder()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP currentNode = nodeToken(2);
    effects.clear();
    imageViewAvailability = {true, true};
    currentNodeValues = {currentNode, currentNode, currentNode};

    manager.setView(QPointer<KisView>());

    QCOMPARE(graphListenerRequests, KisNodeList({currentNode}));
    QCOMPARE(resourceProviderNotifications, KisNodeList({currentNode}));
    QVERIFY(activationRequests.isEmpty());
    QCOMPARE(effects,
             QStringList({QStringLiteral("mask-view"),
                          QStringLiteral("layer-view"),
                          QStringLiteral("has-view"),
                          QStringLiteral("disconnect-activation"),
                          QStringLiteral("disconnect-image"),
                          QStringLiteral("disconnect-reselection"),
                          QStringLiteral("assign-view"),
                          QStringLiteral("command-image"),
                          QStringLiteral("has-view"),
                          QStringLiteral("connect-activation"),
                          QStringLiteral("current"),
                          QStringLiteral("current"),
                          QStringLiteral("graph"),
                          QStringLiteral("connect-reselection"),
                          QStringLiteral("current"),
                          QStringLiteral("resource"),
                          QStringLiteral("connect-isolation")}));
}

void KisNodeManagerLifecycleContractTest::emptyViewRestoresLastActivatedNode()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP lastNode = nodeToken(2);
    effects.clear();
    imageViewAvailability = {false, true};
    currentNodeValues = {KisNodeSP(), KisNodeSP(), KisNodeSP()};
    lastActivatedNodeValue = lastNode;

    manager.setView(QPointer<KisView>());

    QCOMPARE(activationRequests, KisNodeList({lastNode}));
    QCOMPARE(resourceProviderNotifications, KisNodeList({KisNodeSP()}));
}

void KisNodeManagerLifecycleContractTest::detachedCurrentNodeFallsBackToLastRootChild()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP detachedNode = nodeToken(2);
    const KisNodeSP rootChild = nodeToken(3);
    effects.clear();
    imageViewAvailability = {false, true};
    currentNodeValues = {detachedNode, detachedNode, detachedNode};
    graphListenerValue = false;
    lastRootChildValue = rootChild;

    manager.setView(QPointer<KisView>());

    QCOMPARE(graphListenerRequests, KisNodeList({detachedNode}));
    QCOMPARE(activationRequests, KisNodeList({rootChild}));
    QCOMPARE(resourceProviderNotifications, KisNodeList({detachedNode}));
}

void KisNodeManagerLifecycleContractTest::clearingViewLeavesExternalConnectionsDetached()
{
    KisNodeManager manager(nullptr);
    effects.clear();
    imageViewAvailability = {true, false};

    manager.setView(QPointer<KisView>());

    QCOMPARE(effects,
             QStringList({QStringLiteral("mask-view"),
                          QStringLiteral("layer-view"),
                          QStringLiteral("has-view"),
                          QStringLiteral("disconnect-activation"),
                          QStringLiteral("disconnect-image"),
                          QStringLiteral("disconnect-reselection"),
                          QStringLiteral("assign-view"),
                          QStringLiteral("command-image"),
                          QStringLiteral("has-view")}));
    QVERIFY(activationRequests.isEmpty());
    QVERIFY(resourceProviderNotifications.isEmpty());
}

void KisNodeManagerLifecycleContractTest::guiUpdateDelegatesToLayerAndMaskManagers()
{
    KisNodeManager manager(nullptr);
    effects.clear();

    manager.updateGUI();

    QCOMPARE(effects, QStringList({QStringLiteral("update-layer"), QStringLiteral("update-mask")}));
}

QTEST_MAIN(KisNodeManagerLifecycleContractTest)

#include "KisNodeManagerLifecycleContractTest.moc"
