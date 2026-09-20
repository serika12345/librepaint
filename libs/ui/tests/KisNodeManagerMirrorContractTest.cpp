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

#include <kundo2magicstring.h>

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
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

QString KUndo2MagicString::toSecondaryString() const
{
    const int separator = m_text.indexOf(QLatin1Char('\n'));
    return separator > 0 ? m_text.mid(separator + 1) : m_text;
}

bool KUndo2MagicString::isEmpty() const
{
    return m_text.isEmpty();
}

bool KUndo2MagicString::operator==(const KUndo2MagicString &rhs) const
{
    return m_text == rhs.m_text;
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

KisSelectionSP selectionToken(quintptr id)
{
    return KisSelectionSP(token<KisSelection>(id));
}

struct MirrorApplication {
    KisNodeList nodes;
    Qt::Orientation orientation;
    KisSelectionSP selection;
    QString actionName;
};

KisNodeList selectedNodesValue;
KisNodeList maskNodes;
KisSelectionSP selectionValue;
KisNodeSP rootNodeValue;
KisNodeList modifiableNodes;
KisNodeList maskRequests;
KisNodeList modificationRequests;
QList<MirrorApplication> applications;
int selectedNodesRequests = 0;
int selectionRequests = 0;
int rootNodeRequests = 0;
int nodesUpdatedCount = 0;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeList KisNodeManager::MirrorAccess::selectedNodes(KisNodeManager *)
{
    ++selectedNodesRequests;
    return selectedNodesValue;
}

bool KisNodeManager::MirrorAccess::isMask(KisNodeSP node)
{
    maskRequests.append(node);
    return maskNodes.contains(node);
}

KisSelectionSP KisNodeManager::MirrorAccess::selection(KisNodeManager *)
{
    ++selectionRequests;
    return selectionValue;
}

KisNodeSP KisNodeManager::MirrorAccess::rootNode(KisNodeManager *)
{
    ++rootNodeRequests;
    return rootNodeValue;
}

bool KisNodeManager::MirrorAccess::canModifyLayer(KisNodeManager *, KisNodeSP node)
{
    modificationRequests.append(node);
    return modifiableNodes.contains(node);
}

void KisNodeManager::MirrorAccess::applyToNodes(KisNodeManager *,
                                                const KisNodeList &nodes,
                                                Qt::Orientation orientation,
                                                KisSelectionSP selection,
                                                const KUndo2MagicString &actionName)
{
    applications.append({nodes, orientation, selection, actionName.toString()});
}

void KisNodeManager::MirrorAccess::nodesUpdated(KisNodeManager *)
{
    ++nodesUpdatedCount;
}

class KisNodeManagerMirrorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void horizontalSelectedMirrorUsesMaskCommand();
    void verticalSelectedMirrorUsesLayerCount();
    void wholeImageMirrorsUseRootNode();
    void singleNodeMirrorPreservesArguments();
    void multipleNodeMirrorRequiresEveryNodeToBeModifiable();
};

void KisNodeManagerMirrorContractTest::init()
{
    selectedNodesValue.clear();
    maskNodes.clear();
    selectionValue.clear();
    rootNodeValue.clear();
    modifiableNodes.clear();
    maskRequests.clear();
    modificationRequests.clear();
    applications.clear();
    selectedNodesRequests = 0;
    selectionRequests = 0;
    rootNodeRequests = 0;
    nodesUpdatedCount = 0;
}

void KisNodeManagerMirrorContractTest::horizontalSelectedMirrorUsesMaskCommand()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP mask = nodeToken(1);
    selectedNodesValue = {mask};
    maskNodes = {mask};
    selectionValue = selectionToken(2);
    modifiableNodes = {mask};

    manager.mirrorNodeX();

    QCOMPARE(selectedNodesRequests, 1);
    QCOMPARE(maskRequests, (KisNodeList{mask}));
    QCOMPARE(selectionRequests, 1);
    QCOMPARE(modificationRequests, (KisNodeList{mask}));
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications[0].nodes, (KisNodeList{mask}));
    QCOMPARE(applications[0].orientation, Qt::Horizontal);
    QCOMPARE(applications[0].selection, selectionValue);
    QCOMPARE(applications[0].actionName, kundo2_i18n("Mirror Mask Horizontally").toString());
    QCOMPARE(nodesUpdatedCount, 1);
}

