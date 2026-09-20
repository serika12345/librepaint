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

void kis_assert_recoverable(const char *, const char *, int)
{
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

bool graphListenerValue = true;
KisNodeSP activeNodeValue;
bool vectorAbilitiesValue = false;
QString activeToolIdValue;
KisDummiesFacadeBase *dummiesFacadeValue = nullptr;
bool nodeVisibleValue = true;
bool activateNodeResult = true;
bool canvasOnlyValue = false;
QString nodeNameValue;
KisNodeList graphListenerRequests;
KisNodeList vectorAbilityRequests;
QStringList switchedTools;
KisNodeList visibleNodeRequests;
KisNodeList activationRequests;
KisNodeList lastActivatedNodes;
KisNodeList uiNotifications;
KisNodeList activationNotifications;
QStringList shownNodeNames;
int activeNodeRequests = 0;
int dummiesFacadeRequests = 0;
int nodeUpdateCount = 0;
QStringList effects;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

bool KisNodeManager::ActivationAccess::hasGraphListener(KisNodeSP node)
{
    graphListenerRequests.append(node);
    effects.append(QStringLiteral("graph"));
    return graphListenerValue;
}

KisNodeSP KisNodeManager::ActivationAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    effects.append(QStringLiteral("active"));
    return activeNodeValue;
}

bool KisNodeManager::ActivationAccess::nodeHasVectorAbilities(KisNodeSP node)
{
    vectorAbilityRequests.append(node);
    effects.append(QStringLiteral("abilities"));
    return vectorAbilitiesValue;
}

QString KisNodeManager::ActivationAccess::activeToolId()
{
    effects.append(QStringLiteral("tool"));
    return activeToolIdValue;
}

void KisNodeManager::ActivationAccess::switchTool(const QString &toolId)
{
    switchedTools.append(toolId);
    effects.append(QStringLiteral("switch:%1").arg(toolId));
}

KisDummiesFacadeBase *KisNodeManager::ActivationAccess::dummiesFacade(KisNodeManager *)
{
    ++dummiesFacadeRequests;
    effects.append(QStringLiteral("facade"));
    return dummiesFacadeValue;
}

bool KisNodeManager::ActivationAccess::isNodeVisible(KisNodeManager *, KisNodeSP node)
{
    visibleNodeRequests.append(node);
    effects.append(QStringLiteral("visible"));
    return nodeVisibleValue;
}

bool KisNodeManager::ActivationAccess::activateNode(KisNodeManager *, KisNodeSP node)
{
    activationRequests.append(node);
    effects.append(QStringLiteral("activate"));
    return activateNodeResult;
}

void KisNodeManager::ActivationAccess::setLastActivatedNode(KisDummiesFacadeBase *, KisNodeSP node)
{
    lastActivatedNodes.append(node);
    effects.append(QStringLiteral("last"));
}

void KisNodeManager::ActivationAccess::notifyUiNodeChange(KisNodeManager *, KisNodeSP node)
{
    uiNotifications.append(node);
    effects.append(QStringLiteral("ui"));
}

void KisNodeManager::ActivationAccess::notifyNodeActivated(KisNodeManager *, KisNodeSP node)
{
    activationNotifications.append(node);
    effects.append(QStringLiteral("node"));
}

void KisNodeManager::ActivationAccess::nodesUpdated(KisNodeManager *)
{
    ++nodeUpdateCount;
    effects.append(QStringLiteral("updated"));
}

bool KisNodeManager::ActivationAccess::canvasOnly(KisNodeManager *)
{
    effects.append(QStringLiteral("canvas"));
    return canvasOnlyValue;
}

QString KisNodeManager::ActivationAccess::nodeName(KisNodeSP)
{
    effects.append(QStringLiteral("name"));
    return nodeNameValue;
}

void KisNodeManager::ActivationAccess::showNodeName(KisNodeManager *, const QString &name)
{
    shownNodeNames.append(name);
    effects.append(QStringLiteral("message:%1").arg(name));
}

class KisNodeManagerActivationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void detachedNodeBecomesNullBeforeActivation();
    void sameNonUiNodeIsIgnored();
    void uiActivationKeepsToolCompatible_data();
    void uiActivationKeepsToolCompatible();
    void unavailableFacadeAndHiddenNodeRejectActivation();
    void failedActivationPublishesNothing();
    void successfulUiActivationPublishesBothNotifications();
    void successfulActivationOrdersStateAndNotifications();
};

void KisNodeManagerActivationContractTest::init()
{
    graphListenerValue = true;
    activeNodeValue.clear();
    vectorAbilitiesValue = false;
    activeToolIdValue.clear();
    dummiesFacadeValue = token<KisDummiesFacadeBase>(1);
    nodeVisibleValue = true;
    activateNodeResult = true;
    canvasOnlyValue = false;
    nodeNameValue.clear();
    graphListenerRequests.clear();
    vectorAbilityRequests.clear();
    switchedTools.clear();
    visibleNodeRequests.clear();
    activationRequests.clear();
    lastActivatedNodes.clear();
    uiNotifications.clear();
    activationNotifications.clear();
    shownNodeNames.clear();
    activeNodeRequests = 0;
    dummiesFacadeRequests = 0;
    nodeUpdateCount = 0;
    effects.clear();
}

void KisNodeManagerActivationContractTest::detachedNodeBecomesNullBeforeActivation()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP detachedNode = nodeToken(2);
    activeNodeValue = nodeToken(3);
    graphListenerValue = false;

    manager.slotNonUiActivatedNode(detachedNode);

    QCOMPARE(graphListenerRequests, KisNodeList({detachedNode}));
    QCOMPARE(activationRequests, KisNodeList({KisNodeSP()}));
    QVERIFY(lastActivatedNodes.isEmpty());
    QCOMPARE(uiNotifications, KisNodeList({KisNodeSP()}));
    QCOMPARE(activationNotifications, KisNodeList({KisNodeSP()}));
    QCOMPARE(nodeUpdateCount, 1);
    QVERIFY(shownNodeNames.isEmpty());
}

void KisNodeManagerActivationContractTest::sameNonUiNodeIsIgnored()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(2);
    activeNodeValue = node;

    manager.slotNonUiActivatedNode(node);

    QCOMPARE(graphListenerRequests, KisNodeList({node}));
    QCOMPARE(activeNodeRequests, 1);
    QCOMPARE(dummiesFacadeRequests, 0);
    QVERIFY(activationRequests.isEmpty());
    QVERIFY(uiNotifications.isEmpty());
    QVERIFY(activationNotifications.isEmpty());
}

void KisNodeManagerActivationContractTest::uiActivationKeepsToolCompatible_data()
{
    QTest::addColumn<bool>("vectorAbilities");
    QTest::addColumn<QString>("activeTool");
    QTest::addColumn<QString>("expectedTool");

    QTest::newRow("vector-node-from-pixel-tool")
        << true << QStringLiteral("KritaShape/KisToolBrush") << QStringLiteral("InteractionTool");
    QTest::newRow("pixel-node-from-vector-tool")
        << false << QStringLiteral("PathTool") << QStringLiteral("KritaShape/KisToolBrush");
    QTest::newRow("vector-node-from-vector-tool") << true << QStringLiteral("PathTool") << QString();
    QTest::newRow("pixel-node-from-pixel-tool") << false << QStringLiteral("KritaFill/KisToolGradient") << QString();
}

void KisNodeManagerActivationContractTest::uiActivationKeepsToolCompatible()
{
    QFETCH(bool, vectorAbilities);
    QFETCH(QString, activeTool);
    QFETCH(QString, expectedTool);

    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(2);
    activeNodeValue = node;
    vectorAbilitiesValue = vectorAbilities;
    activeToolIdValue = activeTool;

    manager.slotUiActivatedNode(node);

    QCOMPARE(vectorAbilityRequests, KisNodeList({node}));
    QCOMPARE(switchedTools, expectedTool.isEmpty() ? QStringList() : QStringList({expectedTool}));
    QCOMPARE(dummiesFacadeRequests, 0);
    QVERIFY(activationRequests.isEmpty());
}

