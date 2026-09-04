/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Node.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_NODE_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Node::method)), signature>)
} // namespace

class NodeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeTypeLifetimeAndHierarchySchemaRemainStable();
    void nodeColorAnimationAndPresentationStateSignaturesRemainStable();
    void nodePixelGeometryAndTransformationSignaturesRemainStable();
    void nodeIdentityEditingAndPersistenceSignaturesRemainStable();
    void nodePaintingSignaturesRemainStable();
};

void NodeSchemaContractTest::nodeTypeLifetimeAndHierarchySchemaRemainStable()
{
    using CreateNodeSignature = Node *(*)(KisImageSP, KisNodeSP, QObject *);

    static_assert(std::is_same_v<NodeSP, QSharedPointer<Node>>);
    static_assert(std::is_class_v<Node>);
    static_assert(std::is_base_of_v<QObject, Node>);
    static_assert(std::has_virtual_destructor_v<Node>);
    static_assert(std::is_same_v<decltype(static_cast<CreateNodeSignature>(&Node::createNode)), CreateNodeSignature>);
    ASSERT_NODE_SIGNATURE(addChildNode, bool (Node::*)(Node *, Node *));
    ASSERT_NODE_SIGNATURE(channels, QList<Channel *> (Node::*)() const);
    ASSERT_NODE_SIGNATURE(childNodes, QList<Node *> (Node::*)() const);
    ASSERT_NODE_SIGNATURE(clone, Node * (Node::*)() const);
    ASSERT_NODE_SIGNATURE(findChildNodes,
                          QList<Node *> (Node::*)(const QString &, bool, bool, const QString &, int) const);
    ASSERT_NODE_SIGNATURE(mergeDown, Node * (Node::*)());
    ASSERT_NODE_SIGNATURE(parentNode, Node * (Node::*)() const);
    ASSERT_NODE_SIGNATURE(removeChildNode, bool (Node::*)(Node *));
    ASSERT_NODE_SIGNATURE(setChildNodes, void (Node::*)(QList<Node *>));

    using CreateNodeWithDefaultParent =
        decltype(Node::createNode(std::declval<KisImageSP>(), std::declval<KisNodeSP>()));
    using FindChildrenWithDefaults = decltype(std::declval<const Node &>().findChildNodes());
    static_assert(std::is_same_v<CreateNodeWithDefaultParent, Node *>);
    static_assert(std::is_same_v<FindChildrenWithDefaults, QList<Node *>>);
}

void NodeSchemaContractTest::nodeColorAnimationAndPresentationStateSignaturesRemainStable()
{
    ASSERT_NODE_SIGNATURE(alphaLocked, bool (Node::*)() const);
    ASSERT_NODE_SIGNATURE(animated, bool (Node::*)() const);
    ASSERT_NODE_SIGNATURE(blendingMode, QString (Node::*)() const);
    ASSERT_NODE_SIGNATURE(collapsed, bool (Node::*)() const);
    ASSERT_NODE_SIGNATURE(colorDepth, QString (Node::*)() const);
    ASSERT_NODE_SIGNATURE(colorLabel, int (Node::*)() const);
    ASSERT_NODE_SIGNATURE(colorModel, QString (Node::*)() const);
    ASSERT_NODE_SIGNATURE(colorProfile, QString (Node::*)() const);
    ASSERT_NODE_SIGNATURE(enableAnimation, void (Node::*)() const);
    ASSERT_NODE_SIGNATURE(inheritAlpha, bool (Node::*)() const);
    ASSERT_NODE_SIGNATURE(isPinnedToTimeline, bool (Node::*)() const);
    ASSERT_NODE_SIGNATURE(locked, bool (Node::*)() const);
    ASSERT_NODE_SIGNATURE(opacity, int (Node::*)() const);
    ASSERT_NODE_SIGNATURE(setAlphaLocked, void (Node::*)(bool));
    ASSERT_NODE_SIGNATURE(setBlendingMode, void (Node::*)(QString));
    ASSERT_NODE_SIGNATURE(setCollapsed, void (Node::*)(bool));
    ASSERT_NODE_SIGNATURE(setColorLabel, void (Node::*)(int));
    ASSERT_NODE_SIGNATURE(setColorProfile, bool (Node::*)(const QString &));
    ASSERT_NODE_SIGNATURE(setColorSpace, bool (Node::*)(const QString &, const QString &, const QString &));
    ASSERT_NODE_SIGNATURE(setInheritAlpha, void (Node::*)(bool));
    ASSERT_NODE_SIGNATURE(setLocked, void (Node::*)(bool));
    ASSERT_NODE_SIGNATURE(setOpacity, void (Node::*)(int));
    ASSERT_NODE_SIGNATURE(setPinnedToTimeline, void (Node::*)(bool) const);
    ASSERT_NODE_SIGNATURE(setVisible, void (Node::*)(bool));
    ASSERT_NODE_SIGNATURE(visible, bool (Node::*)() const);
}

