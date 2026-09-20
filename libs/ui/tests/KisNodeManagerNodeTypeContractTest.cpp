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

class NodeTypeNames : public KisNodeManager
{
public:
    using CreationKind = NodeCreationKind;
    using ConversionKind = NodeConversionKind;
};

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

QString creationKindName(NodeTypeNames::CreationKind kind)
{
    using Kind = NodeTypeNames::CreationKind;
    switch (kind) {
    case Kind::PaintLayer:
        return QStringLiteral("KisPaintLayer");
    case Kind::GroupLayer:
        return QStringLiteral("KisGroupLayer");
    case Kind::AdjustmentLayer:
        return QStringLiteral("KisAdjustmentLayer");
    case Kind::GeneratorLayer:
        return QStringLiteral("KisGeneratorLayer");
    case Kind::ShapeLayer:
        return QStringLiteral("KisShapeLayer");
    case Kind::CloneLayer:
        return QStringLiteral("KisCloneLayer");
    case Kind::TransparencyMask:
        return QStringLiteral("KisTransparencyMask");
    case Kind::FilterMask:
        return QStringLiteral("KisFilterMask");
    case Kind::FastColorOverlayMask:
        return QStringLiteral("FastColorOverlayFilterMask");
    case Kind::ColorizeMask:
        return QStringLiteral("KisColorizeMask");
    case Kind::TransformMask:
        return QStringLiteral("KisTransformMask");
    case Kind::SelectionMask:
        return QStringLiteral("KisSelectionMask");
    case Kind::FileLayer:
        return QStringLiteral("KisFileLayer");
    }
    return QString();
}

QString conversionKindName(NodeTypeNames::ConversionKind kind)
{
    using Kind = NodeTypeNames::ConversionKind;
    switch (kind) {
    case Kind::PaintLayer:
        return QStringLiteral("KisPaintLayer");
    case Kind::SelectionMask:
        return QStringLiteral("KisSelectionMask");
    case Kind::FilterMask:
        return QStringLiteral("KisFilterMask");
    case Kind::TransparencyMask:
        return QStringLiteral("KisTransparencyMask");
    case Kind::FileLayer:
        return QStringLiteral("KisFileLayer");
    }
    return QString();
}

bool finishPendingOperationsResult = true;
KisNodeSP activeNodeValue;
KisNodeSP rootNodeValue;
KisNodeList selectedNodesValue;
KisNodeSP createdNodeValue;
bool canModifyResult = true;
KisPaintDevice *paintDeviceValue = nullptr;
KisPaintDevice *projectionValue = nullptr;
bool maskConversionResult = true;
int finishPendingOperationsRequests = 0;
int activeNodeRequests = 0;
int rootNodeRequests = 0;
int selectedNodesRequests = 0;
QStringList creationKinds;
KisNodeList creationActiveNodes;
QList<KisNodeList> creationSelectedNodes;
QList<KisPaintDevice *> creationCopyDevices;
QList<bool> creationQuietValues;
KisNodeList modificationRequests;
KisNodeList paintDeviceRequests;
KisNodeList projectionRequests;
QStringList conversionActionNames;
QStringList maskConversionKinds;
KisNodeList maskConversionNodes;
QList<KisPaintDevice *> maskConversionDevices;
QStringList directConversionKinds;
KisNodeList directConversionNodes;
QStringList unsupportedNodeTypes;
int conversionEndRequests = 0;
int forcedFinishRequests = 0;
int undoRequests = 0;
int recoverableAssertionCount = 0;

} // namespace

void kis_assert_recoverable(const char *, const char *, int)
{
    ++recoverableAssertionCount;
}

KisNodeManager::KisNodeManager(KisViewManager *)
    : QObject(nullptr)
    , m_d(nullptr)
{
}

KisNodeManager::~KisNodeManager() = default;

bool KisNodeManager::NodeTypeAccess::finishPendingOperations(KisNodeManager *)
{
    ++finishPendingOperationsRequests;
    return finishPendingOperationsResult;
}

KisNodeSP KisNodeManager::NodeTypeAccess::activeNode(KisNodeManager *)
{
    ++activeNodeRequests;
    return activeNodeValue;
}

