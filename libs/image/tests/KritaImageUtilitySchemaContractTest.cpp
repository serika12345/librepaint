/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <krita_utils.h>

#include <QTest>

#include <type_traits>

namespace
{
struct VisitorProbe {
    void operator()(const QPoint &) const;
};

#define ASSERT_KRITA_UTILITY_SIGNATURE(function, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KritaUtils::function)), signature>)
} // namespace

class KritaImageUtilitySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void partitioningAndFormattingSchemaRemainsStable();
    void opacityChannelNodeAndPaintingSchemaRemainsStable();
    void deviceAndGeometryMutationSchemaRemainsStable();
    void thresholdSchemaRemainsStable();
    void rasterizationSchemaRemainsStable();
};

void KritaImageUtilitySchemaContractTest::partitioningAndFormattingSchemaRemainsStable()
{
    using OptimalPatchSizeSignature = QSize (*)();
    using SplitRectSignature = QVector<QRect> (*)(const QRect &, const QSize &);
    using SplitQRegionSignature = QVector<QRect> (*)(const QRegion &, const QSize &);
    using SplitKisRegionSignature = QVector<QRect> (*)(const KisRegion &, const QSize &);
    using SplitTrianglesSignature = KisRegion (*)(const QPointF &, const QVector<QPointF> &);
    using SplitPathSignature = KisRegion (*)(const QPainterPath &);
    using SplitDisjointPathsSignature = QList<QPainterPath> (*)(const QPainterPath &);
    using PrettyFormatRealSignature = QString (*)(qreal);
    using MaxDimensionPortionSignature = qreal (*)(const QRectF &, qreal, qreal);

    ASSERT_KRITA_UTILITY_SIGNATURE(optimalPatchSize, OptimalPatchSizeSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(splitRectIntoPatches, SplitRectSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(splitRectIntoPatchesTight, SplitRectSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(splitRegionIntoPatches, SplitQRegionSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(splitRegionIntoPatches, SplitKisRegionSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(splitTriangles, SplitTrianglesSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(splitPath, SplitPathSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(splitDisjointPaths, SplitDisjointPathsSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(prettyFormatReal, PrettyFormatRealSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(maxDimensionPortion, MaxDimensionPortionSignature);

    QVERIFY(true);
}

void KritaImageUtilitySchemaContractTest::opacityChannelNodeAndPaintingSchemaRemainsStable()
{
    using MergeOpacityU8Signature = quint8 (*)(quint8, quint8);
    using MergeOpacityFSignature = qreal (*)(qreal, qreal);
    using MergeChannelFlagsSignature = QBitArray (*)(const QBitArray &, const QBitArray &);
    using CompareChannelFlagsSignature = bool (*)(QBitArray, QBitArray);
    using LocalizedOnOffSignature = QString (*)(bool);
    using NearestNodeSignature = KisNodeSP (*)(KisNodeSP);
    using RenderExactRectSignature = void (*)(QPainter *, const QRect &);
    using RenderExactRectWithPenSignature = void (*)(QPainter *, const QRect &, const QPen &);
    using ConvertToGraySignature = QImage (*)(const QImage &);

    ASSERT_KRITA_UTILITY_SIGNATURE(mergeOpacityU8, MergeOpacityU8Signature);
    ASSERT_KRITA_UTILITY_SIGNATURE(mergeOpacityF, MergeOpacityFSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(mergeChannelFlags, MergeChannelFlagsSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(compareChannelFlags, CompareChannelFlagsSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(toLocalizedOnOff, LocalizedOnOffSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(nearestNodeAfterRemoval, NearestNodeSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(renderExactRect, RenderExactRectSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(renderExactRect, RenderExactRectWithPenSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(convertQImageToGrayA, ConvertToGraySignature);

    QVERIFY(true);
}

void KritaImageUtilitySchemaContractTest::deviceAndGeometryMutationSchemaRemainsStable()
{
    using ApplyToAlphaSignature = void (*)(KisPaintDeviceSP, const QRect &, std::function<void(quint8)>);
    using FilterAlphaSignature = void (*)(KisPaintDeviceSP, const QRect &, std::function<quint8(quint8)>);
    using EstimateTransparentSignature = qreal (*)(KisPaintDeviceSP, const QRect &, qreal);
    using MirrorDabPointSignature = void (*)(Qt::Orientation, const QPoint &, KisRenderedDab *, bool);
    using MirrorDabPointFSignature = void (*)(Qt::Orientation, const QPointF &, KisRenderedDab *, bool);
    using MirrorRectPointSignature = void (*)(Qt::Orientation, const QPoint &, QRect *);
    using MirrorRectPointFSignature = void (*)(Qt::Orientation, const QPointF &, QRect *);
    using MirrorPointPointSignature = void (*)(Qt::Orientation, const QPoint &, QPointF *);
    using MirrorPointPointFSignature = void (*)(Qt::Orientation, const QPointF &, QPointF *);
    using BooleanSpaceSignature = QTransform (*)(KisImageSP);
    using CloseTornSubpathsSignature = QPainterPath (*)(QPainterPath);

    ASSERT_KRITA_UTILITY_SIGNATURE(applyToAlpha8Device, ApplyToAlphaSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(filterAlpha8Device, FilterAlphaSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(estimatePortionOfTransparentPixels, EstimateTransparentSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(mirrorDab, MirrorDabPointSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(mirrorDab, MirrorDabPointFSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(mirrorRect, MirrorRectPointSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(mirrorRect, MirrorRectPointFSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(mirrorPoint, MirrorPointPointSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(mirrorPoint, MirrorPointPointFSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(pathShapeBooleanSpaceWorkaround, BooleanSpaceSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(tryCloseTornSubpathsAfterIntersection, CloseTornSubpathsSignature);

    QVERIFY(true);
}

void KritaImageUtilitySchemaContractTest::thresholdSchemaRemainsStable()
{
    using ThresholdSignature = void (*)(KisPaintDeviceSP, const QRect &, KritaUtils::ThresholdMode);

    static_assert(std::is_enum_v<KritaUtils::ThresholdMode>);
    static_assert(KritaUtils::ThresholdNone == 0);
    static_assert(KritaUtils::ThresholdFloor == 1);
    static_assert(KritaUtils::ThresholdCeil == 2);
    static_assert(KritaUtils::ThresholdMaxOut == 3);
    ASSERT_KRITA_UTILITY_SIGNATURE(thresholdOpacity, ThresholdSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(thresholdOpacityAlpha8, ThresholdSignature);

    QVERIFY(true);
}

void KritaImageUtilitySchemaContractTest::rasterizationSchemaRemainsStable()
{
    using RasterLineVisitorSignature = void (*)(const QPoint &, const QPoint &, VisitorProbe);
    using RasterPathVisitorSignature = void (*)(const QVector<QPoint> &, VisitorProbe);
    using RasterLineSignature = QVector<QPoint> (*)(const QPoint &, const QPoint &);
    using RasterPathSignature = QVector<QPoint> (*)(const QVector<QPoint> &);

    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizeHLine<VisitorProbe>, RasterLineVisitorSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizeVLine<VisitorProbe>, RasterLineVisitorSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizeLineDDA<VisitorProbe>, RasterLineVisitorSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizePolylineDDA<VisitorProbe>, RasterPathVisitorSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizePolygonDDA<VisitorProbe>, RasterPathVisitorSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizeHLine, RasterLineSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizeVLine, RasterLineSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizeLineDDA, RasterLineSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizePolylineDDA, RasterPathSignature);
    ASSERT_KRITA_UTILITY_SIGNATURE(rasterizePolygonDDA, RasterPathSignature);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KritaImageUtilitySchemaContractTest)

#include "KritaImageUtilitySchemaContractTest.moc"
