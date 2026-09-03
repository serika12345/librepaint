/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoClipMask.h>
#include <KoClipPath.h>
#include <KoPathShape.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KoPathShapeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clipTypeAndLifetimeSchemaRemainStable();
    void clipCoordinateAndRuleSignaturesRemainStable();
    void clipShapeOwnershipSignaturesRemainStable();
    void clipGeometrySignaturesRemainStable();
    void clipRenderingSignaturesRemainStable();
    void pathShapeIdentityListenerAndValueSchemaRemainStable();
    void pathShapeGeometryAndRenderingSignaturesRemainStable();
    void pathShapeConstructionStyleAndMarkerSignaturesRemainStable();
    void pathShapePointQueryAndMutationSignaturesRemainStable();
    void pathShapeSubpathTopologySignaturesRemainStable();
};

void KoPathShapeSchemaContractTest::clipTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoClipMask>);
    static_assert(std::is_default_constructible_v<KoClipMask>);
    static_assert(std::is_copy_constructible_v<KoClipMask>);
    static_assert(std::is_copy_assignable_v<KoClipMask>);
    static_assert(std::is_destructible_v<KoClipMask>);

    static_assert(std::is_class_v<KoClipPath>);
    static_assert(std::is_constructible_v<KoClipPath, QList<KoShape *>, KoFlake::CoordinateSystem>);
    static_assert(std::is_copy_constructible_v<KoClipPath>);
    static_assert(std::is_copy_assignable_v<KoClipPath>);
    static_assert(std::is_destructible_v<KoClipPath>);
}

void KoPathShapeSchemaContractTest::clipCoordinateAndRuleSignaturesRemainStable()
{
    using MaskCoordinates = KoFlake::CoordinateSystem (KoClipMask::*)() const;
    using SetMaskCoordinates = void (KoClipMask::*)(KoFlake::CoordinateSystem);
    using PathCoordinates = KoFlake::CoordinateSystem (KoClipPath::*)() const;
    using SetClipRule = void (KoClipPath::*)(Qt::FillRule);
    using ClipRule = Qt::FillRule (KoClipPath::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<MaskCoordinates>(&KoClipMask::coordinates)), MaskCoordinates>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetMaskCoordinates>(&KoClipMask::setCoordinates)), SetMaskCoordinates>);
    static_assert(
        std::is_same_v<decltype(static_cast<MaskCoordinates>(&KoClipMask::contentCoordinates)), MaskCoordinates>);
    static_assert(std::is_same_v<decltype(static_cast<SetMaskCoordinates>(&KoClipMask::setContentCoordinates)),
                                 SetMaskCoordinates>);
    static_assert(std::is_same_v<decltype(static_cast<PathCoordinates>(&KoClipPath::coordinates)), PathCoordinates>);
    static_assert(std::is_same_v<decltype(static_cast<SetClipRule>(&KoClipPath::setClipRule)), SetClipRule>);
    static_assert(std::is_same_v<decltype(static_cast<ClipRule>(&KoClipPath::clipRule)), ClipRule>);
}

void KoPathShapeSchemaContractTest::clipShapeOwnershipSignaturesRemainStable()
{
    using CloneMask = KoClipMask *(KoClipMask::*)() const;
    using MaskShapes = QList<KoShape *> (KoClipMask::*)() const;
    using SetMaskShapes = void (KoClipMask::*)(const QList<KoShape *> &);
    using MaskIsEmpty = bool (KoClipMask::*)() const;
    using ClonePath = KoClipPath *(KoClipPath::*)() const;
    using PathShapes = QList<KoPathShape *> (KoClipPath::*)() const;
    using Shapes = QList<KoShape *> (KoClipPath::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<CloneMask>(&KoClipMask::clone)), CloneMask>);
    static_assert(std::is_same_v<decltype(static_cast<MaskShapes>(&KoClipMask::shapes)), MaskShapes>);
    static_assert(std::is_same_v<decltype(static_cast<SetMaskShapes>(&KoClipMask::setShapes)), SetMaskShapes>);
    static_assert(std::is_same_v<decltype(static_cast<MaskIsEmpty>(&KoClipMask::isEmpty)), MaskIsEmpty>);
    static_assert(std::is_same_v<decltype(static_cast<ClonePath>(&KoClipPath::clone)), ClonePath>);
    static_assert(std::is_same_v<decltype(static_cast<PathShapes>(&KoClipPath::clipPathShapes)), PathShapes>);
    static_assert(std::is_same_v<decltype(static_cast<Shapes>(&KoClipPath::clipShapes)), Shapes>);
}

