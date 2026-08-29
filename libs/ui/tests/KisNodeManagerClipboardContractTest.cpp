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

struct ClipboardWrite {
    KisNodeList nodes;
    bool copy;
};

struct PasteRequest {
    const QMimeData *data;
    KisNodeSP targetNode;
    bool copyNode;
    bool changeOffset;
    QPointF offset;
    KisProcessingApplicator *applicator;
};

KisNodeList selectedNodesValue;
KisNodeList parentedNodes;
KisNodeSP parentNodeValue;
bool canModifyResult = false;
const QMimeData *mimeDataValue = nullptr;
KisNodeSP activeNodeValue;
KisNodeSP rootNodeValue;
KisNodeList parentRequests;
QList<ClipboardWrite> clipboardWrites;
QList<KisNodeList> modificationRequests;
QList<KisNodeList> removalRequests;
QStringList removalActionNames;
QList<PasteRequest> pasteRequests;
QStringList events;
int selectedNodesRequests = 0;
int activeNodeRequests = 0;
int rootNodeRequests = 0;

} // namespace

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

KisNodeList KisNodeManager::ClipboardAccess::selectedNodes(KisNodeManager *)
{
    ++selectedNodesRequests;
    return selectedNodesValue;
}

KisNodeSP KisNodeManager::ClipboardAccess::parentNode(KisNodeSP node)
{
    parentRequests.append(node);
    return parentedNodes.contains(node) ? parentNodeValue : KisNodeSP();
}

void KisNodeManager::ClipboardAccess::setLayers(KisNodeManager *, const KisNodeList &nodes, bool copy)
{
    events.append(QStringLiteral("clipboard"));
    clipboardWrites.append({nodes, copy});
}

bool KisNodeManager::ClipboardAccess::canModifyLayers(KisNodeManager *, const KisNodeList &nodes)
{
    events.append(QStringLiteral("modify"));
    modificationRequests.append(nodes);
    return canModifyResult;
}

void KisNodeManager::ClipboardAccess::removeNodes(KisNodeManager *,
                                                  const KisNodeList &nodes,
                                                  const KUndo2MagicString &actionName)
{
    events.append(QStringLiteral("remove"));
    removalRequests.append(nodes);
    removalActionNames.append(actionName.toString());
}

const QMimeData *KisNodeManager::ClipboardAccess::layersMimeData()
{
    return mimeDataValue;
}

KisNodeSP KisNodeManager::ClipboardAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    return activeNodeValue;
}

KisNodeSP KisNodeManager::ClipboardAccess::rootNode(KisNodeManager *)
{
    ++rootNodeRequests;
    return rootNodeValue;
}

void KisNodeManager::ClipboardAccess::insertMimeLayersAsLastChild(KisNodeManager *,
                                                                  const QMimeData *data,
                                                                  KisNodeSP targetNode,
                                                                  bool copyNode,
                                                                  bool changeOffset,
                                                                  QPointF offset,
                                                                  KisProcessingApplicator *applicator)
{
    pasteRequests.append({data, targetNode, copyNode, changeOffset, offset, applicator});
}

class KisNodeManagerClipboardContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cutFiltersParentlessNodesAndChecksModification();
    void copyPreservesEntireSelection();
    void pasteRequiresMimeData();
    void pasteUsesActiveNodeOrRootAndPreservesArguments();
};

void KisNodeManagerClipboardContractTest::init()
{
    selectedNodesValue.clear();
    parentedNodes.clear();
    parentNodeValue.clear();
    canModifyResult = false;
    mimeDataValue = nullptr;
    activeNodeValue.clear();
    rootNodeValue.clear();
    parentRequests.clear();
    clipboardWrites.clear();
    modificationRequests.clear();
    removalRequests.clear();
    removalActionNames.clear();
    pasteRequests.clear();
    events.clear();
    selectedNodesRequests = 0;
    activeNodeRequests = 0;
    rootNodeRequests = 0;
}

