/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Selection.h"
#include "Shape.h"

#include <QTest>

#include <type_traits>
#include <utility>

class SelectionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void selectionWrapperTypeLifetimeAndIdentitySchemaRemainsStable();
    void selectionWrapperGeometryAndPixelDataSchemaRemainsStable();
    void selectionWrapperContentAndClipboardSchemaRemainsStable();
    void selectionWrapperMorphologySchemaRemainsStable();
    void selectionWrapperBooleanCombinationSchemaRemainsStable();
    void shapeWrapperTypeLifetimeAndIdentitySchemaRemainsStable();
    void shapeWrapperMetadataAndPolicySchemaRemainsStable();
    void shapeWrapperGeometryAndVisibilitySchemaRemainsStable();
    void shapeWrapperLifecycleAndSelectionSchemaRemainsStable();
    void shapeWrapperHierarchyAndSerializationSchemaRemainsStable();
};

void SelectionSchemaContractTest::selectionWrapperTypeLifetimeAndIdentitySchemaRemainsStable()
{
    using ComparisonSignature = bool (Selection::*)(const Selection &) const;

    static_assert(std::is_class_v<Selection>);
    static_assert(std::is_default_constructible_v<Selection>);
    static_assert(std::is_constructible_v<Selection, QObject *>);
    static_assert(std::is_constructible_v<Selection, KisSelectionSP>);
    static_assert(std::is_constructible_v<Selection, KisSelectionSP, QObject *>);
    static_assert(std::is_destructible_v<Selection>);
    static_assert(std::is_same_v<decltype(&Selection::operator==), ComparisonSignature>);
    static_assert(std::is_same_v<decltype(&Selection::operator!=), ComparisonSignature>);
}

void SelectionSchemaContractTest::selectionWrapperGeometryAndPixelDataSchemaRemainsStable()
{
    using ConstIntegerSignature = int (Selection::*)() const;

    static_assert(std::is_same_v<decltype(&Selection::duplicate), Selection *(Selection::*)() const>);
    static_assert(std::is_same_v<decltype(&Selection::width), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&Selection::height), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&Selection::x), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&Selection::y), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&Selection::move), void (Selection::*)(int, int)>);
    static_assert(std::is_same_v<decltype(&Selection::resize), void (Selection::*)(int, int)>);
    static_assert(std::is_same_v<decltype(&Selection::pixelData), QByteArray (Selection::*)(int, int, int, int) const>);
    static_assert(
        std::is_same_v<decltype(&Selection::setPixelData), void (Selection::*)(QByteArray, int, int, int, int)>);
}

void SelectionSchemaContractTest::selectionWrapperContentAndClipboardSchemaRemainsStable()
{
    using VoidSignature = void (Selection::*)();
    using NodeSignature = void (Selection::*)(Node *);

    static_assert(std::is_same_v<decltype(&Selection::clear), VoidSignature>);
    static_assert(std::is_same_v<decltype(&Selection::select), void (Selection::*)(int, int, int, int, int)>);
    static_assert(std::is_same_v<decltype(&Selection::selectAll), void (Selection::*)(Node *, int)>);
    static_assert(std::is_same_v<decltype(&Selection::copy), NodeSignature>);
    static_assert(std::is_same_v<decltype(&Selection::cut), NodeSignature>);
    static_assert(std::is_same_v<decltype(&Selection::paste), void (Selection::*)(Node *, int, int)>);
    static_assert(std::is_same_v<decltype(&Selection::invert), VoidSignature>);
}

