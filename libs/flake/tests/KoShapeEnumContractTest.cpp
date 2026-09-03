/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeContainer.h>
#include <KoShapeFactoryBase.h>

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

namespace
{
using ShapeElementList = QList<QPair<QString, QStringList>>;

#define ASSERT_SHAPE_FACTORY_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeFactoryBase::method)), signature>)

class ShapeFactoryConstructorProbe : public KoShapeFactoryBase
{
protected:
    ShapeFactoryConstructorProbe(const QString &id, const QString &name)
        : KoShapeFactoryBase(id, name)
    {
    }
};

class ShapeConstructorProbe final : public KoShape
{
public:
    using KoShape::KoShape;

    void paint(QPainter &painter) const override;
};

class ShapeContainerConstructorProbe final : public KoShapeContainer
{
public:
    using KoShapeContainer::KoShapeContainer;

    void paintComponent(QPainter &painter) const override;
};

#define ASSERT_SHAPE_CONTAINER_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeContainer::method)), signature>)

#define ASSERT_SHAPE_INTERFACE_SIGNATURE(method, signature)                                                            \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&KoShapeContainer::ShapeInterface::method)), signature>)

#define ASSERT_SHAPE_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShape::method)), signature>)
} // namespace

class KoShapeEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeTypeAndLifetimeSchemaRemainStable();
    void shapeChangeNotificationSignaturesRemainStable();
    void shapeDependencySignaturesRemainStable();
    void shapeParentRelationshipSignaturesRemainStable();
    void shapeTreeOrderingSignaturesRemainStable();
    void shapePaintingSignaturesRemainStable();
    void shapeBackgroundSignaturesRemainStable();
    void shapeStrokeSignaturesRemainStable();
    void shapePaintOrderSignaturesRemainStable();
    void shapeTransparencyAndUpdateSignaturesRemainStable();
    void shapeLocalGeometrySignaturesRemainStable();
    void shapeAffineMutationSignaturesRemainStable();
    void shapeTransformationSignaturesRemainStable();
    void shapeCoordinateMappingSignaturesRemainStable();
    void shapeBoundsAndOutlineSignaturesRemainStable();
    void changeTypeValuesRemainStable();
    void paintOrderValuesRemainStable();
    void childZOrderValuesRemainStable();
    void shapeTemplateValueSchemaRemainsStable();
    void shapeFactoryIdentityAndLifecycleSignaturesRemainStable();
    void shapeFactoryMetadataSignaturesRemainStable();
    void shapeFactoryLoadingAndResourceSignaturesRemainStable();
    void shapeFactoryCreationSignaturesRemainStable();
    void shapeContainerIdentityAndLifecycleSignaturesRemainStable();
    void shapeContainerHierarchySignaturesRemainStable();
    void shapeContainerModelAndInterfaceSignaturesRemainStable();
    void shapeContainerClippingAndTransformSignaturesRemainStable();
    void shapeContainerPaintingAndUpdateSignaturesRemainStable();
};

void KoShapeEnumContractTest::shapeTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoShape>);
    static_assert(std::is_abstract_v<KoShape>);
    static_assert(std::is_constructible_v<ShapeConstructorProbe>);
    static_assert(std::has_virtual_destructor_v<KoShape>);
}

void KoShapeEnumContractTest::shapeChangeNotificationSignaturesRemainStable()
{
    using Listener = KoShape::ShapeChangeListener;

    static_assert(std::is_class_v<Listener>);
    static_assert(std::is_abstract_v<Listener>);
    static_assert(std::has_virtual_destructor_v<Listener>);
    static_assert(std::is_same_v<decltype(static_cast<void (Listener::*)(KoShape::ChangeType, KoShape *)>(
                                     &Listener::notifyShapeChanged)),
                                 void (Listener::*)(KoShape::ChangeType, KoShape *)>);
    ASSERT_SHAPE_SIGNATURE(addShapeChangeListener, void (KoShape::*)(Listener *));
    ASSERT_SHAPE_SIGNATURE(removeShapeChangeListener, void (KoShape::*)(Listener *));
    ASSERT_SHAPE_SIGNATURE(notifyChanged, void (KoShape::*)());
}

