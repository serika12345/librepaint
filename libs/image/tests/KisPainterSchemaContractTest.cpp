/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_distance_information.h"
#include "kis_painter.h"

#include <QTest>

#include <type_traits>

#define ASSERT_PAINTER_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPainter::method)), signature>)
#define ASSERT_DISTANCE_INIT_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDistanceInitInfo::method)), signature>)

class KisPainterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndDrawingStyleSchemaRemainStable();
    void deviceSelectionAndSessionSignaturesRemainStable();
    void transactionSignaturesRemainStable();
    void dirtyAndCompositionSignaturesRemainStable();
    void paintDeviceTransferSignaturesRemainStable();
    void fixedDeviceTransferSignaturesRemainStable();
    void mirroringSignaturesRemainStable();
    void primitiveDrawingSignaturesRemainStable();
    void paintOpStrokeSignaturesRemainStable();
    void colorFillAndResourceSignaturesRemainStable();
    void opacityAndFlowSignaturesRemainStable();
    void conversionProgressAndJobSignaturesRemainStable();
    void distanceInitTypeConstructionAndLifetimeSchemaRemainStable();
    void distanceInitEqualityAndAssignmentSignaturesRemainStable();
    void distanceInitCreationSignatureRemainsStable();
    void distanceInitXmlSignaturesRemainStable();
};

// clang-format off
void KisPainterSchemaContractTest::typeAndDrawingStyleSchemaRemainStable()
{
    using Painter = KisPainter;
    static_assert(std::is_class_v<Painter>);
    static_assert(std::is_enum_v<Painter::FillStyle>);
    static_assert(Painter::FillStyleNone == 0);
    static_assert(Painter::FillStyleForegroundColor == 1);
    static_assert(Painter::FillStyleBackgroundColor == 2);
    static_assert(Painter::FillStylePattern == 3);
    static_assert(Painter::FillStyleGenerator == 4);
    static_assert(std::is_enum_v<Painter::StrokeStyle>);
    static_assert(Painter::StrokeStyleNone == 0);
    static_assert(Painter::StrokeStyleBrush == 1);
    static_assert(std::is_default_constructible_v<Painter>);
    static_assert(std::is_constructible_v<Painter, KisPaintDeviceSP>);
    static_assert(std::is_constructible_v<Painter, KisPaintDeviceSP, KisSelectionSP>);
    static_assert(std::has_virtual_destructor_v<Painter>);
}