void SelectionSchemaContractTest::selectionWrapperMorphologySchemaRemainsStable()
{
    using VoidSignature = void (Selection::*)();

    static_assert(std::is_same_v<decltype(&Selection::contract), void (Selection::*)(int)>);
    static_assert(std::is_same_v<decltype(&Selection::erode), VoidSignature>);
    static_assert(std::is_same_v<decltype(&Selection::dilate), VoidSignature>);
    static_assert(std::is_same_v<decltype(&Selection::border), void (Selection::*)(int, int)>);
    static_assert(std::is_same_v<decltype(&Selection::feather), void (Selection::*)(int)>);
    static_assert(std::is_same_v<decltype(&Selection::grow), void (Selection::*)(int, int)>);
    static_assert(std::is_same_v<decltype(&Selection::shrink), void (Selection::*)(int, int, bool)>);
    static_assert(std::is_same_v<decltype(&Selection::smooth), VoidSignature>);
}

void SelectionSchemaContractTest::selectionWrapperBooleanCombinationSchemaRemainsStable()
{
    using CombinationSignature = void (Selection::*)(Selection *);

    static_assert(std::is_same_v<decltype(&Selection::replace), CombinationSignature>);
    static_assert(std::is_same_v<decltype(&Selection::add), CombinationSignature>);
    static_assert(std::is_same_v<decltype(&Selection::subtract), CombinationSignature>);
    static_assert(std::is_same_v<decltype(&Selection::intersect), CombinationSignature>);
    static_assert(std::is_same_v<decltype(&Selection::symmetricdifference), CombinationSignature>);
}

void SelectionSchemaContractTest::shapeWrapperTypeLifetimeAndIdentitySchemaRemainsStable()
{
    using Comparison = bool (Shape::*)(const Shape &) const;
    static_assert(std::is_class_v<Shape>);
    static_assert(std::is_constructible_v<Shape, KoShape *, QObject *>);
    static_assert(std::is_constructible_v<Shape, KoShape *>);
    static_assert(std::is_destructible_v<Shape>);
    static_assert(std::is_same_v<decltype(&Shape::operator==), Comparison>);
    static_assert(std::is_same_v<decltype(&Shape::operator!=), Comparison>);
}

void SelectionSchemaContractTest::shapeWrapperMetadataAndPolicySchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&Shape::name), QString (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::setName), void (Shape::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Shape::type), QString (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::zIndex), int (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::setZIndex), void (Shape::*)(int)>);
    static_assert(std::is_same_v<decltype(&Shape::selectable), bool (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::setSelectable), void (Shape::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Shape::geometryProtected), bool (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::setGeometryProtected), void (Shape::*)(bool)>);
}

void SelectionSchemaContractTest::shapeWrapperGeometryAndVisibilitySchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&Shape::visible), bool (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::setVisible), void (Shape::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Shape::boundingBox), QRectF (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::position), QPointF (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::setPosition), void (Shape::*)(QPointF)>);
    static_assert(std::is_same_v<decltype(&Shape::transformation), QTransform (Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::setTransformation), void (Shape::*)(QTransform)>);
    static_assert(std::is_same_v<decltype(&Shape::absoluteTransformation), QTransform (Shape::*)() const>);
}

void SelectionSchemaContractTest::shapeWrapperLifecycleAndSelectionSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&Shape::remove), bool (Shape::*)()>);
    static_assert(std::is_same_v<decltype(&Shape::update), void (Shape::*)()>);
    static_assert(std::is_same_v<decltype(&Shape::updateAbsolute), void (Shape::*)(QRectF)>);
    static_assert(std::is_same_v<decltype(&Shape::select), void (Shape::*)()>);
    static_assert(std::is_same_v<decltype(&Shape::deselect), void (Shape::*)()>);
    static_assert(std::is_same_v<decltype(&Shape::isSelected), bool (Shape::*)()>);
}

void SelectionSchemaContractTest::shapeWrapperHierarchyAndSerializationSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&Shape::parentShape), Shape *(Shape::*)() const>);
    static_assert(std::is_same_v<decltype(&Shape::toSvg), QString (Shape::*)(bool, bool)>);
    static_assert(std::is_same_v<decltype(std::declval<Shape &>().toSvg()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<Shape &>().toSvg(true)), QString>);
}

QTEST_GUILESS_MAIN(SelectionSchemaContractTest)

#include "SelectionSchemaContractTest.moc"