void KoShapeEnumContractTest::shapeDependencySignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(addDependee, bool (KoShape::*)(KoShape *));
    ASSERT_SHAPE_SIGNATURE(removeDependee, void (KoShape::*)(KoShape *));
    ASSERT_SHAPE_SIGNATURE(hasDependee, bool (KoShape::*)(KoShape *) const);
    ASSERT_SHAPE_SIGNATURE(dependees, QList<KoShape *> (KoShape::*)() const);
}

void KoShapeEnumContractTest::shapeParentRelationshipSignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(parent, KoShapeContainer * (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(setParent, void (KoShape::*)(KoShapeContainer *));
    ASSERT_SHAPE_SIGNATURE(hasCommonParent, bool (KoShape::*)(const KoShape *) const);
    ASSERT_SHAPE_SIGNATURE(inheritsTransformFromAny, bool (KoShape::*)(QList<KoShape *>) const);
}

void KoShapeEnumContractTest::shapeTreeOrderingSignaturesRemainStable()
{
    using ShapeComparator = bool (*)(KoShape *, KoShape *);
    using ShapeLinearizer = QList<KoShape *> (*)(const QList<KoShape *> &);

    static_assert(std::is_same_v<decltype(KoShape::maxZIndex), const qint16>);
    static_assert(std::is_same_v<decltype(KoShape::minZIndex), const qint16>);
    ASSERT_SHAPE_SIGNATURE(childZOrderPolicy, KoShape::ChildZOrderPolicy (KoShape::*)());
    static_assert(
        std::is_same_v<decltype(static_cast<ShapeComparator>(&KoShape::compareShapeZIndex)), ShapeComparator>);
    static_assert(std::is_same_v<decltype(static_cast<ShapeLinearizer>(&KoShape::linearizeSubtree)), ShapeLinearizer>);
    static_assert(
        std::is_same_v<decltype(static_cast<ShapeLinearizer>(&KoShape::linearizeSubtreeSorted)), ShapeLinearizer>);
    ASSERT_SHAPE_SIGNATURE(setZIndex, void (KoShape::*)(qint16));
    ASSERT_SHAPE_SIGNATURE(zIndex, qint16 (KoShape::*)() const);
}

void KoShapeEnumContractTest::shapePaintingSignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(paint, void (KoShape::*)(QPainter &) const);
    ASSERT_SHAPE_SIGNATURE(paintStroke, void (KoShape::*)(QPainter &) const);
    ASSERT_SHAPE_SIGNATURE(paintMarkers, void (KoShape::*)(QPainter &) const);
}