KisNodeSP KisNodeManager::NodeTypeAccess::rootNode(KisNodeManager *)
{
    ++rootNodeRequests;
    return rootNodeValue;
}

KisNodeList KisNodeManager::NodeTypeAccess::selectedNodes(KisNodeManager *)
{
    ++selectedNodesRequests;
    return selectedNodesValue;
}

KisNodeSP KisNodeManager::NodeTypeAccess::createNode(KisNodeManager *,
                                                     NodeCreationKind kind,
                                                     KisNodeSP activeNode,
                                                     const KisNodeList &selectedNodes,
                                                     KisPaintDevice *copyFrom,
                                                     bool quiet)
{
    creationKinds.append(creationKindName(kind));
    creationActiveNodes.append(activeNode);
    creationSelectedNodes.append(selectedNodes);
    creationCopyDevices.append(copyFrom);
    creationQuietValues.append(quiet);
    return createdNodeValue;
}

bool KisNodeManager::NodeTypeAccess::canModifyLayer(KisNodeManager *, KisNodeSP node)
{
    modificationRequests.append(node);
    return canModifyResult;
}

KisPaintDevice *KisNodeManager::NodeTypeAccess::paintDevice(KisNodeSP node)
{
    paintDeviceRequests.append(node);
    return paintDeviceValue;
}

KisPaintDevice *KisNodeManager::NodeTypeAccess::projection(KisNodeSP node)
{
    projectionRequests.append(node);
    return projectionValue;
}

void KisNodeManager::NodeTypeAccess::beginConversion(KisNodeManager *, const KUndo2MagicString &actionName)
{
    conversionActionNames.append(actionName.toString());
}

bool KisNodeManager::NodeTypeAccess::convertToMask(KisNodeManager *,
                                                   NodeConversionKind kind,
                                                   KisNodeSP node,
                                                   KisPaintDevice *copyFrom)
{
    maskConversionKinds.append(conversionKindName(kind));
    maskConversionNodes.append(node);
    maskConversionDevices.append(copyFrom);
    return maskConversionResult;
}

void KisNodeManager::NodeTypeAccess::endConversion(KisNodeManager *)
{
    ++conversionEndRequests;
}

void KisNodeManager::NodeTypeAccess::convertNode(KisNodeManager *, NodeConversionKind kind, KisNodeSP node)
{
    directConversionKinds.append(conversionKindName(kind));
    directConversionNodes.append(node);
}

void KisNodeManager::NodeTypeAccess::finishPendingOperationsForced(KisNodeManager *)
{
    ++forcedFinishRequests;
}

void KisNodeManager::NodeTypeAccess::undoLastConversion(KisNodeManager *)
{
    ++undoRequests;
}

void KisNodeManager::NodeTypeAccess::reportUnsupportedNodeType(const QString &nodeType)
{
    unsupportedNodeTypes.append(nodeType);
}

class KisNodeManagerNodeTypeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void creationRequiresFinishedOperationsAndUsesRootFallback();
    void creationTypes_data();
    void creationTypes();
    void cloneCreationFallsBackToActiveNode();
    void unsupportedCreationReturnsNull();
    void conversionRequiresFinishedOperationsAndModifiableNode();
    void directConversionTypes_data();
    void directConversionTypes();
    void maskConversionTypes_data();
    void maskConversionTypes();
    void failedMaskConversionUsesProjectionAndUndo();
    void unsupportedConversionIsReported();
};

void KisNodeManagerNodeTypeContractTest::init()
{
    finishPendingOperationsResult = true;
    activeNodeValue.clear();
    rootNodeValue.clear();
    selectedNodesValue.clear();
    createdNodeValue.clear();
    canModifyResult = true;
    paintDeviceValue = nullptr;
    projectionValue = nullptr;
    maskConversionResult = true;
    finishPendingOperationsRequests = 0;
    activeNodeRequests = 0;
    rootNodeRequests = 0;
    selectedNodesRequests = 0;
    creationKinds.clear();
    creationActiveNodes.clear();
    creationSelectedNodes.clear();
    creationCopyDevices.clear();
    creationQuietValues.clear();
    modificationRequests.clear();
    paintDeviceRequests.clear();
    projectionRequests.clear();
    conversionActionNames.clear();
    maskConversionKinds.clear();
    maskConversionNodes.clear();
    maskConversionDevices.clear();
    directConversionKinds.clear();
    directConversionNodes.clear();
    unsupportedNodeTypes.clear();
    conversionEndRequests = 0;
    forcedFinishRequests = 0;
    undoRequests = 0;
    recoverableAssertionCount = 0;
}

