/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoFlake.h>
#include <KoGradientBackground.h>
#include <KoMeshGradientBackground.h>
#include <KoPatternBackground.h>
#include <KoShapeFillWrapper.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_GRADIENT_BACKGROUND_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoGradientBackground::method)), signature>)
#define ASSERT_MESH_GRADIENT_BACKGROUND_SIGNATURE(method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoMeshGradientBackground::method)), signature>)
#define ASSERT_PATTERN_BACKGROUND_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPatternBackground::method)), signature>)
#define ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeFillWrapper::method)), signature>)
#define ASSERT_FLAKE_FUNCTION_SIGNATURE(function, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoFlake::function)), signature>)
} // namespace

class KoPatternBackgroundSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gradientBackgroundTypeAndLifetimeSchemaRemainStable();
    void gradientBackgroundValueSemanticsSignaturesRemainStable();
    void gradientBackgroundGradientAndRenderingSignaturesRemainStable();
    void meshGradientBackgroundTypeAndLifetimeSchemaRemainStable();
    void meshGradientBackgroundValueAndRenderingSignaturesRemainStable();
    void patternBackgroundTypeAndValueLifetimeSchemaRemainStable();
    void patternBackgroundRepeatAndReferenceOrdinalsRemainStable();
    void patternImageAndDisplayGeometrySignaturesRemainStable();
    void patternReferencePlacementSignaturesRemainStable();
    void patternTileAndTransformSignaturesRemainStable();
    void shapeFillWrapperTypeAndConstructionSchemaRemainStable();
    void shapeFillWrapperClassificationAndColorSignaturesRemainStable();
    void shapeFillWrapperGradientSignaturesRemainStable();
    void shapeFillWrapperMeshGradientSignaturesRemainStable();
    void gradientUtilityAndPatternRenderingSignaturesRemainStable();
};

void KoPatternBackgroundSchemaContractTest::gradientBackgroundTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoGradientBackground>);
    static_assert(std::is_base_of_v<KoShapeBackground, KoGradientBackground>);
    static_assert(std::is_constructible_v<KoGradientBackground, QGradient *>);
    static_assert(std::is_constructible_v<KoGradientBackground, QGradient *, const QTransform &>);
    static_assert(std::is_constructible_v<KoGradientBackground, const QGradient &>);
    static_assert(std::is_constructible_v<KoGradientBackground, const QGradient &, const QTransform &>);
    static_assert(std::is_copy_constructible_v<KoGradientBackground>);
    static_assert(std::is_destructible_v<KoGradientBackground>);
    static_assert(std::has_virtual_destructor_v<KoGradientBackground>);
}

void KoPatternBackgroundSchemaContractTest::gradientBackgroundValueSemanticsSignaturesRemainStable()
{
    ASSERT_GRADIENT_BACKGROUND_SIGNATURE(operator=,
                                         KoGradientBackground
                                             & (KoGradientBackground::*)(const KoGradientBackground &));
    ASSERT_GRADIENT_BACKGROUND_SIGNATURE(compareTo, bool (KoGradientBackground::*)(const KoShapeBackground *) const);
}

void KoPatternBackgroundSchemaContractTest::gradientBackgroundGradientAndRenderingSignaturesRemainStable()
{
    ASSERT_GRADIENT_BACKGROUND_SIGNATURE(gradient, const QGradient *(KoGradientBackground::*)() const);
    ASSERT_GRADIENT_BACKGROUND_SIGNATURE(setGradient, void (KoGradientBackground::*)(const QGradient &));
    ASSERT_GRADIENT_BACKGROUND_SIGNATURE(transform, QTransform (KoGradientBackground::*)() const);
    ASSERT_GRADIENT_BACKGROUND_SIGNATURE(setTransform, void (KoGradientBackground::*)(const QTransform &));
    ASSERT_GRADIENT_BACKGROUND_SIGNATURE(paint, void (KoGradientBackground::*)(QPainter &, const QPainterPath &) const);
}