void KoPathShapeSchemaContractTest::clipGeometrySignaturesRemainStable()
{
    using MaskRect = QRectF (KoClipMask::*)() const;
    using SetMaskRect = void (KoClipMask::*)(const QRectF &);
    using SetExtraShapeOffset = void (KoClipMask::*)(const QPointF &);
    using Path = QPainterPath (KoClipPath::*)() const;
    using PathForSize = QPainterPath (KoClipPath::*)(const QSizeF &) const;
    using ClipDataTransformation = QTransform (KoClipPath::*)(KoShape *) const;

    static_assert(std::is_same_v<decltype(static_cast<MaskRect>(&KoClipMask::maskRect)), MaskRect>);
    static_assert(std::is_same_v<decltype(static_cast<SetMaskRect>(&KoClipMask::setMaskRect)), SetMaskRect>);
    static_assert(std::is_same_v<decltype(static_cast<SetExtraShapeOffset>(&KoClipMask::setExtraShapeOffset)),
                                 SetExtraShapeOffset>);
    static_assert(std::is_same_v<decltype(static_cast<Path>(&KoClipPath::path)), Path>);
    static_assert(std::is_same_v<decltype(static_cast<PathForSize>(&KoClipPath::pathForSize)), PathForSize>);
    static_assert(std::is_same_v<decltype(static_cast<ClipDataTransformation>(&KoClipPath::clipDataTransformation)),
                                 ClipDataTransformation>);
}

void KoPathShapeSchemaContractTest::clipRenderingSignaturesRemainStable()
{
    using DrawMask = void (KoClipMask::*)(QPainter *, KoShape *);
    using ApplyClipping = void (*)(KoShape *, QPainter &);

    static_assert(std::is_same_v<decltype(static_cast<DrawMask>(&KoClipMask::drawMask)), DrawMask>);
    static_assert(std::is_same_v<decltype(static_cast<ApplyClipping>(&KoClipPath::applyClipping)), ApplyClipping>);
}

void KoPathShapeSchemaContractTest::pathShapeIdentityListenerAndValueSchemaRemainStable()
{
    using Listener = KoPathShape::PointSelectionChangeListener;
    using NotifyPoints = void (Listener::*)(KoPathShape *);
    using NotifyShape = void (Listener::*)(KoShape::ChangeType, KoShape *);
    using RecommendSelection = void (Listener::*)(KoPathShape *, const QList<KoPathPointIndex> &);

    static_assert(std::is_same_v<KoPathPointIndex, QPair<int, int>>);
    static_assert(std::is_same_v<KoSubpath, QList<KoPathPoint *>>);
    static_assert(std::is_same_v<KoSubpathList, QList<KoSubpath *>>);
    static_assert(std::is_class_v<KoPathShape>);
    static_assert(std::is_base_of_v<KoShape, KoPathShape>);
    static_assert(std::is_default_constructible_v<KoPathShape>);
    static_assert(std::is_destructible_v<KoPathShape>);
    static_assert(std::is_class_v<Listener>);
    static_assert(std::is_base_of_v<KoShape::ShapeChangeListener, Listener>);
    static_assert(std::is_abstract_v<Listener>);
    static_assert(
        std::is_same_v<decltype(static_cast<NotifyPoints>(&Listener::notifyPathPointsChanged)), NotifyPoints>);
    static_assert(std::is_same_v<decltype(static_cast<NotifyShape>(&Listener::notifyShapeChanged)), NotifyShape>);
    static_assert(std::is_same_v<decltype(static_cast<RecommendSelection>(&Listener::recommendPointSelectionChange)),
                                 RecommendSelection>);
}

