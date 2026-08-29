/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "svg/SvgMeshPatch.h"

#include <QTest>

#include <array>
#include <cmath>
#include <type_traits>

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

namespace
{
constexpr qreal epsilon = 1e-9;

bool closePoint(const QPointF &actual, const QPointF &expected)
{
    return std::abs(actual.x() - expected.x()) < epsilon && std::abs(actual.y() - expected.y()) < epsilon;
}

bool closeRect(const QRectF &actual, const QRectF &expected)
{
    return closePoint(actual.topLeft(), expected.topLeft()) && closePoint(actual.bottomRight(), expected.bottomRight());
}

SvgMeshPatch squarePatch(qreal side = 4.0)
{
    SvgMeshPatch patch(QPointF(0.0, 0.0));
    patch.addStopLinear({QPointF(0.0, 0.0), QPointF(side, 0.0)}, QColor(Qt::red), SvgMeshPatch::Top);
    patch.addStopLinear({QPointF(side, 0.0), QPointF(side, side)}, QColor(Qt::green), SvgMeshPatch::Right);
    patch.addStopLinear({QPointF(side, side), QPointF(0.0, side)}, QColor(Qt::blue), SvgMeshPatch::Bottom);
    patch.addStopLinear({QPointF(0.0, side), QPointF(0.0, 0.0)}, QColor(Qt::yellow), SvgMeshPatch::Left);
    return patch;
}

struct OwnedPatches {
    ~OwnedPatches()
    {
        for (SvgMeshPatch *patch : values) {
            delete patch;
        }
    }

    QVector<SvgMeshPatch *> values;
};
} // namespace

class SvgMeshPatchContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stopAndTypeStateIsStable();
    void edgeBuildersCreateObservablePathsAndBounds();
    void stringPathsHonorCoordinatesAndIncompleteEndpoints();
    void deCasteljauSamplingAndSplittingKeepContinuity();
    void transformationsAndModificationsDeliverEveryValue();
    void subdivisionKeepsGeometryColorsAndParametricSpace();
    void pointCountReportsCapacityInsteadOfValidStops();
    void copyingSubdivisionResetsParametricCoordinates();
};

void SvgMeshPatchContractTest::stopAndTypeStateIsStable()
{
    static_assert(std::is_same_v<SvgMeshPath, std::array<QPointF, 4>>);
    QCOMPARE(int(SvgMeshPatch::Top), 0);
    QCOMPARE(int(SvgMeshPatch::Right), 1);
    QCOMPARE(int(SvgMeshPatch::Bottom), 2);
    QCOMPARE(int(SvgMeshPatch::Left), 3);
    QCOMPARE(int(SvgMeshPatch::Size), 4);

    const SvgMeshStop emptyStop;
    QVERIFY(!emptyStop.isValid());
    QCOMPARE(emptyStop.point, QPointF());

    const SvgMeshStop stop(QColor(Qt::cyan), QPointF(2.0, 3.0));
    QVERIFY(stop.isValid());
    QCOMPARE(stop.color, QColor(Qt::cyan));
    QCOMPARE(stop.point, QPointF(2.0, 3.0));

    const SvgMeshPatch patch(QPointF(5.0, 7.0));
    for (int edge = SvgMeshPatch::Top; edge < SvgMeshPatch::Size; ++edge) {
        QVERIFY(!patch.getStop(static_cast<SvgMeshPatch::Type>(edge)).isValid());
    }
}

