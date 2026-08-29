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

enum IsolationEffect {
    ToggleLayer,
    Start,
    Stop,
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

bool imageAvailableValue = false;
bool isolatingLayerValue = false;
bool isolatingGroupValue = false;
KisNodeSP activeNodeValue;
bool startSucceeds = true;
bool activeWindowValue = false;
int imageRequests = 0;
int layerStateRequests = 0;
int groupStateRequests = 0;
int activeNodeRequests = 0;
int activeWindowRequests = 0;
QList<IsolationEffect> effects;
KisNodeList startRoots;
QList<bool> startLayerStates;
QList<bool> startGroupStates;
QList<bool> layerActionChecks;
QList<bool> groupActionChecks;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

bool KisNodeManager::IsolationAccess::imageAvailable(KisNodeManager *)
{
    ++imageRequests;
    return imageAvailableValue;
}

bool KisNodeManager::IsolationAccess::isIsolatingLayer(KisNodeManager *)
{
    ++layerStateRequests;
    return isolatingLayerValue;
}

bool KisNodeManager::IsolationAccess::isIsolatingGroup(KisNodeManager *)
{
    ++groupStateRequests;
    return isolatingGroupValue;
}

KisNodeSP KisNodeManager::IsolationAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    return activeNodeValue;
}

bool KisNodeManager::IsolationAccess::startIsolatedMode(KisNodeManager *,
                                                        KisNodeSP isolationRoot,
                                                        bool isolateActiveLayer,
                                                        bool isolateActiveGroup)
{
    effects.append(Start);
    startRoots.append(isolationRoot);
    startLayerStates.append(isolateActiveLayer);
    startGroupStates.append(isolateActiveGroup);
    return startSucceeds;
}

void KisNodeManager::IsolationAccess::stopIsolatedMode(KisNodeManager *)
{
    effects.append(Stop);
}

bool KisNodeManager::IsolationAccess::isActiveWindow(KisNodeManager *)
{
    ++activeWindowRequests;
    return activeWindowValue;
}

void KisNodeManager::IsolationAccess::toggleLayerAction(KisNodeManager *)
{
    effects.append(ToggleLayer);
}

void KisNodeManager::IsolationAccess::setLayerActionChecked(KisNodeManager *, bool checked)
{
    layerActionChecks.append(checked);
}

void KisNodeManager::IsolationAccess::setGroupActionChecked(KisNodeManager *, bool checked)
{
    groupActionChecks.append(checked);
}

class KisNodeManagerIsolationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void toggleRequestsTheLayerAction();
    void layerModePreservesGroupState();
    void groupModePreservesLayerState();
    void isolationModeRequiresImageAndNode();
    void isolationRootPreservesMode();
    void externalChangeRequiresActiveWindow();
    void reinitializeClearsActions();
};

void KisNodeManagerIsolationContractTest::init()
{
    imageAvailableValue = false;
    isolatingLayerValue = false;
    isolatingGroupValue = false;
    activeNodeValue.clear();
    startSucceeds = true;
    activeWindowValue = false;
    imageRequests = 0;
    layerStateRequests = 0;
    groupStateRequests = 0;
    activeNodeRequests = 0;
    activeWindowRequests = 0;
    effects.clear();
    startRoots.clear();
    startLayerStates.clear();
    startGroupStates.clear();
    layerActionChecks.clear();
    groupActionChecks.clear();
}

void KisNodeManagerIsolationContractTest::toggleRequestsTheLayerAction()
{
    KisNodeManager manager(nullptr);

    manager.toggleIsolateActiveNode();

    QCOMPARE(effects, (QList<IsolationEffect>{ToggleLayer}));
}

void KisNodeManagerIsolationContractTest::layerModePreservesGroupState()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP active = nodeToken(1);

    manager.setIsolateActiveLayerMode(true);
    QVERIFY(effects.isEmpty());
    QCOMPARE(groupStateRequests, 0);

    imageAvailableValue = true;
    isolatingGroupValue = true;
    activeNodeValue = active;
    manager.setIsolateActiveLayerMode(false);

    QCOMPARE(groupStateRequests, 1);
    QCOMPARE(startRoots, (KisNodeList{active}));
    QCOMPARE(startLayerStates, (QList<bool>{false}));
    QCOMPARE(startGroupStates, (QList<bool>{true}));
}

