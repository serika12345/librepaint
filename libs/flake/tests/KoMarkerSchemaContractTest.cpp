/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoMarker.h>

#include <QTest>

#include <type_traits>

class KoMarkerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void markerIdentityLifecycleAndCoordinateEnumSchemaRemainsStable();
    void markerCoordinateAndReferenceSignaturesRemainStable();
    void markerOrientationNameAndEqualitySignaturesRemainStable();
    void markerShapeAndGeometrySignaturesRemainStable();
    void markerPaintingAndStrokeSignaturesRemainStable();
};

void KoMarkerSchemaContractTest::markerIdentityLifecycleAndCoordinateEnumSchemaRemainsStable()
{
    static_assert(std::is_class_v<KoMarker>);
    static_assert(std::is_enum_v<KoMarker::MarkerCoordinateSystem>);
    static_assert(KoMarker::StrokeWidth == static_cast<KoMarker::MarkerCoordinateSystem>(0));
    static_assert(KoMarker::UserSpaceOnUse == static_cast<KoMarker::MarkerCoordinateSystem>(1));
    static_assert(std::is_default_constructible_v<KoMarker>);
    static_assert(std::is_copy_constructible_v<KoMarker>);
    static_assert(std::is_destructible_v<KoMarker>);
}

void KoMarkerSchemaContractTest::markerCoordinateAndReferenceSignaturesRemainStable()
{
    using CoordinateGetter = KoMarker::MarkerCoordinateSystem (KoMarker::*)() const;
    using CoordinateSetter = void (KoMarker::*)(KoMarker::MarkerCoordinateSystem);
    using CoordinateParser = KoMarker::MarkerCoordinateSystem (*)(const QString &);
    using CoordinateWriter = QString (*)(KoMarker::MarkerCoordinateSystem);
    using ReferencePointGetter = QPointF (KoMarker::*)() const;
    using ReferencePointSetter = void (KoMarker::*)(const QPointF &);
    using ReferenceSizeGetter = QSizeF (KoMarker::*)() const;
    using ReferenceSizeSetter = void (KoMarker::*)(const QSizeF &);

    static_assert(
        std::is_same_v<decltype(static_cast<CoordinateGetter>(&KoMarker::coordinateSystem)), CoordinateGetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<CoordinateSetter>(&KoMarker::setCoordinateSystem)), CoordinateSetter>);
    static_assert(std::is_same_v<decltype(static_cast<CoordinateParser>(&KoMarker::coordinateSystemFromString)),
                                 CoordinateParser>);
    static_assert(
        std::is_same_v<decltype(static_cast<CoordinateWriter>(&KoMarker::coordinateSystemToString)), CoordinateWriter>);
    static_assert(
        std::is_same_v<decltype(static_cast<ReferencePointGetter>(&KoMarker::referencePoint)), ReferencePointGetter>);
    static_assert(std::is_same_v<decltype(static_cast<ReferencePointSetter>(&KoMarker::setReferencePoint)),
                                 ReferencePointSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<ReferenceSizeGetter>(&KoMarker::referenceSize)), ReferenceSizeGetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<ReferenceSizeSetter>(&KoMarker::setReferenceSize)), ReferenceSizeSetter>);
}

void KoMarkerSchemaContractTest::markerOrientationNameAndEqualitySignaturesRemainStable()
{
    using NameGetter = QString (KoMarker::*)() const;
    using Equality = bool (KoMarker::*)(const KoMarker &) const;
    using AutoOrientationGetter = bool (KoMarker::*)() const;
    using AutoOrientationSetter = void (KoMarker::*)(bool);
    using ExplicitOrientationGetter = qreal (KoMarker::*)() const;
    using ExplicitOrientationSetter = void (KoMarker::*)(qreal);

    static_assert(std::is_same_v<decltype(static_cast<NameGetter>(&KoMarker::name)), NameGetter>);
    static_assert(std::is_same_v<decltype(static_cast<Equality>(&KoMarker::operator==)), Equality>);
    static_assert(std::is_same_v<decltype(static_cast<AutoOrientationGetter>(&KoMarker::hasAutoOrientation)),
                                 AutoOrientationGetter>);
    static_assert(std::is_same_v<decltype(static_cast<AutoOrientationSetter>(&KoMarker::setAutoOrientation)),
                                 AutoOrientationSetter>);
    static_assert(std::is_same_v<decltype(static_cast<ExplicitOrientationGetter>(&KoMarker::explicitOrientation)),
                                 ExplicitOrientationGetter>);
    static_assert(std::is_same_v<decltype(static_cast<ExplicitOrientationSetter>(&KoMarker::setExplicitOrientation)),
                                 ExplicitOrientationSetter>);
}

void KoMarkerSchemaContractTest::markerShapeAndGeometrySignaturesRemainStable()
{
    using ShapesSetter = void (KoMarker::*)(const QList<KoShape *> &);
    using ShapesGetter = QList<KoShape *> (KoMarker::*)() const;
    using MaxInsetGetter = qreal (KoMarker::*)(qreal) const;
    using BoundingRectGetter = QRectF (KoMarker::*)(qreal, qreal) const;
    using OutlineGetter = QPainterPath (KoMarker::*)(qreal, qreal) const;

    static_assert(std::is_same_v<decltype(static_cast<ShapesSetter>(&KoMarker::setShapes)), ShapesSetter>);
    static_assert(std::is_same_v<decltype(static_cast<ShapesGetter>(&KoMarker::shapes)), ShapesGetter>);
    static_assert(std::is_same_v<decltype(static_cast<MaxInsetGetter>(&KoMarker::maxInset)), MaxInsetGetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<BoundingRectGetter>(&KoMarker::boundingRect)), BoundingRectGetter>);
    static_assert(std::is_same_v<decltype(static_cast<OutlineGetter>(&KoMarker::outline)), OutlineGetter>);
}

void KoMarkerSchemaContractTest::markerPaintingAndStrokeSignaturesRemainStable()
{
    using PaintAtPosition = void (KoMarker::*)(QPainter *, const QPointF &, qreal, qreal);
    using DrawPreview = void (KoMarker::*)(QPainter *, const QRectF &, const QPen &, KoFlake::MarkerPosition);
    using ApplyShapeStroke = void (KoMarker::*)(const KoShape *, KoShapeStroke *, const QPointF &, qreal, qreal);

    static_assert(std::is_same_v<decltype(static_cast<PaintAtPosition>(&KoMarker::paintAtPosition)), PaintAtPosition>);
    static_assert(std::is_same_v<decltype(static_cast<DrawPreview>(&KoMarker::drawPreview)), DrawPreview>);
    static_assert(
        std::is_same_v<decltype(static_cast<ApplyShapeStroke>(&KoMarker::applyShapeStroke)), ApplyShapeStroke>);
}

QTEST_GUILESS_MAIN(KoMarkerSchemaContractTest)

#include "KoMarkerSchemaContractTest.moc"