void KisNodeManagerActivationContractTest::unavailableFacadeAndHiddenNodeRejectActivation()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(2);
    activeNodeValue = nodeToken(3);
    dummiesFacadeValue = nullptr;

    manager.slotNonUiActivatedNode(node);

    QVERIFY(visibleNodeRequests.isEmpty());
    QVERIFY(activationRequests.isEmpty());

    dummiesFacadeValue = token<KisDummiesFacadeBase>(1);
    nodeVisibleValue = false;
    effects.clear();

    manager.slotNonUiActivatedNode(node);

    QCOMPARE(visibleNodeRequests, KisNodeList({node}));
    QVERIFY(activationRequests.isEmpty());
    QVERIFY(uiNotifications.isEmpty());
    QVERIFY(activationNotifications.isEmpty());
}

void KisNodeManagerActivationContractTest::failedActivationPublishesNothing()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(2);
    activeNodeValue = nodeToken(3);
    activateNodeResult = false;

    manager.slotNonUiActivatedNode(node);

    QCOMPARE(activationRequests, KisNodeList({node}));
    QVERIFY(lastActivatedNodes.isEmpty());
    QVERIFY(uiNotifications.isEmpty());
    QVERIFY(activationNotifications.isEmpty());
    QCOMPARE(nodeUpdateCount, 0);
}

void KisNodeManagerActivationContractTest::successfulUiActivationPublishesBothNotifications()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(2);
    activeNodeValue = nodeToken(3);
    activeToolIdValue = QStringLiteral("KritaShape/KisToolBrush");

    manager.slotUiActivatedNode(node);

    QCOMPARE(uiNotifications, KisNodeList({node}));
    QCOMPARE(activationNotifications, KisNodeList({node}));
    QCOMPARE(effects,
             QStringList({QStringLiteral("graph"),
                          QStringLiteral("abilities"),
                          QStringLiteral("tool"),
                          QStringLiteral("active"),
                          QStringLiteral("facade"),
                          QStringLiteral("visible"),
                          QStringLiteral("active"),
                          QStringLiteral("activate"),
                          QStringLiteral("last"),
                          QStringLiteral("ui"),
                          QStringLiteral("node"),
                          QStringLiteral("updated"),
                          QStringLiteral("canvas")}));
}

void KisNodeManagerActivationContractTest::successfulActivationOrdersStateAndNotifications()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(2);
    activeNodeValue = nodeToken(3);
    canvasOnlyValue = true;
    nodeNameValue = QStringLiteral("Ink");

    manager.slotNonUiActivatedNode(node);

    QCOMPARE(lastActivatedNodes, KisNodeList({node}));
    QCOMPARE(uiNotifications, KisNodeList({node}));
    QCOMPARE(activationNotifications, KisNodeList({node}));
    QCOMPARE(nodeUpdateCount, 1);
    QCOMPARE(shownNodeNames, QStringList({QStringLiteral("Ink")}));
    QCOMPARE(effects,
             QStringList({QStringLiteral("graph"),
                          QStringLiteral("active"),
                          QStringLiteral("facade"),
                          QStringLiteral("visible"),
                          QStringLiteral("active"),
                          QStringLiteral("activate"),
                          QStringLiteral("last"),
                          QStringLiteral("ui"),
                          QStringLiteral("node"),
                          QStringLiteral("updated"),
                          QStringLiteral("canvas"),
                          QStringLiteral("name"),
                          QStringLiteral("message:Ink")}));
}

QTEST_MAIN(KisNodeManagerActivationContractTest)

#include "KisNodeManagerActivationContractTest.moc"
