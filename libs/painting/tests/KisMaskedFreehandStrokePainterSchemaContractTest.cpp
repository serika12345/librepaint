/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <strokes/KisMaskedFreehandStrokePainter.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisMaskedFreehandStrokePainterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void maskedFreehandPainterTypeLifetimeAndPresetSchemaRemainStable();
    void maskedFreehandPainterDabLineAndBezierSignaturesRemainStable();
    void maskedFreehandPainterGeometryPrimitiveSignaturesRemainStable();
    void maskedFreehandPainterPathDrawingSignaturesRemainStable();
    void maskedFreehandPainterAsynchronousDirtyAndMaskSignaturesRemainStable();
};

using Subject = KisMaskedFreehandStrokePainter;

#define ASSERT_SIGNATURE(Method, Signature)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Subject::Method)), Signature>)

void KisMaskedFreehandStrokePainterSchemaContractTest::maskedFreehandPainterTypeLifetimeAndPresetSchemaRemainStable()
{
    using Preset = KisPaintOpPresetSP (Subject::*)() const;
    static_assert(std::is_class_v<Subject>);
    static_assert(std::is_constructible_v<Subject, KisFreehandStrokeInfo *, KisFreehandStrokeInfo *>);
    ASSERT_SIGNATURE(preset, Preset);
}

void KisMaskedFreehandStrokePainterSchemaContractTest::maskedFreehandPainterDabLineAndBezierSignaturesRemainStable()
{
    using Dab = void (Subject::*)(const KisPaintInformation &);
    using Line = void (Subject::*)(const KisPaintInformation &, const KisPaintInformation &);
    using Bezier =
        void (Subject::*)(const KisPaintInformation &, const QPointF &, const QPointF &, const KisPaintInformation &);
    ASSERT_SIGNATURE(paintAt, Dab);
    ASSERT_SIGNATURE(paintLine, Line);
    ASSERT_SIGNATURE(paintBezierCurve, Bezier);
}

void KisMaskedFreehandStrokePainterSchemaContractTest::maskedFreehandPainterGeometryPrimitiveSignaturesRemainStable()
{
    using Rect = void (Subject::*)(const QRectF &);
    using Path = void (Subject::*)(const QPainterPath &);
    using Points = void (Subject::*)(const QVector<QPointF> &);
    using Polyline = void (Subject::*)(const QVector<QPointF> &, int, int);
    ASSERT_SIGNATURE(paintEllipse, Rect);
    ASSERT_SIGNATURE(paintPainterPath, Path);
    ASSERT_SIGNATURE(paintPolygon, Points);
    ASSERT_SIGNATURE(paintPolyline, Polyline);
    ASSERT_SIGNATURE(paintRect, Rect);
    static_assert(
        std::is_same_v<decltype(std::declval<Subject &>().paintPolyline(std::declval<const QVector<QPointF> &>())),
                       void>);
}

void KisMaskedFreehandStrokePainterSchemaContractTest::maskedFreehandPainterPathDrawingSignaturesRemainStable()
{
    using Draw = void (Subject::*)(const QPainterPath &, const QPen &);
    using Fill = void (Subject::*)(const QPainterPath &, const QPen &, const KoColor &);
    ASSERT_SIGNATURE(drawAndFillPainterPath, Fill);
    ASSERT_SIGNATURE(drawPainterPath, Draw);
}

void KisMaskedFreehandStrokePainterSchemaContractTest::
    maskedFreehandPainterAsynchronousDirtyAndMaskSignaturesRemainStable()
{
    using Update = std::pair<int, bool> (Subject::*)(QVector<KisRunnableStrokeJobData *> &);
    using Boolean = bool (Subject::*)() const;
    using Dirty = QVector<QRect> (Subject::*)();
    ASSERT_SIGNATURE(doAsynchronousUpdate, Update);
    ASSERT_SIGNATURE(hasDirtyRegion, Boolean);
    ASSERT_SIGNATURE(hasMasking, Boolean);
    ASSERT_SIGNATURE(takeDirtyRegion, Dirty);
}

#undef ASSERT_SIGNATURE

QTEST_APPLESS_MAIN(KisMaskedFreehandStrokePainterSchemaContractTest)

#include "KisMaskedFreehandStrokePainterSchemaContractTest.moc"