void KisNodeManagerNodeTypeContractTest::creationRequiresFinishedOperationsAndUsesRootFallback()
{
    KisNodeManager manager(nullptr);
    finishPendingOperationsResult = false;

    QVERIFY(manager.createNode(QStringLiteral("KisPaintLayer")).isNull());
    QCOMPARE(finishPendingOperationsRequests, 1);
    QCOMPARE(activeNodeRequests, 0);
    QVERIFY(creationKinds.isEmpty());

    finishPendingOperationsResult = true;
    QVERIFY(manager.createNode(QStringLiteral("KisPaintLayer")).isNull());
    QCOMPARE(recoverableAssertionCount, 1);

    rootNodeValue = nodeToken(1);
    createdNodeValue = nodeToken(2);

    QCOMPARE(manager.createNode(QStringLiteral("KisPaintLayer")), createdNodeValue);
    QCOMPARE(activeNodeRequests, 2);
    QCOMPARE(rootNodeRequests, 2);
    QCOMPARE(creationActiveNodes, (KisNodeList{rootNodeValue}));
}

void KisNodeManagerNodeTypeContractTest::creationTypes_data()
{
    QTest::addColumn<QString>("nodeType");

    const QStringList nodeTypes = {
        QStringLiteral("KisPaintLayer"),
        QStringLiteral("KisGroupLayer"),
        QStringLiteral("KisAdjustmentLayer"),
        QStringLiteral("KisGeneratorLayer"),
        QStringLiteral("KisShapeLayer"),
        QStringLiteral("KisCloneLayer"),
        QStringLiteral("KisTransparencyMask"),
        QStringLiteral("KisFilterMask"),
        QStringLiteral("FastColorOverlayFilterMask"),
        QStringLiteral("KisColorizeMask"),
        QStringLiteral("KisTransformMask"),
        QStringLiteral("KisSelectionMask"),
        QStringLiteral("KisFileLayer"),
    };
    for (const QString &nodeType : nodeTypes) {
        QTest::newRow(qPrintable(nodeType)) << nodeType;
    }
}

void KisNodeManagerNodeTypeContractTest::creationTypes()
{
    QFETCH(QString, nodeType);
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    selectedNodesValue = KisNodeList{nodeToken(2), nodeToken(3)};
    createdNodeValue = nodeToken(4);

    QCOMPARE(manager.createNode(nodeType, true), createdNodeValue);

    QCOMPARE(creationKinds, (QStringList{nodeType}));
    QCOMPARE(creationActiveNodes, (KisNodeList{activeNodeValue}));
    QCOMPARE(creationCopyDevices, (QList<KisPaintDevice *>{nullptr}));
    QCOMPARE(creationQuietValues, (QList<bool>{true}));
    if (nodeType == QLatin1String("KisCloneLayer")) {
        QCOMPARE(selectedNodesRequests, 1);
        QCOMPARE(creationSelectedNodes, (QList<KisNodeList>{selectedNodesValue}));
    } else {
        QCOMPARE(selectedNodesRequests, 0);
        QCOMPARE(creationSelectedNodes, (QList<KisNodeList>{KisNodeList()}));
    }
}

void KisNodeManagerNodeTypeContractTest::cloneCreationFallsBackToActiveNode()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);

    manager.createNode(QStringLiteral("KisCloneLayer"));

    QCOMPARE(creationSelectedNodes, (QList<KisNodeList>{KisNodeList{activeNodeValue}}));
}

void KisNodeManagerNodeTypeContractTest::unsupportedCreationReturnsNull()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);

    QVERIFY(manager.createNode(QStringLiteral("UnknownNode")).isNull());
    QVERIFY(creationKinds.isEmpty());
}