void KisNodeManagerMirrorContractTest::verticalSelectedMirrorUsesLayerCount()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP first = nodeToken(1);
    const KisNodeSP second = nodeToken(2);
    selectedNodesValue = {first, second};
    selectionValue = selectionToken(3);
    modifiableNodes = selectedNodesValue;

    manager.mirrorNodeY();

    QCOMPARE(maskRequests.size(), 0);
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications[0].nodes, selectedNodesValue);
    QCOMPARE(applications[0].orientation, Qt::Vertical);
    QCOMPARE(applications[0].selection, selectionValue);
    QCOMPARE(applications[0].actionName,
             kundo2_i18np("Mirror Layer Vertically", "Mirror %1 Layers Vertically", 2).toString());
    QCOMPARE(nodesUpdatedCount, 1);
}

void KisNodeManagerMirrorContractTest::wholeImageMirrorsUseRootNode()
{
    KisNodeManager manager(nullptr);
    rootNodeValue = nodeToken(1);
    selectionValue = selectionToken(2);
    modifiableNodes = {rootNodeValue};

    manager.mirrorAllNodesX();
    manager.mirrorAllNodesY();

    QCOMPARE(rootNodeRequests, 2);
    QCOMPARE(selectionRequests, 2);
    QCOMPARE(applications.size(), 2);
    QCOMPARE(applications[0].nodes, (KisNodeList{rootNodeValue}));
    QCOMPARE(applications[0].orientation, Qt::Horizontal);
    QCOMPARE(applications[0].selection, selectionValue);
    QCOMPARE(applications[0].actionName, kundo2_i18n("Mirror All Layers Horizontally").toString());
    QCOMPARE(applications[1].nodes, (KisNodeList{rootNodeValue}));
    QCOMPARE(applications[1].orientation, Qt::Vertical);
    QCOMPARE(applications[1].selection, selectionValue);
    QCOMPARE(applications[1].actionName, kundo2_i18n("Mirror All Layers Vertically").toString());
    QCOMPARE(nodesUpdatedCount, 2);
}

void KisNodeManagerMirrorContractTest::singleNodeMirrorPreservesArguments()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP node = nodeToken(1);
    const KisSelectionSP selection = selectionToken(2);
    const KUndo2MagicString actionName = kundo2_noi18n(QStringLiteral("Provided mirror action"));
    modifiableNodes = {node};

    manager.mirrorNode(node, actionName, Qt::Vertical, selection);

    QCOMPARE(modificationRequests, (KisNodeList{node}));
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications[0].nodes, (KisNodeList{node}));
    QCOMPARE(applications[0].orientation, Qt::Vertical);
    QCOMPARE(applications[0].selection, selection);
    QCOMPARE(applications[0].actionName, actionName.toString());
    QCOMPARE(nodesUpdatedCount, 1);
}

void KisNodeManagerMirrorContractTest::multipleNodeMirrorRequiresEveryNodeToBeModifiable()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP first = nodeToken(1);
    const KisNodeSP blocked = nodeToken(2);
    const KisNodeSP last = nodeToken(3);
    const KisNodeList nodes = {first, blocked, last};
    const KUndo2MagicString actionName = kundo2_noi18n(QStringLiteral("Provided mirror action"));
    modifiableNodes = {first, last};

    manager.mirrorNodes(nodes, actionName, Qt::Horizontal, KisSelectionSP());

    QCOMPARE(modificationRequests, (KisNodeList{first, blocked}));
    QVERIFY(applications.isEmpty());
    QCOMPARE(nodesUpdatedCount, 0);

    modificationRequests.clear();
    modifiableNodes = nodes;
    manager.mirrorNodes(nodes, actionName, Qt::Horizontal, KisSelectionSP());

    QCOMPARE(modificationRequests, nodes);
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications[0].nodes, nodes);
    QCOMPARE(applications[0].orientation, Qt::Horizontal);
    QVERIFY(!applications[0].selection);
    QCOMPARE(applications[0].actionName, actionName.toString());
    QCOMPARE(nodesUpdatedCount, 1);
}

QTEST_GUILESS_MAIN(KisNodeManagerMirrorContractTest)

#include "KisNodeManagerMirrorContractTest.moc"