void SvgMeshPatchContractTest::edgeBuildersCreateObservablePathsAndBounds()
{
    SvgMeshPatch direct(QPointF(0.0, 0.0));
    direct.moveTo(QPointF(0.0, 0.0));
    direct.lineTo(QPointF(4.0, 0.0));
    direct.curveTo(QPointF(4.0, 1.0), QPointF(4.0, 3.0), QPointF(4.0, 4.0));
    direct.lineTo(QPointF(0.0, 4.0));
    direct.lineTo(QPointF(0.0, 0.0));

    const SvgMeshPath top = direct.getSegment(SvgMeshPatch::Top);
    QVERIFY(closePoint(top[0], QPointF(0.0, 0.0)));
    QVERIFY(closePoint(top[1], QPointF(4.0 / 3.0, 0.0)));
    QVERIFY(closePoint(top[2], QPointF(8.0 / 3.0, 0.0)));
    QVERIFY(closePoint(top[3], QPointF(4.0, 0.0)));
    QCOMPARE(direct.getSegment(SvgMeshPatch::Right),
             (SvgMeshPath{QPointF(4.0, 0.0), QPointF(4.0, 1.0), QPointF(4.0, 3.0), QPointF(4.0, 4.0)}));

    const QPainterPath path = direct.getPath();
    QCOMPARE(path.elementCount(), 13);
    QCOMPARE(path.currentPosition(), QPointF(0.0, 0.0));
    QVERIFY(closeRect(direct.boundingRect(), QRectF(0.0, 0.0, 4.0, 4.0)));
    QCOMPARE(direct.size(), QSizeF(4.0, 4.0));

    SvgMeshPatch cubic(QPointF(0.0, 0.0));
    const SvgMeshPath cubicTop{QPointF(0.0, 0.0), QPointF(1.0, -1.0), QPointF(3.0, -1.0), QPointF(4.0, 0.0)};
    cubic.addStop(cubicTop, QColor(Qt::magenta), SvgMeshPatch::Top);
    cubic.addStopLinear({QPointF(4.0, 0.0), QPointF(4.0, 4.0)}, QColor(Qt::green), SvgMeshPatch::Right);
    cubic.addStopLinear({QPointF(4.0, 4.0), QPointF(0.0, 4.0)}, QColor(Qt::blue), SvgMeshPatch::Bottom);
    cubic.addStopLinear({QPointF(0.0, 4.0), QPointF(0.0, 0.0)}, QColor(Qt::yellow), SvgMeshPatch::Left);

    QCOMPARE(cubic.getSegment(SvgMeshPatch::Top), cubicTop);
    QCOMPARE(cubic.getStop(SvgMeshPatch::Top).color, QColor(Qt::magenta));
    QCOMPARE(cubic.getStop(SvgMeshPatch::Top).point, cubicTop[0]);
}

void SvgMeshPatchContractTest::stringPathsHonorCoordinatesAndIncompleteEndpoints()
{
    SvgMeshPatch patch(QPointF(0.0, 0.0));
    patch.addStop(QStringLiteral("L 4 0"), QColor(Qt::red), SvgMeshPatch::Top);
    patch.addStop(QStringLiteral("c 0 1 0 3 0 4"), QColor(Qt::green), SvgMeshPatch::Right);
    patch.addStop(QStringLiteral("l -4 0"), QColor(Qt::blue), SvgMeshPatch::Bottom);
    patch.addStop(QStringLiteral("L 0 99"), QColor(Qt::yellow), SvgMeshPatch::Left, true, QPointF(0.0, 0.0));

    QCOMPARE(patch.getStop(SvgMeshPatch::Top).point, QPointF(0.0, 0.0));
    QCOMPARE(patch.getStop(SvgMeshPatch::Right).point, QPointF(4.0, 0.0));
    QCOMPARE(patch.getStop(SvgMeshPatch::Bottom).point, QPointF(4.0, 4.0));
    QCOMPARE(patch.getStop(SvgMeshPatch::Left).point, QPointF(0.0, 4.0));
    QCOMPARE(patch.getSegment(SvgMeshPatch::Right),
             (SvgMeshPath{QPointF(4.0, 0.0), QPointF(4.0, 1.0), QPointF(4.0, 3.0), QPointF(4.0, 4.0)}));
    QCOMPARE(patch.getSegment(SvgMeshPatch::Left)[3], QPointF(0.0, 0.0));
    QVERIFY(closeRect(patch.boundingRect(), QRectF(0.0, 0.0, 4.0, 4.0)));
}

void SvgMeshPatchContractTest::deCasteljauSamplingAndSplittingKeepContinuity()
{
    const SvgMeshPatch patch = squarePatch();

    QCOMPARE(patch.segmentPointAt(SvgMeshPatch::Top, 0.0), QPointF(0.0, 0.0));
    QVERIFY(closePoint(patch.segmentPointAt(SvgMeshPatch::Top, 0.5), QPointF(2.0, 0.0)));
    QCOMPARE(patch.segmentPointAt(SvgMeshPatch::Top, 1.0), QPointF(4.0, 0.0));

    const auto split = patch.segmentSplitAt(SvgMeshPatch::Top, 0.5);
    QCOMPARE(split.first[0], QPointF(0.0, 0.0));
    QVERIFY(closePoint(split.first[3], QPointF(2.0, 0.0)));
    QCOMPARE(split.first[3], split.second[0]);
    QCOMPARE(split.second[3], QPointF(4.0, 0.0));

    QCOMPARE(patch.getMidpointParametric(SvgMeshPatch::Top), QPointF(0.5, 0.0));
    QCOMPARE(patch.getMidpointParametric(SvgMeshPatch::Right), QPointF(1.0, 0.5));
    QCOMPARE(patch.getMidpointParametric(SvgMeshPatch::Bottom), QPointF(0.5, 1.0));
    QCOMPARE(patch.getMidpointParametric(SvgMeshPatch::Left), QPointF(0.0, 0.5));

    const SvgMeshPath vertical = patch.getMidCurve(true);
    QVERIFY(closePoint(vertical.front(), QPointF(2.0, 0.0)));
    QVERIFY(closePoint(vertical.back(), QPointF(2.0, 4.0)));
    const SvgMeshPath horizontal = patch.getMidCurve(false);
    QVERIFY(closePoint(horizontal.front(), QPointF(0.0, 2.0)));
    QVERIFY(closePoint(horizontal.back(), QPointF(4.0, 2.0)));
}