void KoShapeEnumContractTest::shapeBackgroundSignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(setBackground, void (KoShape::*)(QSharedPointer<KoShapeBackground>));
    ASSERT_SHAPE_SIGNATURE(background, QSharedPointer<KoShapeBackground> (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(setInheritBackground, void (KoShape::*)(bool));
    ASSERT_SHAPE_SIGNATURE(inheritBackground, bool (KoShape::*)() const);
}

void KoShapeEnumContractTest::shapeStrokeSignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(stroke, KoShapeStrokeModelSP (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(setStroke, void (KoShape::*)(KoShapeStrokeModelSP));
    ASSERT_SHAPE_SIGNATURE(setInheritStroke, void (KoShape::*)(bool));
    ASSERT_SHAPE_SIGNATURE(inheritStroke, bool (KoShape::*)() const);
}

void KoShapeEnumContractTest::shapePaintOrderSignaturesRemainStable()
{
    using DefaultPaintOrder = QVector<KoShape::PaintOrder> (*)();

    ASSERT_SHAPE_SIGNATURE(setPaintOrder, void (KoShape::*)(KoShape::PaintOrder, KoShape::PaintOrder));
    ASSERT_SHAPE_SIGNATURE(paintOrder, QVector<KoShape::PaintOrder> (KoShape::*)() const);
    static_assert(
        std::is_same_v<decltype(static_cast<DefaultPaintOrder>(&KoShape::defaultPaintOrder)), DefaultPaintOrder>);
    ASSERT_SHAPE_SIGNATURE(setInheritPaintOrder, void (KoShape::*)(bool));
    ASSERT_SHAPE_SIGNATURE(inheritPaintOrder, bool (KoShape::*)() const);
}

void KoShapeEnumContractTest::shapeTransparencyAndUpdateSignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(hasTransparency, bool (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(setTransparency, void (KoShape::*)(qreal));
    ASSERT_SHAPE_SIGNATURE(transparency, qreal (KoShape::*)(bool) const);
    ASSERT_SHAPE_SIGNATURE(update, void (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(updateAbsolute, void (KoShape::*)(const QRectF &) const);

    static_assert(std::is_same_v<decltype(std::declval<const KoShape &>().transparency()), qreal>);
}

void KoShapeEnumContractTest::shapeLocalGeometrySignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(setSize, void (KoShape::*)(const QSizeF &));
    ASSERT_SHAPE_SIGNATURE(size, QSizeF (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(setPosition, void (KoShape::*)(const QPointF &));
    ASSERT_SHAPE_SIGNATURE(position, QPointF (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(setMinimumHeight, void (KoShape::*)(qreal));
    ASSERT_SHAPE_SIGNATURE(minimumHeight, qreal (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(setKeepAspectRatio, void (KoShape::*)(bool));
    ASSERT_SHAPE_SIGNATURE(keepAspectRatio, bool (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(setResolution, void (KoShape::*)(qreal, qreal));
}

void KoShapeEnumContractTest::shapeAffineMutationSignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(scale, void (KoShape::*)(qreal, qreal));
    ASSERT_SHAPE_SIGNATURE(rotate, void (KoShape::*)(qreal));
    ASSERT_SHAPE_SIGNATURE(rotation, qreal (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(shear, void (KoShape::*)(qreal, qreal));
}

void KoShapeEnumContractTest::shapeTransformationSignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(absolutePosition, QPointF (KoShape::*)(KoFlake::AnchorPosition) const);
    ASSERT_SHAPE_SIGNATURE(setAbsolutePosition, void (KoShape::*)(const QPointF &, KoFlake::AnchorPosition));
    ASSERT_SHAPE_SIGNATURE(absoluteTransformation, QTransform (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(applyAbsoluteTransformation, void (KoShape::*)(const QTransform &));
    ASSERT_SHAPE_SIGNATURE(setTransformation, void (KoShape::*)(const QTransform &));
    ASSERT_SHAPE_SIGNATURE(transformation, QTransform (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(applyTransformation, void (KoShape::*)(const QTransform &));

    static_assert(std::is_same_v<decltype(std::declval<const KoShape &>().absolutePosition()), QPointF>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoShape &>().setAbsolutePosition(std::declval<const QPointF &>())), void>);
}

void KoShapeEnumContractTest::shapeCoordinateMappingSignaturesRemainStable()
{
    ASSERT_SHAPE_SIGNATURE(shapeToDocument, QPointF (KoShape::*)(const QPointF &) const);
    ASSERT_SHAPE_SIGNATURE(shapeToDocument, QRectF (KoShape::*)(const QRectF &) const);
    ASSERT_SHAPE_SIGNATURE(documentToShape, QPointF (KoShape::*)(const QPointF &) const);
    ASSERT_SHAPE_SIGNATURE(documentToShape, QRectF (KoShape::*)(const QRectF &) const);
}

void KoShapeEnumContractTest::shapeBoundsAndOutlineSignaturesRemainStable()
{
    using ShapesRect = QRectF (*)(const QList<KoShape *> &);

    ASSERT_SHAPE_SIGNATURE(hitTest, bool (KoShape::*)(const QPointF &) const);
    ASSERT_SHAPE_SIGNATURE(boundingRect, QRectF (KoShape::*)() const);
    static_assert(std::is_same_v<decltype(static_cast<ShapesRect>(&KoShape::boundingRect)), ShapesRect>);
    ASSERT_SHAPE_SIGNATURE(absoluteOutlineRect, QRectF (KoShape::*)() const);
    static_assert(std::is_same_v<decltype(static_cast<ShapesRect>(&KoShape::absoluteOutlineRect)), ShapesRect>);
    ASSERT_SHAPE_SIGNATURE(outline, QPainterPath (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(outlineRect, QRectF (KoShape::*)() const);
    ASSERT_SHAPE_SIGNATURE(strokeInsets, KoInsets (KoShape::*)() const);
}

void KoShapeEnumContractTest::changeTypeValuesRemainStable()
{
    const std::array<KoShape::ChangeType, 20> changes{{
        KoShape::PositionChanged,
        KoShape::RotationChanged,
        KoShape::ScaleChanged,
        KoShape::ShearChanged,
        KoShape::SizeChanged,
        KoShape::GenericMatrixChange,
        KoShape::KeepAspectRatioChange,
        KoShape::ParentChanged,
        KoShape::Deleted,
        KoShape::StrokeChanged,
        KoShape::BackgroundChanged,
        KoShape::BorderChanged,
        KoShape::ParameterChanged,
        KoShape::ContentChanged,
        KoShape::TextContourMarginChanged,
        KoShape::ChildChanged,
        KoShape::ConnectionPointChanged,
        KoShape::ClipPathChanged,
        KoShape::ClipMaskChanged,
        KoShape::TransparencyChanged,
    }};

    for (std::size_t index = 0; index < changes.size(); ++index) {
        QCOMPARE(int(changes[index]), int(index));
    }
}

void KoShapeEnumContractTest::paintOrderValuesRemainStable()
{
    QCOMPARE(int(KoShape::Fill), 0);
    QCOMPARE(int(KoShape::Stroke), 1);
    QCOMPARE(int(KoShape::Markers), 2);
}

void KoShapeEnumContractTest::childZOrderValuesRemainStable()
{
    QCOMPARE(int(KoShape::ChildZDefault), 0);
    QCOMPARE(int(KoShape::ChildZParentChild), int(KoShape::ChildZDefault));
    QCOMPARE(int(KoShape::ChildZPassThrough), 1);
}

void KoShapeEnumContractTest::shapeTemplateValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(KoShapeTemplate::id), QString>);
    static_assert(std::is_same_v<decltype(KoShapeTemplate::templateId), QString>);
    static_assert(std::is_same_v<decltype(KoShapeTemplate::name), QString>);
    static_assert(std::is_same_v<decltype(KoShapeTemplate::family), QString>);
    static_assert(std::is_same_v<decltype(KoShapeTemplate::toolTip), QString>);
    static_assert(std::is_same_v<decltype(KoShapeTemplate::iconName), QString>);
    static_assert(std::is_same_v<decltype(KoShapeTemplate::properties), const KoProperties *>);

    KoShapeTemplate value;
    QVERIFY(value.id.isEmpty());
    QVERIFY(value.templateId.isEmpty());
    QVERIFY(value.name.isEmpty());
    QVERIFY(value.family.isEmpty());
    QVERIFY(value.toolTip.isEmpty());
    QVERIFY(value.iconName.isEmpty());
    QCOMPARE(value.properties, nullptr);

    value.name = QStringLiteral("source");
    const auto *propertiesMarker = reinterpret_cast<const KoProperties *>(&value);
    value.properties = propertiesMarker;
    KoShapeTemplate copy = value;
    value.name = QStringLiteral("changed");
    QCOMPARE(copy.name, QStringLiteral("source"));
    QCOMPARE(copy.properties, propertiesMarker);
}

void KoShapeEnumContractTest::shapeFactoryIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoShapeFactoryBase>);
    static_assert(std::is_abstract_v<KoShapeFactoryBase>);
    static_assert(std::is_base_of_v<QObject, KoShapeFactoryBase>);
    static_assert(std::has_virtual_destructor_v<KoShapeFactoryBase>);
    static_assert(std::is_abstract_v<ShapeFactoryConstructorProbe>);
}

void KoShapeEnumContractTest::shapeFactoryMetadataSignaturesRemainStable()
{
    ASSERT_SHAPE_FACTORY_SIGNATURE(family, QString (KoShapeFactoryBase::*)() const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(hidden, bool (KoShapeFactoryBase::*)() const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(iconName, QString (KoShapeFactoryBase::*)() const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(id, QString (KoShapeFactoryBase::*)() const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(loadingPriority, int (KoShapeFactoryBase::*)() const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(name, QString (KoShapeFactoryBase::*)() const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(templates, QList<KoShapeTemplate> (KoShapeFactoryBase::*)() const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(toolTip, QString (KoShapeFactoryBase::*)() const);
}

void KoShapeEnumContractTest::shapeFactoryLoadingAndResourceSignaturesRemainStable()
{
    ASSERT_SHAPE_FACTORY_SIGNATURE(newDocumentResourceManager,
                                   void (KoShapeFactoryBase::*)(KoDocumentResourceManager *) const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(odfElements, ShapeElementList (KoShapeFactoryBase::*)() const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(supports,
                                   bool (KoShapeFactoryBase::*)(const QDomElement &, KoShapeLoadingContext &) const);
}

void KoShapeEnumContractTest::shapeFactoryCreationSignaturesRemainStable()
{
    ASSERT_SHAPE_FACTORY_SIGNATURE(createDefaultShape,
                                   KoShape * (KoShapeFactoryBase::*)(KoDocumentResourceManager *) const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(
        createShape,
        KoShape * (KoShapeFactoryBase::*)(const KoProperties *, KoDocumentResourceManager *) const);
    ASSERT_SHAPE_FACTORY_SIGNATURE(createShapeOptionPanels, QList<KoShapeConfigWidgetBase *> (KoShapeFactoryBase::*)());
}

void KoShapeEnumContractTest::shapeContainerIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoShapeContainer>);
    static_assert(std::is_base_of_v<KoShape, KoShapeContainer>);
    static_assert(std::is_abstract_v<KoShapeContainer>);
    static_assert(std::has_virtual_destructor_v<KoShapeContainer>);
    static_assert(std::is_constructible_v<ShapeContainerConstructorProbe>);
    static_assert(std::is_constructible_v<ShapeContainerConstructorProbe, KoShapeContainerModel *>);
    static_assert(std::is_class_v<KoShapeContainer::ShapeInterface>);
    static_assert(std::is_constructible_v<KoShapeContainer::ShapeInterface, KoShapeContainer *>);
}

void KoShapeEnumContractTest::shapeContainerHierarchySignaturesRemainStable()
{
    ASSERT_SHAPE_CONTAINER_SIGNATURE(addShape, void (KoShapeContainer::*)(KoShape *));
    ASSERT_SHAPE_CONTAINER_SIGNATURE(removeShape, void (KoShapeContainer::*)(KoShape *));
    ASSERT_SHAPE_CONTAINER_SIGNATURE(shapeCount, int (KoShapeContainer::*)() const);
    ASSERT_SHAPE_CONTAINER_SIGNATURE(shapes, QList<KoShape *> (KoShapeContainer::*)() const);
    ASSERT_SHAPE_INTERFACE_SIGNATURE(addShape, void (KoShapeContainer::ShapeInterface::*)(KoShape *));
    ASSERT_SHAPE_INTERFACE_SIGNATURE(removeShape, void (KoShapeContainer::ShapeInterface::*)(KoShape *));
}

void KoShapeEnumContractTest::shapeContainerModelAndInterfaceSignaturesRemainStable()
{
    ASSERT_SHAPE_CONTAINER_SIGNATURE(model, KoShapeContainerModel * (KoShapeContainer::*)() const);
    ASSERT_SHAPE_CONTAINER_SIGNATURE(shapeInterface, KoShapeContainer::ShapeInterface * (KoShapeContainer::*)());
}

void KoShapeEnumContractTest::shapeContainerClippingAndTransformSignaturesRemainStable()
{
    ASSERT_SHAPE_CONTAINER_SIGNATURE(setClipped, void (KoShapeContainer::*)(const KoShape *, bool));
    ASSERT_SHAPE_CONTAINER_SIGNATURE(isClipped, bool (KoShapeContainer::*)(const KoShape *) const);
    ASSERT_SHAPE_CONTAINER_SIGNATURE(setInheritsTransform, void (KoShapeContainer::*)(const KoShape *, bool));
    ASSERT_SHAPE_CONTAINER_SIGNATURE(inheritsTransform, bool (KoShapeContainer::*)(const KoShape *) const);
}

void KoShapeEnumContractTest::shapeContainerPaintingAndUpdateSignaturesRemainStable()
{
    ASSERT_SHAPE_CONTAINER_SIGNATURE(paint, void (KoShapeContainer::*)(QPainter &) const);
    ASSERT_SHAPE_CONTAINER_SIGNATURE(paintComponent, void (KoShapeContainer::*)(QPainter &) const);
    ASSERT_SHAPE_CONTAINER_SIGNATURE(update, void (KoShapeContainer::*)() const);
}

QTEST_GUILESS_MAIN(KoShapeEnumContractTest)

#include "KoShapeEnumContractTest.moc"