void KoPatternBackgroundSchemaContractTest::meshGradientBackgroundTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoMeshGradientBackground>);
    static_assert(std::is_base_of_v<KoShapeBackground, KoMeshGradientBackground>);
    static_assert(std::is_constructible_v<KoMeshGradientBackground, const SvgMeshGradient *>);
    static_assert(std::is_constructible_v<KoMeshGradientBackground, const SvgMeshGradient *, const QTransform &>);
    static_assert(std::is_copy_constructible_v<KoMeshGradientBackground>);
    static_assert(std::is_destructible_v<KoMeshGradientBackground>);
    static_assert(std::has_virtual_destructor_v<KoMeshGradientBackground>);
}

void KoPatternBackgroundSchemaContractTest::meshGradientBackgroundValueAndRenderingSignaturesRemainStable()
{
    ASSERT_MESH_GRADIENT_BACKGROUND_SIGNATURE(operator=,
                                              KoMeshGradientBackground
                                                  & (KoMeshGradientBackground::*)(const KoMeshGradientBackground &));
    ASSERT_MESH_GRADIENT_BACKGROUND_SIGNATURE(compareTo,
                                              bool (KoMeshGradientBackground::*)(const KoShapeBackground *) const);
    ASSERT_MESH_GRADIENT_BACKGROUND_SIGNATURE(gradient, SvgMeshGradient * (KoMeshGradientBackground::*)());
    ASSERT_MESH_GRADIENT_BACKGROUND_SIGNATURE(transform, QTransform (KoMeshGradientBackground::*)());
    ASSERT_MESH_GRADIENT_BACKGROUND_SIGNATURE(paint,
                                              void (KoMeshGradientBackground::*)(QPainter &, const QPainterPath &)
                                                  const);
}

void KoPatternBackgroundSchemaContractTest::patternBackgroundTypeAndValueLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoPatternBackground>);
    static_assert(std::is_base_of_v<KoShapeBackground, KoPatternBackground>);
    static_assert(std::is_default_constructible_v<KoPatternBackground>);
    static_assert(std::is_copy_constructible_v<KoPatternBackground>);
    static_assert(std::is_copy_assignable_v<KoPatternBackground>);
    static_assert(std::is_destructible_v<KoPatternBackground>);
    static_assert(std::has_virtual_destructor_v<KoPatternBackground>);

    ASSERT_PATTERN_BACKGROUND_SIGNATURE(compareTo, bool (KoPatternBackground::*)(const KoShapeBackground *) const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(operator=,
                                        KoPatternBackground & (KoPatternBackground::*)(const KoPatternBackground &));
}

void KoPatternBackgroundSchemaContractTest::patternBackgroundRepeatAndReferenceOrdinalsRemainStable()
{
    static_assert(std::is_enum_v<KoPatternBackground::PatternRepeat>);
    static_assert(KoPatternBackground::Original == 0);
    static_assert(KoPatternBackground::Tiled == 1);
    static_assert(KoPatternBackground::Stretched == 2);

    static_assert(std::is_enum_v<KoPatternBackground::ReferencePoint>);
    static_assert(KoPatternBackground::TopLeft == 0);
    static_assert(KoPatternBackground::Top == 1);
    static_assert(KoPatternBackground::TopRight == 2);
    static_assert(KoPatternBackground::Left == 3);
    static_assert(KoPatternBackground::Center == 4);
    static_assert(KoPatternBackground::Right == 5);
    static_assert(KoPatternBackground::BottomLeft == 6);
    static_assert(KoPatternBackground::Bottom == 7);
    static_assert(KoPatternBackground::BottomRight == 8);
}

