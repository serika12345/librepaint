/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <GroupShape.h>
#include <VectorLayer.h>

#include <QTest>

#include <type_traits>
#include <utility>

class VectorGroupShapeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void vectorLayerTypeLifetimeAndConstructionSchemaRemainStable();
    void vectorLayerIdentityAndRenderingPolicySignaturesRemainStable();
    void vectorLayerShapeQuerySignaturesRemainStable();
    void vectorLayerSvgAndGroupingSignaturesRemainStable();
    void groupShapeTypeLifetimeAndHierarchySignaturesRemainStable();
};

void VectorGroupShapeSchemaContractTest::vectorLayerTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<VectorLayer>);
    static_assert(std::is_base_of_v<Node, VectorLayer>);
    static_assert(std::is_constructible_v<VectorLayer, KisShapeLayerSP, QObject *>);
    static_assert(std::is_constructible_v<VectorLayer, KoShapeControllerBase *, KisImageSP, QString, QObject *>);
    static_assert(std::has_virtual_destructor_v<VectorLayer>);
    static_assert(!std::is_copy_constructible_v<VectorLayer>);

    using WrappedLayerWithDefaultParent = decltype(VectorLayer(std::declval<KisShapeLayerSP>()));
    using NewLayerWithDefaultParent = decltype(VectorLayer(std::declval<KoShapeControllerBase *>(),
                                                           std::declval<KisImageSP>(),
                                                           std::declval<QString>()));
    static_assert(std::is_same_v<WrappedLayerWithDefaultParent, VectorLayer>);
    static_assert(std::is_same_v<NewLayerWithDefaultParent, VectorLayer>);
}

void VectorGroupShapeSchemaContractTest::vectorLayerIdentityAndRenderingPolicySignaturesRemainStable()
{
    using TypeSignature = QString (VectorLayer::*)() const;
    using IsAntialiasedSignature = bool (VectorLayer::*)() const;
    using SetAntialiasedSignature = void (VectorLayer::*)(const bool);

    static_assert(std::is_same_v<decltype(static_cast<TypeSignature>(&VectorLayer::type)), TypeSignature>);
    static_assert(std::is_same_v<decltype(&VectorLayer::isAntialiased), IsAntialiasedSignature>);
    static_assert(std::is_same_v<decltype(&VectorLayer::setAntialiased), SetAntialiasedSignature>);
}

void VectorGroupShapeSchemaContractTest::vectorLayerShapeQuerySignaturesRemainStable()
{
    using ShapesSignature = QList<Shape *> (VectorLayer::*)() const;
    using ShapeAtPositionSignature = Shape *(VectorLayer::*)(const QPointF &) const;
    using ShapesInRectSignature = QList<Shape *> (VectorLayer::*)(const QRectF &, bool, bool) const;

    static_assert(std::is_same_v<decltype(&VectorLayer::shapes), ShapesSignature>);
    static_assert(std::is_same_v<decltype(&VectorLayer::shapeAtPosition), ShapeAtPositionSignature>);
    static_assert(std::is_same_v<decltype(&VectorLayer::shapesInRect), ShapesInRectSignature>);
    static_assert(
        std::is_same_v<decltype(std::declval<const VectorLayer &>().shapesInRect(std::declval<const QRectF &>())),
                       QList<Shape *>>);
}

void VectorGroupShapeSchemaContractTest::vectorLayerSvgAndGroupingSignaturesRemainStable()
{
    using AddShapesSignature = QList<Shape *> (VectorLayer::*)(const QString &);
    using CreateGroupSignature = Shape *(VectorLayer::*)(const QString &, QList<Shape *>) const;
    using ToSvgSignature = QString (VectorLayer::*)();

    static_assert(std::is_same_v<decltype(&VectorLayer::addShapesFromSvg), AddShapesSignature>);
    static_assert(std::is_same_v<decltype(&VectorLayer::createGroupShape), CreateGroupSignature>);
    static_assert(std::is_same_v<decltype(&VectorLayer::toSvg), ToSvgSignature>);
}

void VectorGroupShapeSchemaContractTest::groupShapeTypeLifetimeAndHierarchySignaturesRemainStable()
{
    using TypeSignature = QString (GroupShape::*)() const;
    using ChildrenSignature = QList<Shape *> (GroupShape::*)();

    static_assert(std::is_class_v<GroupShape>);
    static_assert(std::is_base_of_v<Shape, GroupShape>);
    static_assert(std::is_constructible_v<GroupShape, QObject *>);
    static_assert(std::is_constructible_v<GroupShape, KoShapeGroup *, QObject *>);
    static_assert(std::has_virtual_destructor_v<GroupShape>);
    static_assert(!std::is_copy_constructible_v<GroupShape>);
    static_assert(std::is_same_v<decltype(static_cast<TypeSignature>(&GroupShape::type)), TypeSignature>);
    static_assert(std::is_same_v<decltype(&GroupShape::children), ChildrenSignature>);

    using DefaultGroup = decltype(GroupShape());
    using WrappedGroupWithDefaultParent = decltype(GroupShape(std::declval<KoShapeGroup *>()));
    static_assert(std::is_same_v<DefaultGroup, GroupShape>);
    static_assert(std::is_same_v<WrappedGroupWithDefaultParent, GroupShape>);
}

QTEST_APPLESS_MAIN(VectorGroupShapeSchemaContractTest)

#include "VectorGroupShapeSchemaContractTest.moc"
