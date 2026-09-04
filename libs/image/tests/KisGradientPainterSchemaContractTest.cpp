/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_gradient_painter.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_GRADIENT_PAINTER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGradientPainter::method)), signature>)

struct GradientPaintPolicyProbe;

} // namespace

class KisGradientPainterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gradientPainterTypeLifetimeAndConstructionSchemaRemainStable();
    void gradientPainterShapeSchemaRemainStable();
    void gradientPainterRepeatSchemaRemainStable();
    void gradientPainterConfigurationSignaturesRemainStable();
    void gradientPainterPaintingSignaturesRemainStable();
};

void KisGradientPainterSchemaContractTest::gradientPainterTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisGradientPainter>);
    static_assert(std::is_default_constructible_v<KisGradientPainter>);
    static_assert(std::is_constructible_v<KisGradientPainter, KisPaintDeviceSP>);
    static_assert(std::is_constructible_v<KisGradientPainter, KisPaintDeviceSP, KisSelectionSP>);
    static_assert(std::has_virtual_destructor_v<KisGradientPainter>);
}

void KisGradientPainterSchemaContractTest::gradientPainterShapeSchemaRemainStable()
{
    using Shape = KisGradientPainter::enumGradientShape;

    static_assert(std::is_enum_v<Shape>);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapeLinear) == 0);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapeBiLinear) == 1);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapeRadial) == 2);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapeSquare) == 3);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapeConical) == 4);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapeConicalSymetric) == 5);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapeSpiral) == 6);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapeReverseSpiral) == 7);
    static_assert(static_cast<int>(KisGradientPainter::GradientShapePolygonal) == 8);
}

void KisGradientPainterSchemaContractTest::gradientPainterRepeatSchemaRemainStable()
{
    using Repeat = KisGradientPainter::enumGradientRepeat;

    static_assert(std::is_enum_v<Repeat>);
    static_assert(static_cast<int>(KisGradientPainter::GradientRepeatNone) == 0);
    static_assert(static_cast<int>(KisGradientPainter::GradientRepeatForwards) == 1);
    static_assert(static_cast<int>(KisGradientPainter::GradientRepeatAlternate) == 2);
}

void KisGradientPainterSchemaContractTest::gradientPainterConfigurationSignaturesRemainStable()
{
    ASSERT_GRADIENT_PAINTER_SIGNATURE(setGradientShape,
                                      void (KisGradientPainter::*)(KisGradientPainter::enumGradientShape));
    ASSERT_GRADIENT_PAINTER_SIGNATURE(precalculateShape, void (KisGradientPainter::*)());
}

void KisGradientPainterSchemaContractTest::gradientPainterPaintingSignaturesRemainStable()
{
    using CoordinateSignature = bool (KisGradientPainter::*)(const QPointF &,
                                                             const QPointF &,
                                                             KisGradientPainter::enumGradientRepeat,
                                                             double,
                                                             bool,
                                                             qint32,
                                                             qint32,
                                                             qint32,
                                                             qint32,
                                                             bool);
    using RectangleSignature = bool (KisGradientPainter::*)(const QPointF &,
                                                            const QPointF &,
                                                            KisGradientPainter::enumGradientRepeat,
                                                            double,
                                                            bool,
                                                            const QRect &,
                                                            bool);
    using PolicySignature = bool (KisGradientPainter::*)(const QPointF &,
                                                         const QPointF &,
                                                         KisGradientPainter::enumGradientRepeat,
                                                         double,
                                                         bool,
                                                         bool,
                                                         const QRect &,
                                                         GradientPaintPolicyProbe &);

    ASSERT_GRADIENT_PAINTER_SIGNATURE(paintGradient, CoordinateSignature);
    ASSERT_GRADIENT_PAINTER_SIGNATURE(paintGradient, RectangleSignature);
    static_assert(std::is_same_v<decltype(static_cast<PolicySignature>(
                                     &KisGradientPainter::paintGradient<GradientPaintPolicyProbe>)),
                                 PolicySignature>);
}

QTEST_GUILESS_MAIN(KisGradientPainterSchemaContractTest)

#include "KisGradientPainterSchemaContractTest.moc"