void SvgMeshPatchContractTest::transformationsAndModificationsDeliverEveryValue()
{
    SvgMeshPatch modified = squarePatch();
    const SvgMeshPath newTop{QPointF(1.0, 1.0), QPointF(2.0, 0.0), QPointF(3.0, 0.0), QPointF(5.0, 1.0)};
    modified.modifyPath(SvgMeshPatch::Top, newTop);
    QCOMPARE(modified.getSegment(SvgMeshPatch::Top), newTop);
    QCOMPARE(modified.getStop(SvgMeshPatch::Top).point, newTop[0]);

    const SvgMeshPath topBefore = modified.getSegment(SvgMeshPatch::Top);
    const SvgMeshPath rightBefore = modified.getSegment(SvgMeshPatch::Right);
    const QPointF delta(1.0, 2.0);
    modified.modifyCorner(SvgMeshPatch::Right, delta);
    QCOMPARE(modified.getSegment(SvgMeshPatch::Right)[0], rightBefore[0] - delta);
    QCOMPARE(modified.getSegment(SvgMeshPatch::Right)[1], rightBefore[1] - delta);
    QCOMPARE(modified.getSegment(SvgMeshPatch::Top)[2], topBefore[2] - delta);
    QCOMPARE(modified.getSegment(SvgMeshPatch::Top)[3], topBefore[3] - delta);

    modified.setStopColor(SvgMeshPatch::Right, QColor(Qt::black));
    QCOMPARE(modified.getStop(SvgMeshPatch::Right).color, QColor(Qt::black));

    SvgMeshPatch transformed = squarePatch();
    transformed.setTransform(QTransform::fromTranslate(3.0, -2.0));
    QCOMPARE(transformed.getSegment(SvgMeshPatch::Top)[0], QPointF(3.0, -2.0));
    QCOMPARE(transformed.getSegment(SvgMeshPatch::Bottom)[3], QPointF(3.0, 2.0));
    QCOMPARE(transformed.getStop(SvgMeshPatch::Right).point, QPointF(7.0, -2.0));
    QVERIFY(closeRect(transformed.boundingRect(), QRectF(3.0, -2.0, 4.0, 4.0)));
}