void KisNodeManagerIsolationContractTest::groupModePreservesLayerState()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP active = nodeToken(1);
    imageAvailableValue = true;
    isolatingLayerValue = true;
    activeNodeValue = active;

    manager.setIsolateActiveGroupMode(false);

    QCOMPARE(layerStateRequests, 1);
    QCOMPARE(startRoots, (KisNodeList{active}));
    QCOMPARE(startLayerStates, (QList<bool>{true}));
    QCOMPARE(startGroupStates, (QList<bool>{false}));
}

void KisNodeManagerIsolationContractTest::isolationModeRequiresImageAndNode()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP active = nodeToken(1);

    manager.changeIsolationMode(true, false);
    QCOMPARE(activeNodeRequests, 1);
    QVERIFY(effects.isEmpty());

    imageAvailableValue = true;
    manager.changeIsolationMode(true, false);
    QCOMPARE(activeNodeRequests, 2);
    QVERIFY(effects.isEmpty());

    activeNodeValue = active;
    manager.changeIsolationMode(false, false);
    QCOMPARE(effects, (QList<IsolationEffect>{Stop}));

    manager.changeIsolationMode(true, false);
    QCOMPARE(effects, (QList<IsolationEffect>{Stop, Start}));
    QCOMPARE(startRoots, (KisNodeList{active}));
    QCOMPARE(startLayerStates, (QList<bool>{true}));
    QCOMPARE(startGroupStates, (QList<bool>{false}));

    startSucceeds = false;
    manager.changeIsolationMode(false, true);
    QCOMPARE(effects, (QList<IsolationEffect>{Stop, Start, Start}));
    QCOMPARE(layerActionChecks, (QList<bool>{false}));
    QCOMPARE(groupActionChecks, (QList<bool>{false}));
}

void KisNodeManagerIsolationContractTest::isolationRootPreservesMode()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP root = nodeToken(1);

    manager.changeIsolationRoot(root);
    imageAvailableValue = true;
    manager.changeIsolationRoot(KisNodeSP());
    QVERIFY(effects.isEmpty());
    QCOMPARE(layerStateRequests, 0);
    QCOMPARE(groupStateRequests, 0);

    isolatingLayerValue = true;
    manager.changeIsolationRoot(root);
    QCOMPARE(startRoots, (KisNodeList{root}));
    QCOMPARE(startLayerStates, (QList<bool>{true}));
    QCOMPARE(startGroupStates, (QList<bool>{false}));

    startSucceeds = false;
    manager.changeIsolationRoot(root);
    QCOMPARE(startRoots, (KisNodeList{root, root}));
    QCOMPARE(layerActionChecks, (QList<bool>{false}));
    QCOMPARE(groupActionChecks, (QList<bool>{false}));
}

void KisNodeManagerIsolationContractTest::externalChangeRequiresActiveWindow()
{
    KisNodeManager manager(nullptr);

    manager.handleExternalIsolationChange();
    QCOMPARE(activeWindowRequests, 1);
    QCOMPARE(layerStateRequests, 0);
    QCOMPARE(groupStateRequests, 0);

    activeWindowValue = true;
    isolatingGroupValue = true;
    manager.handleExternalIsolationChange();

    QCOMPARE(activeWindowRequests, 2);
    QCOMPARE(layerStateRequests, 1);
    QCOMPARE(groupStateRequests, 1);
    QCOMPARE(layerActionChecks, (QList<bool>{false}));
    QCOMPARE(groupActionChecks, (QList<bool>{true}));
}

void KisNodeManagerIsolationContractTest::reinitializeClearsActions()
{
    KisNodeManager manager(nullptr);

    manager.reinitializeIsolationActionGroup();

    QCOMPARE(layerActionChecks, (QList<bool>{false}));
    QCOMPARE(groupActionChecks, (QList<bool>{false}));
}

QTEST_GUILESS_MAIN(KisNodeManagerIsolationContractTest)

#include "KisNodeManagerIsolationContractTest.moc"