void KoPathShapeSchemaContractTest::pathShapeGeometryAndRenderingSignaturesRemainStable()
{
    using CloneShape = KoShape *(KoPathShape::*)() const;
    using Paint = void (KoPathShape::*)(QPainter &) const;
    using PaintPoints = void (KoPathShape::*)(KisHandlePainterHelper &);
    using OutlineRect = QRectF (KoPathShape::*)() const;
    using Outline = QPainterPath (KoPathShape::*)() const;
    using BoundingRect = QRectF (KoPathShape::*)() const;
    using Size = QSizeF (KoPathShape::*)() const;
    using SetSize = void (KoPathShape::*)(const QSizeF &);
    using HitTest = bool (KoPathShape::*)(const QPointF &) const;
    using PathStroke = QPainterPath (KoPathShape::*)(const QPen &) const;
    using PathShapeId = QString (KoPathShape::*)() const;
    using ToString = QString (KoPathShape::*)(const QTransform &) const;
    using Normalize = QPointF (KoPathShape::*)();

    static_assert(std::is_same_v<decltype(static_cast<CloneShape>(&KoPathShape::cloneShape)), CloneShape>);
    static_assert(std::is_same_v<decltype(static_cast<Paint>(&KoPathShape::paint)), Paint>);
    static_assert(std::is_same_v<decltype(static_cast<PaintPoints>(&KoPathShape::paintPoints)), PaintPoints>);
    static_assert(std::is_same_v<decltype(static_cast<OutlineRect>(&KoPathShape::outlineRect)), OutlineRect>);
    static_assert(std::is_same_v<decltype(static_cast<Outline>(&KoPathShape::outline)), Outline>);
    static_assert(std::is_same_v<decltype(static_cast<BoundingRect>(&KoPathShape::boundingRect)), BoundingRect>);
    static_assert(std::is_same_v<decltype(static_cast<Size>(&KoPathShape::size)), Size>);
    static_assert(std::is_same_v<decltype(static_cast<SetSize>(&KoPathShape::setSize)), SetSize>);
    static_assert(std::is_same_v<decltype(static_cast<HitTest>(&KoPathShape::hitTest)), HitTest>);
    static_assert(std::is_same_v<decltype(static_cast<PathStroke>(&KoPathShape::pathStroke)), PathStroke>);
    static_assert(std::is_same_v<decltype(static_cast<PathShapeId>(&KoPathShape::pathShapeId)), PathShapeId>);
    static_assert(std::is_same_v<decltype(static_cast<ToString>(&KoPathShape::toString)), ToString>);
    static_assert(std::is_same_v<decltype(std::declval<const KoPathShape &>().toString()), QString>);
    static_assert(std::is_same_v<decltype(static_cast<Normalize>(&KoPathShape::normalize)), Normalize>);
}