void KoPatternBackgroundSchemaContractTest::patternImageAndDisplayGeometrySignaturesRemainStable()
{
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(pattern, QImage (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(patternDisplaySize, QSizeF (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(patternOriginalSize, QSizeF (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(patternRectFromFillSize, QRectF (KoPatternBackground::*)(const QSizeF &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(repeat, KoPatternBackground::PatternRepeat (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setPattern, void (KoPatternBackground::*)(const QImage &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setPatternDisplaySize, void (KoPatternBackground::*)(const QSizeF &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setRepeat, void (KoPatternBackground::*)(KoPatternBackground::PatternRepeat));
}

void KoPatternBackgroundSchemaContractTest::patternReferencePlacementSignaturesRemainStable()
{
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(referencePoint,
                                        KoPatternBackground::ReferencePoint (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(referencePointOffset, QPointF (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setReferencePoint,
                                        void (KoPatternBackground::*)(KoPatternBackground::ReferencePoint));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setReferencePointOffset, void (KoPatternBackground::*)(const QPointF &));
}

void KoPatternBackgroundSchemaContractTest::patternTileAndTransformSignaturesRemainStable()
{
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setTileRepeatOffset, void (KoPatternBackground::*)(const QPointF &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setTransform, void (KoPatternBackground::*)(const QTransform &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(tileRepeatOffset, QPointF (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(transform, QTransform (KoPatternBackground::*)() const);
}

void KoPatternBackgroundSchemaContractTest::shapeFillWrapperTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KoShapeFillWrapper>);
    static_assert(std::is_constructible_v<KoShapeFillWrapper, KoShape *, KoFlake::FillVariant>);
    static_assert(std::is_constructible_v<KoShapeFillWrapper, QList<KoShape *>, KoFlake::FillVariant>);
    static_assert(std::is_destructible_v<KoShapeFillWrapper>);
}

void KoPatternBackgroundSchemaContractTest::shapeFillWrapperClassificationAndColorSignaturesRemainStable()
{
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(color, QColor (KoShapeFillWrapper::*)() const);
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(hasZeroLineWidth, bool (KoShapeFillWrapper::*)() const);
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(isMixedFill, bool (KoShapeFillWrapper::*)() const);
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(setColor, KUndo2Command * (KoShapeFillWrapper::*)(const QColor &));
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(setLineWidth, KUndo2Command * (KoShapeFillWrapper::*)(const float &));
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(type, KoFlake::FillType (KoShapeFillWrapper::*)() const);
}

void KoPatternBackgroundSchemaContractTest::shapeFillWrapperGradientSignaturesRemainStable()
{
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(applyGradient, KUndo2Command * (KoShapeFillWrapper::*)(const QGradient *));
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(applyGradientStopsOnly,
                                        KUndo2Command * (KoShapeFillWrapper::*)(const QGradient *));
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(gradient, const QGradient *(KoShapeFillWrapper::*)() const);
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(gradientTransform, QTransform (KoShapeFillWrapper::*)() const);
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(setGradient,
                                        KUndo2Command * (KoShapeFillWrapper::*)(const QGradient *, const QTransform &));
}

void KoPatternBackgroundSchemaContractTest::shapeFillWrapperMeshGradientSignaturesRemainStable()
{
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(meshgradient, const SvgMeshGradient *(KoShapeFillWrapper::*)() const);
    ASSERT_SHAPE_FILL_WRAPPER_SIGNATURE(setMeshGradient,
                                        KUndo2Command
                                            * (KoShapeFillWrapper::*)(const SvgMeshGradient *, const QTransform &));
}

void KoPatternBackgroundSchemaContractTest::gradientUtilityAndPatternRenderingSignaturesRemainStable()
{
    ASSERT_FLAKE_FUNCTION_SIGNATURE(cloneGradient, QGradient * (*)(const QGradient *));
    ASSERT_FLAKE_FUNCTION_SIGNATURE(mergeGradient, QGradient * (*)(const QGradient *, const QGradient *));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(paint, void (KoPatternBackground::*)(QPainter &, const QPainterPath &) const);
}

QTEST_GUILESS_MAIN(KoPatternBackgroundSchemaContractTest)

#include "KoPatternBackgroundSchemaContractTest.moc"