void KisPainterSchemaContractTest::deviceSelectionAndSessionSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(begin, void (Painter::*)(KisPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(begin, void (Painter::*)(KisPaintDeviceSP, KisSelectionSP));
    ASSERT_PAINTER_SIGNATURE(device, KisPaintDeviceSP (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(device, const KisPaintDeviceSP (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(end, void (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(selection, KisSelectionSP (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(setSelection, void (Painter::*)(KisSelectionSP));
}

void KisPainterSchemaContractTest::transactionSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(beginTransaction, void (Painter::*)(const KUndo2MagicString &, int));
    ASSERT_PAINTER_SIGNATURE(deleteTransaction, void (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(endAndTakeTransaction, KUndo2Command *(Painter::*)());
    ASSERT_PAINTER_SIGNATURE(endTransaction, void (Painter::*)(KisPostExecutionUndoAdapter *));
    ASSERT_PAINTER_SIGNATURE(endTransaction, void (Painter::*)(KisUndoAdapter *));
    ASSERT_PAINTER_SIGNATURE(putTransaction, void (Painter::*)(KisTransaction *));
    ASSERT_PAINTER_SIGNATURE(revertTransaction, void (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(takeTransaction, KisTransaction *(Painter::*)());
}

void KisPainterSchemaContractTest::dirtyAndCompositionSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(addDirtyRect, void (Painter::*)(const QRect &));
    ASSERT_PAINTER_SIGNATURE(addDirtyRects, void (Painter::*)(const QVector<QRect> &));
    ASSERT_PAINTER_SIGNATURE(hasDirtyRegion, bool (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(takeDirtyRegion, QVector<QRect> (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(channelFlags, QBitArray (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(setChannelFlags, void (Painter::*)(QBitArray));
    ASSERT_PAINTER_SIGNATURE(compositeOpId, QString (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(setCompositeOpId, void (Painter::*)(const KoCompositeOp *));
    ASSERT_PAINTER_SIGNATURE(setCompositeOpId, void (Painter::*)(const QString &));
    ASSERT_PAINTER_SIGNATURE(setColorConversionFlags, void (Painter::*)(KoColorConversionTransformation::ConversionFlags));
    ASSERT_PAINTER_SIGNATURE(setRenderingIntent, void (Painter::*)(KoColorConversionTransformation::Intent));
    ASSERT_PAINTER_SIGNATURE(fillStyle, Painter::FillStyle (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(setFillStyle, void (Painter::*)(Painter::FillStyle));
    ASSERT_PAINTER_SIGNATURE(strokeStyle, Painter::StrokeStyle (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(setStrokeStyle, void (Painter::*)(Painter::StrokeStyle));
    ASSERT_PAINTER_SIGNATURE(antiAliasPolygonFill, bool (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(setAntiAliasPolygonFill, void (Painter::*)(bool));
}

void KisPainterSchemaContractTest::paintDeviceTransferSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(bitBlt, void (Painter::*)(const QPoint &, KisPaintDeviceSP, const QRect &));
    ASSERT_PAINTER_SIGNATURE(bitBlt, void (Painter::*)(qint32, qint32, KisPaintDeviceSP, qint32, qint32, qint32, qint32));
    ASSERT_PAINTER_SIGNATURE(bitBltOldData, void (Painter::*)(const QPoint &, KisPaintDeviceSP, const QRect &));
    ASSERT_PAINTER_SIGNATURE(bitBltOldData, void (Painter::*)(qint32, qint32, KisPaintDeviceSP, qint32, qint32, qint32, qint32));
    ASSERT_PAINTER_SIGNATURE(bitBltWithFixedSelection, void (Painter::*)(qint32, qint32, KisPaintDeviceSP, KisFixedPaintDeviceSP, qint32, qint32, qint32, qint32, qint32, qint32));
    ASSERT_PAINTER_SIGNATURE(bitBltWithFixedSelection, void (Painter::*)(qint32, qint32, KisPaintDeviceSP, KisFixedPaintDeviceSP, qint32, qint32));
    ASSERT_PAINTER_SIGNATURE(copyAreaOptimized, void (*)(const QPoint &, KisPaintDeviceSP, KisPaintDeviceSP, const QRect &));
    ASSERT_PAINTER_SIGNATURE(copyAreaOptimized, void (*)(const QPoint &, KisPaintDeviceSP, KisPaintDeviceSP, const QRect &, KisSelectionSP));
    ASSERT_PAINTER_SIGNATURE(copyAreaOptimizedOldData, void (*)(const QPoint &, KisPaintDeviceSP, KisPaintDeviceSP, const QRect &));
}

void KisPainterSchemaContractTest::fixedDeviceTransferSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(bltFixed, void (Painter::*)(const QPoint &, KisFixedPaintDeviceSP, const QRect &));
    ASSERT_PAINTER_SIGNATURE(bltFixed, void (Painter::*)(const QRect &, QList<KisRenderedDab>));
    ASSERT_PAINTER_SIGNATURE(bltFixed, void (Painter::*)(qint32, qint32, KisFixedPaintDeviceSP, qint32, qint32, qint32, qint32));
    ASSERT_PAINTER_SIGNATURE(bltFixedWithFixedSelection, void (Painter::*)(qint32, qint32, KisFixedPaintDeviceSP, KisFixedPaintDeviceSP, qint32, qint32, qint32, qint32, quint32, quint32));
    ASSERT_PAINTER_SIGNATURE(bltFixedWithFixedSelection, void (Painter::*)(qint32, qint32, KisFixedPaintDeviceSP, KisFixedPaintDeviceSP, quint32, quint32));
    ASSERT_PAINTER_SIGNATURE(renderDabWithMirroringNonIncremental, void (Painter::*)(QRect, KisPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(renderMirrorMask, void (Painter::*)(QRect, KisFixedPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(renderMirrorMask, void (Painter::*)(QRect, KisFixedPaintDeviceSP, KisFixedPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(renderMirrorMask, void (Painter::*)(QRect, KisPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(renderMirrorMask, void (Painter::*)(QRect, KisPaintDeviceSP, int, int, KisFixedPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(renderMirrorMaskSafe, void (Painter::*)(QRect, KisFixedPaintDeviceSP, KisFixedPaintDeviceSP, bool));
    ASSERT_PAINTER_SIGNATURE(renderMirrorMaskSafe, void (Painter::*)(QRect, KisFixedPaintDeviceSP, bool));
    ASSERT_PAINTER_SIGNATURE(renderMirrorMaskSafe, void (Painter::*)(QRect, KisPaintDeviceSP, int, int, KisFixedPaintDeviceSP, bool));
}

void KisPainterSchemaContractTest::mirroringSignaturesRemainStable()
{
    using Painter = KisPainter;
    using PointPair = QPair<QPointF, QPointF>;
    using MirroredPairs = QVector<PointPair>;
    ASSERT_PAINTER_SIGNATURE(calculateAllMirroredPoints, const MirroredPairs (Painter::*)(const PointPair &));
    ASSERT_PAINTER_SIGNATURE(calculateAllMirroredPoints, const QVector<QPointF> (Painter::*)(const QPointF &));
    ASSERT_PAINTER_SIGNATURE(calculateAllMirroredRects, const QVector<QRect> (Painter::*)(const QRect &));
    ASSERT_PAINTER_SIGNATURE(copyMirrorInformationFrom, void (Painter::*)(const Painter *));
    ASSERT_PAINTER_SIGNATURE(hasHorizontalMirroring, bool (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(hasMirroring, bool (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(hasVerticalMirroring, bool (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(mirrorDab, void (Painter::*)(Qt::Orientation, KisRenderedDab *, bool) const);
    ASSERT_PAINTER_SIGNATURE(mirrorRect, void (Painter::*)(Qt::Orientation, QRect *) const);
    ASSERT_PAINTER_SIGNATURE(setMaskImageSize, void (Painter::*)(qint32, qint32));
    ASSERT_PAINTER_SIGNATURE(setMirrorInformation, void (Painter::*)(const QPointF &, bool, bool));
}

void KisPainterSchemaContractTest::primitiveDrawingSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(drawDDALine, void (Painter::*)(const QPointF &, const QPointF &));
    ASSERT_PAINTER_SIGNATURE(drawLine, void (Painter::*)(const QPointF &, const QPointF &));
    ASSERT_PAINTER_SIGNATURE(drawLine, void (Painter::*)(const QPointF &, const QPointF &, qreal, bool));
    ASSERT_PAINTER_SIGNATURE(drawPainterPath, void (Painter::*)(const QPainterPath &, const QPen &));
    ASSERT_PAINTER_SIGNATURE(drawPainterPath, void (Painter::*)(const QPainterPath &, const QPen &, const QRect &));
    ASSERT_PAINTER_SIGNATURE(drawThickLine, void (Painter::*)(const QPointF &, const QPointF &, int, int));
    ASSERT_PAINTER_SIGNATURE(drawWobblyLine, void (Painter::*)(const QPointF &, const QPointF &));
    ASSERT_PAINTER_SIGNATURE(drawWuLine, void (Painter::*)(const QPointF &, const QPointF &));
    ASSERT_PAINTER_SIGNATURE(fill, void (Painter::*)(qint32, qint32, qint32, qint32, const KoColor &));
    ASSERT_PAINTER_SIGNATURE(fillPainterPath, void (Painter::*)(const QPainterPath &));
    ASSERT_PAINTER_SIGNATURE(fillPainterPath, void (Painter::*)(const QPainterPath &, const QRect &));
    ASSERT_PAINTER_SIGNATURE(getBezierCurvePoints, void (Painter::*)(const QPointF &, const QPointF &, const QPointF &, const QPointF &, vQPointF &) const);
    ASSERT_PAINTER_SIGNATURE(paintEllipse, void (Painter::*)(const QRectF &));
    ASSERT_PAINTER_SIGNATURE(paintEllipse, void (Painter::*)(const qreal, const qreal, const qreal, const qreal));
    ASSERT_PAINTER_SIGNATURE(paintPainterPath, void (Painter::*)(const QPainterPath &));
    ASSERT_PAINTER_SIGNATURE(paintPolygon, void (Painter::*)(const vQPointF &));
    ASSERT_PAINTER_SIGNATURE(paintPolyline, void (Painter::*)(const QVector<QPointF> &, int, int));
    ASSERT_PAINTER_SIGNATURE(paintRect, void (Painter::*)(const QRectF &));
    ASSERT_PAINTER_SIGNATURE(paintRect, void (Painter::*)(const qreal, const qreal, const qreal, const qreal));
}

void KisPainterSchemaContractTest::paintOpStrokeSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(paintAt, void (Painter::*)(const KisPaintInformation &, KisDistanceInformation *));
    ASSERT_PAINTER_SIGNATURE(paintBezierCurve, void (Painter::*)(const KisPaintInformation &, const QPointF &, const QPointF &, const KisPaintInformation &, KisDistanceInformation *));
    ASSERT_PAINTER_SIGNATURE(paintLine, void (Painter::*)(const KisPaintInformation &, const KisPaintInformation &, KisDistanceInformation *));
    ASSERT_PAINTER_SIGNATURE(paintOp, KisPaintOp *(Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(preset, KisPaintOpPresetSP (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(runnableStrokeJobsInterface, KisRunnableStrokeJobsInterface *(Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(setPaintOpPreset, void (Painter::*)(KisPaintOpPresetSP, KisNodeSP, KisImageSP));
    ASSERT_PAINTER_SIGNATURE(setRunnableStrokeJobsInterface, void (Painter::*)(KisRunnableStrokeJobsInterface *));
}

void KisPainterSchemaContractTest::colorFillAndResourceSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(backgroundColor, const KoColor &(Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(generator, const KisFilterConfigurationSP (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(gradient, const KoAbstractGradientSP (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(paintColor, const KoColor &(Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(pattern, const KoPatternSP (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(patternTransform, QTransform (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(setBackgroundColor, void (Painter::*)(const KoColor &));
    ASSERT_PAINTER_SIGNATURE(setGenerator, void (Painter::*)(KisFilterConfigurationSP));
    ASSERT_PAINTER_SIGNATURE(setGradient, void (Painter::*)(KoAbstractGradientSP));
    ASSERT_PAINTER_SIGNATURE(setPaintColor, void (Painter::*)(const KoColor &));
    ASSERT_PAINTER_SIGNATURE(setPattern, void (Painter::*)(KoPatternSP));
    ASSERT_PAINTER_SIGNATURE(setPatternTransform, void (Painter::*)(QTransform));
}

void KisPainterSchemaContractTest::opacityAndFlowSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(blendAverageOpacity, qreal (*)(qreal, qreal));
    ASSERT_PAINTER_SIGNATURE(flow, qreal (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(isOpacityUnit, bool (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(opacityF, qreal (Painter::*)() const);
    ASSERT_PAINTER_SIGNATURE(setAverageOpacity, void (Painter::*)(qreal));
    ASSERT_PAINTER_SIGNATURE(setFlow, void (Painter::*)(qreal));
    ASSERT_PAINTER_SIGNATURE(setOpacityF, void (Painter::*)(qreal));
    ASSERT_PAINTER_SIGNATURE(setOpacityToUnit, void (Painter::*)());
    ASSERT_PAINTER_SIGNATURE(setOpacityU8, void (Painter::*)(quint8));
    ASSERT_PAINTER_SIGNATURE(setOpacityUpdateAverage, void (Painter::*)(qreal));
}

void KisPainterSchemaContractTest::conversionProgressAndJobSignaturesRemainStable()
{
    using Painter = KisPainter;
    ASSERT_PAINTER_SIGNATURE(checkDeviceHasTransparency, bool (*)(KisPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(convertToAlphaAsAlpha, KisPaintDeviceSP (*)(KisPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(convertToAlphaAsGray, KisPaintDeviceSP (*)(KisPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(convertToAlphaAsPureAlpha, KisPaintDeviceSP (*)(KisPaintDeviceSP));
    ASSERT_PAINTER_SIGNATURE(setProgress, void (Painter::*)(KoUpdater *));
}

void KisPainterSchemaContractTest::distanceInitTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Info = KisDistanceInitInfo;
    static_assert(std::is_class_v<Info>);
    static_assert(std::is_default_constructible_v<Info>);
    static_assert(std::is_copy_constructible_v<Info>);
    static_assert(std::is_constructible_v<Info, qreal, qreal, int>);
    static_assert(std::is_constructible_v<Info, const QPointF &, qreal, int>);
    static_assert(std::is_constructible_v<Info, const QPointF &, qreal, qreal, qreal, int>);
    static_assert(std::is_destructible_v<Info>);
}

void KisPainterSchemaContractTest::distanceInitEqualityAndAssignmentSignaturesRemainStable()
{
    using Info = KisDistanceInitInfo;
    ASSERT_DISTANCE_INIT_SIGNATURE(operator==, bool (Info::*)(const Info &) const);
    ASSERT_DISTANCE_INIT_SIGNATURE(operator=, Info & (Info::*)(const Info &));
}

void KisPainterSchemaContractTest::distanceInitCreationSignatureRemainsStable()
{
    using Info = KisDistanceInitInfo;
    ASSERT_DISTANCE_INIT_SIGNATURE(makeDistInfo, KisDistanceInformation (Info::*)());
}

void KisPainterSchemaContractTest::distanceInitXmlSignaturesRemainStable()
{
    using Info = KisDistanceInitInfo;
    ASSERT_DISTANCE_INIT_SIGNATURE(toXML, void (Info::*)(QDomDocument &, QDomElement &) const);
    ASSERT_DISTANCE_INIT_SIGNATURE(fromXML, Info (*)(const QDomElement &));
}

#undef ASSERT_DISTANCE_INIT_SIGNATURE
// clang-format on

QTEST_APPLESS_MAIN(KisPainterSchemaContractTest)

#include "KisPainterSchemaContractTest.moc"