void SvgMeshPatchContractTest::subdivisionKeepsGeometryColorsAndParametricSpace()
{
    const SvgMeshPatch patch = squarePatch();
    const QVector<QColor> colors{QColor(Qt::cyan),
                                 QColor(Qt::magenta),
                                 QColor(Qt::gray),
                                 QColor(Qt::white),
                                 QColor(Qt::black)};

    QVERIFY(patch.isDivisibleHorizontally());
    QVERIFY(patch.isDivisibleVertically());
    const SvgMeshPatch tiny = squarePatch(0.5);
    QVERIFY(!tiny.isDivisibleHorizontally());
    QVERIFY(!tiny.isDivisibleVertically());

    OwnedPatches horizontal;
    patch.subdivideHorizontally(horizontal.values, colors);
    QCOMPARE(horizontal.values.size(), 2);
    QVERIFY(closeRect(horizontal.values[0]->boundingRect(), QRectF(0.0, 0.0, 4.0, 2.0)));
    QVERIFY(closeRect(horizontal.values[1]->boundingRect(), QRectF(0.0, 2.0, 4.0, 2.0)));
    QCOMPARE(horizontal.values[0]->getStop(SvgMeshPatch::Bottom).color, colors[1]);
    QCOMPARE(horizontal.values[1]->getStop(SvgMeshPatch::Top).color, colors[3]);
    QCOMPARE(horizontal.values[0]->getMidpointParametric(SvgMeshPatch::Right), QPointF(1.0, 0.25));

    OwnedPatches vertical;
    patch.subdivideVertically(vertical.values, colors);
    QCOMPARE(vertical.values.size(), 2);
    QVERIFY(closeRect(vertical.values[0]->boundingRect(), QRectF(0.0, 0.0, 2.0, 4.0)));
    QVERIFY(closeRect(vertical.values[1]->boundingRect(), QRectF(2.0, 0.0, 2.0, 4.0)));
    QCOMPARE(vertical.values[0]->getStop(SvgMeshPatch::Right).color, colors[0]);
    QCOMPARE(vertical.values[1]->getStop(SvgMeshPatch::Left).color, colors[2]);

    OwnedPatches quarters;
    patch.subdivide(quarters.values, colors);
    QCOMPARE(quarters.values.size(), 4);
    const std::array<QRectF, 4> expectedBounds{QRectF(0.0, 0.0, 2.0, 2.0),
                                               QRectF(2.0, 0.0, 2.0, 2.0),
                                               QRectF(0.0, 2.0, 2.0, 2.0),
                                               QRectF(2.0, 2.0, 2.0, 2.0)};
    for (int i = 0; i < quarters.values.size(); ++i) {
        QVERIFY(closeRect(quarters.values[i]->boundingRect(), expectedBounds[std::size_t(i)]));
    }
    QCOMPARE(quarters.values[0]->getStop(SvgMeshPatch::Top).color, QColor(Qt::red));
    QCOMPARE(quarters.values[0]->getStop(SvgMeshPatch::Right).color, colors[0]);
    QCOMPARE(quarters.values[0]->getStop(SvgMeshPatch::Bottom).color, colors[4]);
    QCOMPARE(quarters.values[0]->getStop(SvgMeshPatch::Left).color, colors[3]);
    QCOMPARE(quarters.values[3]->getStop(SvgMeshPatch::Right).color, colors[1]);
    QCOMPARE(quarters.values[3]->getStop(SvgMeshPatch::Bottom).color, QColor(Qt::blue));
    QCOMPARE(quarters.values[0]->getMidpointParametric(SvgMeshPatch::Top), QPointF(0.25, 0.0));
}

void SvgMeshPatchContractTest::pointCountReportsCapacityInsteadOfValidStops()
{
    SvgMeshPatch patch(QPointF(0.0, 0.0));
    QCOMPARE(patch.countPoints(), int(SvgMeshPatch::Size));

    patch.addStopLinear({QPointF(0.0, 0.0), QPointF(4.0, 0.0)}, QColor(Qt::red), SvgMeshPatch::Top);
    QCOMPARE(patch.countPoints(), int(SvgMeshPatch::Size));
    QVERIFY(patch.getStop(SvgMeshPatch::Top).isValid());
    QVERIFY(!patch.getStop(SvgMeshPatch::Right).isValid());
}

void SvgMeshPatchContractTest::copyingSubdivisionResetsParametricCoordinates()
{
    const SvgMeshPatch patch = squarePatch();
    const QVector<QColor> colors{QColor(Qt::cyan),
                                 QColor(Qt::magenta),
                                 QColor(Qt::gray),
                                 QColor(Qt::white),
                                 QColor(Qt::black)};
    OwnedPatches quarters;
    patch.subdivide(quarters.values, colors);

    const SvgMeshPatch copy(*quarters.values[0]);
    QCOMPARE(copy.getSegment(SvgMeshPatch::Top), quarters.values[0]->getSegment(SvgMeshPatch::Top));
    QCOMPARE(copy.getStop(SvgMeshPatch::Top).color, quarters.values[0]->getStop(SvgMeshPatch::Top).color);
    QCOMPARE(quarters.values[0]->getMidpointParametric(SvgMeshPatch::Top), QPointF(0.25, 0.0));
    QCOMPARE(copy.getMidpointParametric(SvgMeshPatch::Top), QPointF(0.5, 0.0));

    SvgMeshPatch independent(copy);
    independent.setStopColor(SvgMeshPatch::Top, QColor(Qt::darkRed));
    QCOMPARE(copy.getStop(SvgMeshPatch::Top).color, QColor(Qt::red));
    QCOMPARE(independent.getStop(SvgMeshPatch::Top).color, QColor(Qt::darkRed));
}

QTEST_GUILESS_MAIN(SvgMeshPatchContractTest)

#include "SvgMeshPatchContractTest.moc"
