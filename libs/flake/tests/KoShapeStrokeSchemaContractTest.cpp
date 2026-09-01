/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeStroke.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_SHAPE_STROKE_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeStroke::method)), signature>)
} // namespace

class KoShapeStrokeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeStrokeTypeAndLifetimeSchemaRemainStable();
    void shapeStrokeLineGeometrySignaturesRemainStable();
    void shapeStrokeDashPatternSignaturesRemainStable();
    void shapeStrokeColorBrushAndPenSignaturesRemainStable();
    void shapeStrokeRenderingAndComparisonSignaturesRemainStable();
};

void KoShapeStrokeSchemaContractTest::shapeStrokeTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoShapeStroke>);
    static_assert(std::is_base_of_v<KoShapeStrokeModel, KoShapeStroke>);
    static_assert(!std::is_abstract_v<KoShapeStroke>);
    static_assert(std::is_default_constructible_v<KoShapeStroke>);
    static_assert(std::is_copy_constructible_v<KoShapeStroke>);
    static_assert(std::is_constructible_v<KoShapeStroke, qreal>);
    static_assert(std::is_constructible_v<KoShapeStroke, qreal, const QColor &>);
    static_assert(!std::is_convertible_v<qreal, KoShapeStroke>);
    static_assert(std::is_copy_assignable_v<KoShapeStroke>);
    static_assert(std::is_destructible_v<KoShapeStroke>);
    static_assert(std::has_virtual_destructor_v<KoShapeStroke>);

    ASSERT_SHAPE_STROKE_SIGNATURE(operator=, KoShapeStroke & (KoShapeStroke::*)(const KoShapeStroke &));
}

void KoShapeStrokeSchemaContractTest::shapeStrokeLineGeometrySignaturesRemainStable()
{
    ASSERT_SHAPE_STROKE_SIGNATURE(setCapStyle, void (KoShapeStroke::*)(Qt::PenCapStyle));
    ASSERT_SHAPE_STROKE_SIGNATURE(capStyle, Qt::PenCapStyle (KoShapeStroke::*)() const);
    ASSERT_SHAPE_STROKE_SIGNATURE(setJoinStyle, void (KoShapeStroke::*)(Qt::PenJoinStyle));
    ASSERT_SHAPE_STROKE_SIGNATURE(joinStyle, Qt::PenJoinStyle (KoShapeStroke::*)() const);
    ASSERT_SHAPE_STROKE_SIGNATURE(setLineWidth, void (KoShapeStroke::*)(qreal));
    ASSERT_SHAPE_STROKE_SIGNATURE(lineWidth, qreal (KoShapeStroke::*)() const);
    ASSERT_SHAPE_STROKE_SIGNATURE(setMiterLimit, void (KoShapeStroke::*)(qreal));
    ASSERT_SHAPE_STROKE_SIGNATURE(miterLimit, qreal (KoShapeStroke::*)() const);
}

void KoShapeStrokeSchemaContractTest::shapeStrokeDashPatternSignaturesRemainStable()
{
    ASSERT_SHAPE_STROKE_SIGNATURE(setLineStyle, void (KoShapeStroke::*)(Qt::PenStyle, const QVector<qreal> &));
    ASSERT_SHAPE_STROKE_SIGNATURE(lineStyle, Qt::PenStyle (KoShapeStroke::*)() const);
    ASSERT_SHAPE_STROKE_SIGNATURE(lineDashes, QVector<qreal> (KoShapeStroke::*)() const);
    ASSERT_SHAPE_STROKE_SIGNATURE(setDashOffset, void (KoShapeStroke::*)(qreal));
    ASSERT_SHAPE_STROKE_SIGNATURE(dashOffset, qreal (KoShapeStroke::*)() const);
}

void KoShapeStrokeSchemaContractTest::shapeStrokeColorBrushAndPenSignaturesRemainStable()
{
    ASSERT_SHAPE_STROKE_SIGNATURE(color, QColor (KoShapeStroke::*)() const);
    ASSERT_SHAPE_STROKE_SIGNATURE(setColor, void (KoShapeStroke::*)(const QColor &));
    ASSERT_SHAPE_STROKE_SIGNATURE(lineBrush, QBrush (KoShapeStroke::*)() const);
    ASSERT_SHAPE_STROKE_SIGNATURE(setLineBrush, void (KoShapeStroke::*)(const QBrush &));
    ASSERT_SHAPE_STROKE_SIGNATURE(resultLinePen, QPen (KoShapeStroke::*)() const);
}

void KoShapeStrokeSchemaContractTest::shapeStrokeRenderingAndComparisonSignaturesRemainStable()
{
    ASSERT_SHAPE_STROKE_SIGNATURE(strokeInsets, void (KoShapeStroke::*)(const KoShape *, KoInsets &) const);
    ASSERT_SHAPE_STROKE_SIGNATURE(strokeMaxMarkersInset, qreal (KoShapeStroke::*)(const KoShape *) const);
    ASSERT_SHAPE_STROKE_SIGNATURE(hasTransparency, bool (KoShapeStroke::*)() const);
    ASSERT_SHAPE_STROKE_SIGNATURE(paint, void (KoShapeStroke::*)(const KoShape *, QPainter &) const);
    ASSERT_SHAPE_STROKE_SIGNATURE(paintMarkers, void (KoShapeStroke::*)(const KoShape *, QPainter &) const);
    ASSERT_SHAPE_STROKE_SIGNATURE(compareFillTo, bool (KoShapeStroke::*)(const KoShapeStrokeModel *));
    ASSERT_SHAPE_STROKE_SIGNATURE(compareStyleTo, bool (KoShapeStroke::*)(const KoShapeStrokeModel *));
    ASSERT_SHAPE_STROKE_SIGNATURE(isVisible, bool (KoShapeStroke::*)() const);
}

QTEST_GUILESS_MAIN(KoShapeStrokeSchemaContractTest)

#include "KoShapeStrokeSchemaContractTest.moc"
