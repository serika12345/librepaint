/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_perspective_math.h"

#include <QRect>
#include <QTest>

#include <type_traits>

namespace
{

constexpr qreal pointTolerance = 1e-8;

bool pointsAreClose(const QPointF &actual, const QPointF &expected)
{
    return qAbs(actual.x() - expected.x()) <= pointTolerance && qAbs(actual.y() - expected.y()) <= pointTolerance;
}

void comparePoint(const QPointF &actual, const QPointF &expected)
{
    const QByteArray diagnostic = QByteArrayLiteral("actual=(%1, %2), expected=(%3, %4)")
                                      .replace("%1", QByteArray::number(actual.x(), 'g', 17))
                                      .replace("%2", QByteArray::number(actual.y(), 'g', 17))
                                      .replace("%3", QByteArray::number(expected.x(), 'g', 17))
                                      .replace("%4", QByteArray::number(expected.y(), 'g', 17));
    QVERIFY2(pointsAreClose(actual, expected), diagnostic.constData());
}

} // namespace

class KisPerspectiveMathContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aliasesDescribeProjectiveStorage();
    void matProductAppliesAffineAndProjectiveTerms();
    void mapsNondegenerateQuadrilateralCorners();
    void mapsRectangleAndQuadrilateralInBothDirections();
};

void KisPerspectiveMathContractTest::aliasesDescribeProjectiveStorage()
{
    static_assert(std::is_same_v<typename Matrix3qreal::Scalar, qreal>);
    static_assert(Matrix3qreal::RowsAtCompileTime == 3);
    static_assert(Matrix3qreal::ColsAtCompileTime == 3);
    static_assert(std::is_same_v<typename Matrix9qreal::Scalar, qreal>);
    static_assert(Matrix9qreal::RowsAtCompileTime == 9);
    static_assert(Matrix9qreal::ColsAtCompileTime == 9);
    static_assert(std::is_same_v<typename Vector9qreal::Scalar, qreal>);
    static_assert(Vector9qreal::RowsAtCompileTime == 9);
    static_assert(Vector9qreal::ColsAtCompileTime == 1);
    static_assert(std::is_same_v<typename LineEquation::Scalar, qreal>);
    static_assert(LineEquation::AmbientDimAtCompileTime == 2);
    static_assert(std::is_class_v<KisPerspectiveMath>);
    static_assert(!std::is_default_constructible_v<KisPerspectiveMath>);

    QVERIFY(true);
}

void KisPerspectiveMathContractTest::matProductAppliesAffineAndProjectiveTerms()
{
    const QPointF point(4.0, 2.0);

    comparePoint(KisPerspectiveMath::matProd(Matrix3qreal::Identity(), point), point);

    Matrix3qreal translation = Matrix3qreal::Identity();
    translation(0, 2) = 7.5;
    translation(1, 2) = -3.25;
    comparePoint(KisPerspectiveMath::matProd(translation, point), QPointF(11.5, -1.25));

    Matrix3qreal projective = Matrix3qreal::Identity();
    projective(0, 0) = 2.0;
    projective(0, 1) = 0.5;
    projective(0, 2) = 3.0;
    projective(1, 0) = -0.25;
    projective(1, 1) = 1.5;
    projective(1, 2) = -2.0;
    projective(2, 0) = 0.125;
    projective(2, 1) = 0.25;
    comparePoint(KisPerspectiveMath::matProd(projective, point), QPointF(6.0, 0.0));
}

void KisPerspectiveMathContractTest::mapsNondegenerateQuadrilateralCorners()
{
    const QPointF sourceTopLeft(1.0, 2.0);
    const QPointF sourceTopRight(8.0, 1.0);
    const QPointF sourceBottomLeft(0.0, 7.0);
    const QPointF sourceBottomRight(10.0, 9.0);
    const QPointF targetTopLeft(-3.0, 4.0);
    const QPointF targetTopRight(15.0, 2.0);
    const QPointF targetBottomLeft(-1.0, 16.0);
    const QPointF targetBottomRight(18.0, 20.0);

    const Matrix3qreal mapping = KisPerspectiveMath::computeMatrixTransfo(sourceTopLeft,
                                                                          sourceTopRight,
                                                                          sourceBottomLeft,
                                                                          sourceBottomRight,
                                                                          targetTopLeft,
                                                                          targetTopRight,
                                                                          targetBottomLeft,
                                                                          targetBottomRight);

    comparePoint(KisPerspectiveMath::matProd(mapping, sourceTopLeft), targetTopLeft);
    comparePoint(KisPerspectiveMath::matProd(mapping, sourceTopRight), targetTopRight);
    comparePoint(KisPerspectiveMath::matProd(mapping, sourceBottomLeft), targetBottomLeft);
    comparePoint(KisPerspectiveMath::matProd(mapping, sourceBottomRight), targetBottomRight);
}

void KisPerspectiveMathContractTest::mapsRectangleAndQuadrilateralInBothDirections()
{
    const QRect rectangle(10, 20, 51, 41);
    const QPointF topLeft(-4.0, 2.0);
    const QPointF topRight(73.0, -3.0);
    const QPointF bottomLeft(3.0, 68.0);
    const QPointF bottomRight(80.0, 75.0);

    const Matrix3qreal toRectangle =
        KisPerspectiveMath::computeMatrixTransfoToPerspective(topLeft, topRight, bottomLeft, bottomRight, rectangle);
    const Matrix3qreal fromRectangle =
        KisPerspectiveMath::computeMatrixTransfoFromPerspective(rectangle, topLeft, topRight, bottomLeft, bottomRight);

    comparePoint(KisPerspectiveMath::matProd(toRectangle, topLeft), rectangle.topLeft());
    comparePoint(KisPerspectiveMath::matProd(toRectangle, topRight), rectangle.topRight());
    comparePoint(KisPerspectiveMath::matProd(toRectangle, bottomLeft), rectangle.bottomLeft());
    comparePoint(KisPerspectiveMath::matProd(toRectangle, bottomRight), rectangle.bottomRight());

    comparePoint(KisPerspectiveMath::matProd(fromRectangle, rectangle.topLeft()), topLeft);
    comparePoint(KisPerspectiveMath::matProd(fromRectangle, rectangle.topRight()), topRight);
    comparePoint(KisPerspectiveMath::matProd(fromRectangle, rectangle.bottomLeft()), bottomLeft);
    comparePoint(KisPerspectiveMath::matProd(fromRectangle, rectangle.bottomRight()), bottomRight);

    const QPointF rectangleInterior(33.25, 41.75);
    const QPointF quadrilateralInterior = KisPerspectiveMath::matProd(fromRectangle, rectangleInterior);
    comparePoint(KisPerspectiveMath::matProd(toRectangle, quadrilateralInterior), rectangleInterior);
}

QTEST_GUILESS_MAIN(KisPerspectiveMathContractTest)

#include "KisPerspectiveMathContractTest.moc"