void KoPathShapeSchemaContractTest::pathShapeConstructionStyleAndMarkerSignaturesRemainStable()
{
    using Clear = void (KoPathShape::*)();
    using AddPoint = KoPathPoint *(KoPathShape::*)(const QPointF &);
    using QuadraticCurve = KoPathPoint *(KoPathShape::*)(const QPointF &, const QPointF &);
    using CubicCurve = KoPathPoint *(KoPathShape::*)(const QPointF &, const QPointF &, const QPointF &);
    using Arc = KoPathPoint *(KoPathShape::*)(qreal, qreal, qreal, qreal);
    using Close = void (KoPathShape::*)();
    using CreateShape = KoPathShape *(*)(const QPainterPath &);
    using Combine = int (KoPathShape::*)(KoPathShape *);
    using Separate = bool (KoPathShape::*)(QList<KoPathShape *> &);
    using FillRule = Qt::FillRule (KoPathShape::*)() const;
    using SetFillRule = void (KoPathShape::*)(Qt::FillRule);
    using Marker = KoMarker *(KoPathShape::*)(KoFlake::MarkerPosition) const;
    using SetMarker = void (KoPathShape::*)(KoMarker *, KoFlake::MarkerPosition);
    using BooleanGetter = bool (KoPathShape::*)() const;
    using BooleanSetter = void (KoPathShape::*)(bool);

    static_assert(std::is_same_v<decltype(static_cast<Clear>(&KoPathShape::clear)), Clear>);
    static_assert(std::is_same_v<decltype(static_cast<AddPoint>(&KoPathShape::moveTo)), AddPoint>);
    static_assert(std::is_same_v<decltype(static_cast<AddPoint>(&KoPathShape::lineTo)), AddPoint>);
    static_assert(std::is_same_v<decltype(static_cast<QuadraticCurve>(&KoPathShape::curveTo)), QuadraticCurve>);
    static_assert(std::is_same_v<decltype(static_cast<CubicCurve>(&KoPathShape::curveTo)), CubicCurve>);
    static_assert(std::is_same_v<decltype(static_cast<Arc>(&KoPathShape::arcTo)), Arc>);
    static_assert(std::is_same_v<decltype(static_cast<Close>(&KoPathShape::close)), Close>);
    static_assert(std::is_same_v<decltype(static_cast<Close>(&KoPathShape::closeMerge)), Close>);
    static_assert(
        std::is_same_v<decltype(static_cast<CreateShape>(&KoPathShape::createShapeFromPainterPath)), CreateShape>);
    static_assert(std::is_same_v<decltype(static_cast<Combine>(&KoPathShape::combine)), Combine>);
    static_assert(std::is_same_v<decltype(static_cast<Separate>(&KoPathShape::separate)), Separate>);
    static_assert(std::is_same_v<decltype(static_cast<FillRule>(&KoPathShape::fillRule)), FillRule>);
    static_assert(std::is_same_v<decltype(static_cast<SetFillRule>(&KoPathShape::setFillRule)), SetFillRule>);
    static_assert(std::is_same_v<decltype(static_cast<Marker>(&KoPathShape::marker)), Marker>);
    static_assert(std::is_same_v<decltype(static_cast<SetMarker>(&KoPathShape::setMarker)), SetMarker>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanGetter>(&KoPathShape::hasMarkers)), BooleanGetter>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanGetter>(&KoPathShape::autoFillMarkers)), BooleanGetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<BooleanSetter>(&KoPathShape::setAutoFillMarkers)), BooleanSetter>);
}

void KoPathShapeSchemaContractTest::pathShapePointQueryAndMutationSignaturesRemainStable()
{
    using PointsAt = QList<KoPathPoint *> (KoPathShape::*)(const QRectF &, bool) const;
    using SegmentsAt = QList<KoPathSegment> (KoPathShape::*)(const QRectF &) const;
    using PathPointIndex = KoPathPointIndex (KoPathShape::*)(const KoPathPoint *) const;
    using PointByIndex = KoPathPoint *(KoPathShape::*)(const KoPathPointIndex &) const;
    using SegmentByIndex = KoPathSegment (KoPathShape::*)(const KoPathPointIndex &) const;
    using SegmentAtPoint = KoPathSegment (KoPathShape::*)(const QPointF &, const QRectF &) const;
    using PointCount = int (KoPathShape::*)() const;
    using InsertPoint = bool (KoPathShape::*)(KoPathPoint *, const KoPathPointIndex &);
    using RemovePoint = KoPathPoint *(KoPathShape::*)(const KoPathPointIndex &);
    using BreakAfter = bool (KoPathShape::*)(const KoPathPointIndex &);
    using RecommendSelection = void (KoPathShape::*)(const QList<KoPathPointIndex> &);
    using LoadNodeTypes = void (KoPathShape::*)(const QString &);
    using NodeTypes = QString (KoPathShape::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<PointsAt>(&KoPathShape::pointsAt)), PointsAt>);
    static_assert(std::is_same_v<decltype(std::declval<const KoPathShape &>().pointsAt(std::declval<const QRectF &>())),
                                 QList<KoPathPoint *>>);
    static_assert(std::is_same_v<decltype(static_cast<SegmentsAt>(&KoPathShape::segmentsAt)), SegmentsAt>);
    static_assert(std::is_same_v<decltype(static_cast<PathPointIndex>(&KoPathShape::pathPointIndex)), PathPointIndex>);
    static_assert(std::is_same_v<decltype(static_cast<PointByIndex>(&KoPathShape::pointByIndex)), PointByIndex>);
    static_assert(std::is_same_v<decltype(static_cast<SegmentByIndex>(&KoPathShape::segmentByIndex)), SegmentByIndex>);
    static_assert(std::is_same_v<decltype(static_cast<SegmentAtPoint>(&KoPathShape::segmentAtPoint)), SegmentAtPoint>);
    static_assert(std::is_same_v<decltype(static_cast<PointCount>(&KoPathShape::pointCount)), PointCount>);
    static_assert(std::is_same_v<decltype(static_cast<InsertPoint>(&KoPathShape::insertPoint)), InsertPoint>);
    static_assert(std::is_same_v<decltype(static_cast<RemovePoint>(&KoPathShape::removePoint)), RemovePoint>);
    static_assert(std::is_same_v<decltype(static_cast<BreakAfter>(&KoPathShape::breakAfter)), BreakAfter>);
    static_assert(std::is_same_v<decltype(static_cast<RecommendSelection>(&KoPathShape::recommendPointSelectionChange)),
                                 RecommendSelection>);
    static_assert(std::is_same_v<decltype(static_cast<LoadNodeTypes>(&KoPathShape::loadNodeTypes)), LoadNodeTypes>);
    static_assert(std::is_same_v<decltype(static_cast<NodeTypes>(&KoPathShape::nodeTypes)), NodeTypes>);
}