void KisNodeManagerNodeTypeContractTest::conversionRequiresFinishedOperationsAndModifiableNode()
{
    KisNodeManager manager(nullptr);
    finishPendingOperationsResult = false;

    manager.convertNode(QStringLiteral("KisPaintLayer"));
    QCOMPARE(activeNodeRequests, 0);

    finishPendingOperationsResult = true;
    manager.convertNode(QStringLiteral("KisPaintLayer"));
    QCOMPARE(activeNodeRequests, 1);
    QVERIFY(modificationRequests.isEmpty());

    activeNodeValue = nodeToken(1);
    canModifyResult = false;
    manager.convertNode(QStringLiteral("KisPaintLayer"));
    QCOMPARE(modificationRequests, (KisNodeList{activeNodeValue}));
    QVERIFY(directConversionKinds.isEmpty());
}

void KisNodeManagerNodeTypeContractTest::directConversionTypes_data()
{
    QTest::addColumn<QString>("nodeType");
    QTest::newRow("paint-layer") << QStringLiteral("KisPaintLayer");
    QTest::newRow("file-layer") << QStringLiteral("KisFileLayer");
}

void KisNodeManagerNodeTypeContractTest::directConversionTypes()
{
    QFETCH(QString, nodeType);
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);

    manager.convertNode(nodeType);

    QCOMPARE(directConversionKinds, (QStringList{nodeType}));
    QCOMPARE(directConversionNodes, (KisNodeList{activeNodeValue}));
    QVERIFY(conversionActionNames.isEmpty());
}

void KisNodeManagerNodeTypeContractTest::maskConversionTypes_data()
{
    QTest::addColumn<QString>("nodeType");
    QTest::newRow("selection-mask") << QStringLiteral("KisSelectionMask");
    QTest::newRow("filter-mask") << QStringLiteral("KisFilterMask");
    QTest::newRow("transparency-mask") << QStringLiteral("KisTransparencyMask");
}

void KisNodeManagerNodeTypeContractTest::maskConversionTypes()
{
    QFETCH(QString, nodeType);
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    paintDeviceValue = token<KisPaintDevice>(1);
    projectionValue = token<KisPaintDevice>(2);

    manager.convertNode(nodeType);

    QCOMPARE(paintDeviceRequests, (KisNodeList{activeNodeValue}));
    QVERIFY(projectionRequests.isEmpty());
    QCOMPARE(conversionActionNames, (QStringList{kundo2_i18n("Convert to a Selection Mask").toString()}));
    QCOMPARE(maskConversionKinds, (QStringList{nodeType}));
    QCOMPARE(maskConversionNodes, (KisNodeList{activeNodeValue}));
    QCOMPARE(maskConversionDevices, (QList<KisPaintDevice *>{paintDeviceValue}));
    QCOMPARE(conversionEndRequests, 1);
    QCOMPARE(forcedFinishRequests, 0);
    QCOMPARE(undoRequests, 0);
}

void KisNodeManagerNodeTypeContractTest::failedMaskConversionUsesProjectionAndUndo()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);
    projectionValue = token<KisPaintDevice>(2);
    maskConversionResult = false;

    manager.convertNode(QStringLiteral("KisSelectionMask"));

    QCOMPARE(projectionRequests, (KisNodeList{activeNodeValue}));
    QCOMPARE(maskConversionDevices, (QList<KisPaintDevice *>{projectionValue}));
    QCOMPARE(conversionEndRequests, 1);
    QCOMPARE(forcedFinishRequests, 1);
    QCOMPARE(undoRequests, 1);
}

void KisNodeManagerNodeTypeContractTest::unsupportedConversionIsReported()
{
    KisNodeManager manager(nullptr);
    activeNodeValue = nodeToken(1);

    manager.convertNode(QStringLiteral("UnknownNode"));

    QCOMPARE(unsupportedNodeTypes, (QStringList{QStringLiteral("UnknownNode")}));
    QVERIFY(directConversionKinds.isEmpty());
    QVERIFY(maskConversionKinds.isEmpty());
}

QTEST_GUILESS_MAIN(KisNodeManagerNodeTypeContractTest)

#include "KisNodeManagerNodeTypeContractTest.moc"