void NodeSchemaContractTest::nodePixelGeometryAndTransformationSignaturesRemainStable()
{
    ASSERT_NODE_SIGNATURE(bounds, QRect (Node::*)() const);
    ASSERT_NODE_SIGNATURE(cropNode, void (Node::*)(int, int, int, int));
    ASSERT_NODE_SIGNATURE(hasExtents, bool (Node::*)());
    ASSERT_NODE_SIGNATURE(hasKeyframeAtTime, bool (Node::*)(int));
    ASSERT_NODE_SIGNATURE(move, void (Node::*)(int, int));
    ASSERT_NODE_SIGNATURE(pixelData, QByteArray (Node::*)(int, int, int, int) const);
    ASSERT_NODE_SIGNATURE(pixelDataAtTime, QByteArray (Node::*)(int, int, int, int, int) const);
    ASSERT_NODE_SIGNATURE(position, QPoint (Node::*)() const);
    ASSERT_NODE_SIGNATURE(projectionPixelData, QByteArray (Node::*)(int, int, int, int) const);
    ASSERT_NODE_SIGNATURE(rotateNode, void (Node::*)(double));
    ASSERT_NODE_SIGNATURE(scaleNode, void (Node::*)(QPointF, int, int, QString));
    ASSERT_NODE_SIGNATURE(setPixelData, bool (Node::*)(QByteArray, int, int, int, int));
    ASSERT_NODE_SIGNATURE(shearNode, void (Node::*)(double, double));
    ASSERT_NODE_SIGNATURE(thumbnail, QImage (Node::*)(int, int));
}

void NodeSchemaContractTest::nodeIdentityEditingAndPersistenceSignaturesRemainStable()
{
    ASSERT_NODE_SIGNATURE(duplicate, Node * (Node::*)());
    ASSERT_NODE_SIGNATURE(icon, QIcon (Node::*)() const);
    ASSERT_NODE_SIGNATURE(index, int (Node::*)() const);
    ASSERT_NODE_SIGNATURE(layerStyleToAsl, QString (Node::*)());
    ASSERT_NODE_SIGNATURE(name, QString (Node::*)() const);
    ASSERT_NODE_SIGNATURE(operator!=, bool (Node::*)(const Node &) const);
    ASSERT_NODE_SIGNATURE(operator==, bool (Node::*)(const Node &) const);
    ASSERT_NODE_SIGNATURE(paintAbility, QString (Node::*)());
    ASSERT_NODE_SIGNATURE(remove, bool (Node::*)());
    ASSERT_NODE_SIGNATURE(save, bool (Node::*)(const QString &, double, double, const InfoObject &, const QRect &));
    ASSERT_NODE_SIGNATURE(setLayerStyleFromAsl, bool (Node::*)(const QString &));
    ASSERT_NODE_SIGNATURE(setName, void (Node::*)(QString));
    ASSERT_NODE_SIGNATURE(type, QString (Node::*)() const);
    ASSERT_NODE_SIGNATURE(uniqueId, QUuid (Node::*)() const);

    using SaveWithDefaultExportRect = decltype(std::declval<Node &>().save(std::declval<const QString &>(),
                                                                           std::declval<double>(),
                                                                           std::declval<double>(),
                                                                           std::declval<const InfoObject &>()));
    static_assert(std::is_same_v<SaveWithDefaultExportRect, bool>);
}

void NodeSchemaContractTest::nodePaintingSignaturesRemainStable()
{
    ASSERT_NODE_SIGNATURE(paintEllipse, void (Node::*)(const QRectF &, QString, QString));
    ASSERT_NODE_SIGNATURE(paintLine, void (Node::*)(QPointF, QPointF, double, double, QString));
    ASSERT_NODE_SIGNATURE(paintPath, void (Node::*)(const QPainterPath &, QString, QString));
    ASSERT_NODE_SIGNATURE(paintPolygon, void (Node::*)(QList<QPointF>, QString, QString));
    ASSERT_NODE_SIGNATURE(paintRectangle, void (Node::*)(const QRectF &, QString, QString));

    using PaintEllipseWithDefaults = decltype(std::declval<Node &>().paintEllipse(std::declval<const QRectF &>()));
    using PaintLineWithDefaults =
        decltype(std::declval<Node &>().paintLine(std::declval<QPointF>(), std::declval<QPointF>()));
    using PaintPathWithDefaults = decltype(std::declval<Node &>().paintPath(std::declval<const QPainterPath &>()));
    using PaintPolygonWithDefaults = decltype(std::declval<Node &>().paintPolygon(std::declval<QList<QPointF>>()));
    using PaintRectangleWithDefaults = decltype(std::declval<Node &>().paintRectangle(std::declval<const QRectF &>()));

    static_assert(std::is_same_v<PaintEllipseWithDefaults, void>);
    static_assert(std::is_same_v<PaintLineWithDefaults, void>);
    static_assert(std::is_same_v<PaintPathWithDefaults, void>);
    static_assert(std::is_same_v<PaintPolygonWithDefaults, void>);
    static_assert(std::is_same_v<PaintRectangleWithDefaults, void>);
}

QTEST_APPLESS_MAIN(NodeSchemaContractTest)

#include "NodeSchemaContractTest.moc"
