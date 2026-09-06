/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_algebra_2d.h>

#include <QTest>

#include <type_traits>

class KisAlgebraExtendedSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void coordinateAndShapeTransformationSchemaRemainsStable();
    void vectorPathIntersectionSchemaRemainsStable();
};

void KisAlgebraExtendedSchemaContractTest::coordinateAndShapeTransformationSchemaRemainsStable()
{
    using AdjustIfOnBoundarySignature = void (*)(const QPolygonF &, int, QPointF *);
    using AlignForZoomSignature = QPointF (*)(const QPointF &, qreal);
    using CrossingIndexesSignature = QList<int> (*)(const QLineF &, const QPainterPath &);
    using IntersectCirclesSignature = QVector<QPointF> (*)(const QPointF &, qreal, const QPointF &, qreal);
    using MoveElasticPointSignature =
        QPointF (*)(const QPointF &, const QPointF &, const QPointF &, const QVector<QPointF> &);
    using TransformEllipseSignature = std::pair<QPointF, QTransform> (*)(const QPointF &, const QTransform &);

    static_assert(std::is_same_v<decltype(&KisAlgebra2D::adjustIfOnPolygonBoundary), AdjustIfOnBoundarySignature>);
    static_assert(std::is_same_v<decltype(&KisAlgebra2D::alignForZoom), AlignForZoomSignature>);
    static_assert(std::is_same_v<decltype(&KisAlgebra2D::getLineSegmentCrossingLineIndexes), CrossingIndexesSignature>);
    static_assert(std::is_same_v<decltype(&KisAlgebra2D::intersectTwoCircles), IntersectCirclesSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MoveElasticPointSignature>(&KisAlgebra2D::moveElasticPoint)),
                                 MoveElasticPointSignature>);
    static_assert(std::is_same_v<decltype(&KisAlgebra2D::transformEllipse), TransformEllipseSignature>);

    QVERIFY(true);
}

void KisAlgebraExtendedSchemaContractTest::vectorPathIntersectionSchemaRemainsStable()
{
    using VectorPath = KisAlgebra2D::VectorPath;
    using SegmentSignature = QList<QPointF> (*)(const QLineF &, const VectorPath::Segment &);
    using PointPairSignature =
        QList<QPointF> (*)(const QLineF &, const VectorPath::VectorPathPoint &, const VectorPath::VectorPathPoint &);

    static_assert(std::is_same_v<decltype(static_cast<SegmentSignature>(&VectorPath::intersectSegmentWithLineBounded)),
                                 SegmentSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<PointPairSignature>(&VectorPath::intersectSegmentWithLineBounded)),
                       PointPairSignature>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisAlgebraExtendedSchemaContractTest)

#include "KisAlgebraExtendedSchemaContractTest.moc"