void KoPathShapeSchemaContractTest::pathShapeSubpathTopologySignaturesRemainStable()
{
    using SubpathCount = int (KoPathShape::*)() const;
    using SubpathPointCount = int (KoPathShape::*)(int) const;
    using IsClosedSubpath = bool (KoPathShape::*)(int) const;
    using Join = bool (KoPathShape::*)(int);
    using MoveSubpath = bool (KoPathShape::*)(int, int);
    using ChangeOpening = KoPathPointIndex (KoPathShape::*)(const KoPathPointIndex &);
    using ReverseSubpath = bool (KoPathShape::*)(int);
    using RemoveSubpath = KoSubpath *(KoPathShape::*)(int);
    using AddSubpath = bool (KoPathShape::*)(KoSubpath *, int);

    static_assert(std::is_same_v<decltype(static_cast<SubpathCount>(&KoPathShape::subpathCount)), SubpathCount>);
    static_assert(
        std::is_same_v<decltype(static_cast<SubpathPointCount>(&KoPathShape::subpathPointCount)), SubpathPointCount>);
    static_assert(
        std::is_same_v<decltype(static_cast<IsClosedSubpath>(&KoPathShape::isClosedSubpath)), IsClosedSubpath>);
    static_assert(std::is_same_v<decltype(static_cast<Join>(&KoPathShape::join)), Join>);
    static_assert(std::is_same_v<decltype(static_cast<MoveSubpath>(&KoPathShape::moveSubpath)), MoveSubpath>);
    static_assert(std::is_same_v<decltype(static_cast<ChangeOpening>(&KoPathShape::openSubpath)), ChangeOpening>);
    static_assert(std::is_same_v<decltype(static_cast<ChangeOpening>(&KoPathShape::closeSubpath)), ChangeOpening>);
    static_assert(std::is_same_v<decltype(static_cast<ReverseSubpath>(&KoPathShape::reverseSubpath)), ReverseSubpath>);
    static_assert(std::is_same_v<decltype(static_cast<RemoveSubpath>(&KoPathShape::removeSubpath)), RemoveSubpath>);
    static_assert(std::is_same_v<decltype(static_cast<AddSubpath>(&KoPathShape::addSubpath)), AddSubpath>);
}

QTEST_GUILESS_MAIN(KoPathShapeSchemaContractTest)

#include "KoPathShapeSchemaContractTest.moc"
