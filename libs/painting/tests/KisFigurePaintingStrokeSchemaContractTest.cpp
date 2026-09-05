/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_figure_painting_stroke.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(method, signature)                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisFigurePaintingStroke::method)), signature>)
} // namespace

class KisFigurePaintingStrokeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void figurePaintingStrokeTypeLifetimeAndConstructionSchemaRemainStable();
    void figurePaintingStrokeResourceOverrideSignaturesRemainStable();
    void figurePaintingStrokeGeometrySignaturesRemainStable();
    void figurePaintingStrokePainterPathSignatureRemainsStable();
    void figurePaintingStrokePenPathSignaturesRemainStable();
};

void KisFigurePaintingStrokeSchemaContractTest::figurePaintingStrokeTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Stroke = KisFigurePaintingStroke;

    static_assert(std::is_class_v<Stroke>);
    static_assert(std::is_same_v<decltype(Stroke(std::declval<const KUndo2MagicString &>(),
                                                 std::declval<KisImageWSP>(),
                                                 std::declval<KisNodeSP>(),
                                                 std::declval<KoCanvasResourceProvider *>(),
                                                 std::declval<KisFigurePaintingOptions::StrokeStyle>(),
                                                 std::declval<KisFigurePaintingOptions::FillStyle>())),
                                 Stroke>);
    static_assert(std::is_destructible_v<Stroke>);
    static_assert(!std::is_copy_constructible_v<Stroke>);
    static_assert(!std::is_copy_assignable_v<Stroke>);
}

void KisFigurePaintingStrokeSchemaContractTest::figurePaintingStrokeResourceOverrideSignaturesRemainStable()
{
    using Stroke = KisFigurePaintingStroke;

    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(setFGColorOverride, void (Stroke::*)(const KoColor &));
    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(setBGColorOverride, void (Stroke::*)(const KoColor &));
    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(setBrush, void (Stroke::*)(const KisPaintOpPresetSP &));
    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(setSelectionOverride, void (Stroke::*)(KisSelectionSP));
}

void KisFigurePaintingStrokeSchemaContractTest::figurePaintingStrokeGeometrySignaturesRemainStable()
{
    using Stroke = KisFigurePaintingStroke;

    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(paintLine,
                                            void (Stroke::*)(const KisPaintInformation &, const KisPaintInformation &));
    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(paintPolyline, void (Stroke::*)(const vQPointF &));
    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(paintPolygon, void (Stroke::*)(const vQPointF &));
    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(paintRect, void (Stroke::*)(const QRectF &));
    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(paintEllipse, void (Stroke::*)(const QRectF &));
}

void KisFigurePaintingStrokeSchemaContractTest::figurePaintingStrokePainterPathSignatureRemainsStable()
{
    using Stroke = KisFigurePaintingStroke;

    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(paintPainterPath, void (Stroke::*)(const QPainterPath &));
}

void KisFigurePaintingStrokeSchemaContractTest::figurePaintingStrokePenPathSignaturesRemainStable()
{
    using Stroke = KisFigurePaintingStroke;

    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(paintPainterPathQPen,
                                            void (Stroke::*)(const QPainterPath, const QPen &, const KoColor &));
    ASSERT_FIGURE_PAINTING_STROKE_SIGNATURE(paintPainterPathQPenFill,
                                            void (Stroke::*)(const QPainterPath, const QPen &, const KoColor &));
}

QTEST_APPLESS_MAIN(KisFigurePaintingStrokeSchemaContractTest)

#include "KisFigurePaintingStrokeSchemaContractTest.moc"
