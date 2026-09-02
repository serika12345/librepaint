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
} // namespace

class KoShapeEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
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