void KisNodeManagerClipboardContractTest::cutFiltersParentlessNodesAndChecksModification()
{
    KisNodeManager manager(nullptr);

    manager.cutLayersToClipboard();
    QCOMPARE(selectedNodesRequests, 1);
    QVERIFY(parentRequests.isEmpty());
    QVERIFY(clipboardWrites.isEmpty());

    const KisNodeSP first = nodeToken(1);
    const KisNodeSP parentless = nodeToken(2);
    const KisNodeSP last = nodeToken(3);
    selectedNodesValue = {first, parentless, last};
    parentedNodes = {first, last};
    parentNodeValue = nodeToken(4);

    manager.cutLayersToClipboard();

    QCOMPARE(parentRequests, selectedNodesValue);
    QCOMPARE(clipboardWrites.size(), 1);
    QCOMPARE(clipboardWrites[0].nodes, (KisNodeList{first, last}));
    QCOMPARE(clipboardWrites[0].copy, false);
    QCOMPARE(modificationRequests, (QList<KisNodeList>{{first, last}}));
    QVERIFY(removalRequests.isEmpty());
    QCOMPARE(events, (QStringList{QStringLiteral("clipboard"), QStringLiteral("modify")}));

    canModifyResult = true;
    manager.cutLayersToClipboard();

    QCOMPARE(removalRequests, (QList<KisNodeList>{{first, last}}));
    QCOMPARE(removalActionNames, (QStringList{kundo2_i18n("Cut Nodes").toString()}));
    QCOMPARE(events.mid(2),
             (QStringList{QStringLiteral("clipboard"), QStringLiteral("modify"), QStringLiteral("remove")}));
}

void KisNodeManagerClipboardContractTest::copyPreservesEntireSelection()
{
    KisNodeManager manager(nullptr);
    const KisNodeSP first = nodeToken(1);
    const KisNodeSP second = nodeToken(2);
    selectedNodesValue = {first, second};

    manager.copyLayersToClipboard();

    QCOMPARE(selectedNodesRequests, 1);
    QVERIFY(parentRequests.isEmpty());
    QCOMPARE(clipboardWrites.size(), 1);
    QCOMPARE(clipboardWrites[0].nodes, selectedNodesValue);
    QCOMPARE(clipboardWrites[0].copy, true);
    QVERIFY(modificationRequests.isEmpty());
}

void KisNodeManagerClipboardContractTest::pasteRequiresMimeData()
{
    KisNodeManager manager(nullptr);

    manager.pasteLayersFromClipboard();

    QCOMPARE(activeNodeRequests, 0);
    QCOMPARE(rootNodeRequests, 0);
    QVERIFY(pasteRequests.isEmpty());
}

void KisNodeManagerClipboardContractTest::pasteUsesActiveNodeOrRootAndPreservesArguments()
{
    KisNodeManager manager(nullptr);
    mimeDataValue = token<QMimeData>(1);
    activeNodeValue = nodeToken(2);
    rootNodeValue = nodeToken(3);
    const QPointF offset(12.5, -8.0);
    KisProcessingApplicator *applicator = token<KisProcessingApplicator>(4);

    manager.pasteLayersFromClipboard(true, offset, applicator);

    QCOMPARE(activeNodeRequests, 1);
    QCOMPARE(rootNodeRequests, 0);
    QCOMPARE(pasteRequests.size(), 1);
    QCOMPARE(pasteRequests[0].data, mimeDataValue);
    QCOMPARE(pasteRequests[0].targetNode, activeNodeValue);
    QCOMPARE(pasteRequests[0].copyNode, false);
    QCOMPARE(pasteRequests[0].changeOffset, true);
    QCOMPARE(pasteRequests[0].offset, offset);
    QCOMPARE(pasteRequests[0].applicator, applicator);

    activeNodeValue.clear();
    manager.pasteLayersFromClipboard();

    QCOMPARE(activeNodeRequests, 2);
    QCOMPARE(rootNodeRequests, 1);
    QCOMPARE(pasteRequests.size(), 2);
    QCOMPARE(pasteRequests[1].targetNode, rootNodeValue);
    QCOMPARE(pasteRequests[1].copyNode, false);
    QCOMPARE(pasteRequests[1].changeOffset, false);
    QCOMPARE(pasteRequests[1].offset, QPointF());
    QCOMPARE(pasteRequests[1].applicator, nullptr);
}

QTEST_GUILESS_MAIN(KisNodeManagerClipboardContractTest)

#include "KisNodeManagerClipboardContractTest.moc"
