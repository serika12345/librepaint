/*
 * SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_algebra_2d.h"

#include <array>

#include <Eigen/Core>

#include <kis_debug.h>

namespace
{

Eigen::Matrix3d fromQTransformStraight(const QTransform &transform)
{
    Eigen::Matrix3d matrix;

    matrix << transform.m11(), transform.m12(), transform.m13(), transform.m21(), transform.m22(), transform.m23(),
        transform.m31(), transform.m32(), transform.m33();

    return matrix;
}

bool fuzzyMatrixEqual(const QTransform &lhs, const QTransform &rhs, qreal delta)
{
    return qAbs(lhs.m11() - rhs.m11()) < delta && qAbs(lhs.m12() - rhs.m12()) < delta
        && qAbs(lhs.m13() - rhs.m13()) < delta && qAbs(lhs.m21() - rhs.m21()) < delta
        && qAbs(lhs.m22() - rhs.m22()) < delta && qAbs(lhs.m23() - rhs.m23()) < delta
        && qAbs(lhs.m31() - rhs.m31()) < delta && qAbs(lhs.m32() - rhs.m32()) < delta
        && qAbs(lhs.m33() - rhs.m33()) < delta;
}

} // namespace

namespace KisAlgebra2D
{

DecomposedMatrix::DecomposedMatrix() = default;

DecomposedMatrix::DecomposedMatrix(const QTransform &originalTransform)
{
    QTransform transform(originalTransform);
    QTransform projectMatrix;

    if (transform.m33() == 0.0 || originalTransform.determinant() == 0.0) {
        qWarning() << "Cannot decompose matrix!" << transform;
        valid = false;
        return;
    }

    if (transform.type() == QTransform::TxProject) {
        const QTransform affineTransform(transform.m11(),
                                         transform.m12(),
                                         0,
                                         transform.m21(),
                                         transform.m22(),
                                         0,
                                         transform.m31(),
                                         transform.m32(),
                                         1);
        projectMatrix = affineTransform.inverted() * transform;

        transform = affineTransform;
        proj[0] = projectMatrix.m13();
        proj[1] = projectMatrix.m23();
        proj[2] = projectMatrix.m33();
    }

    // QVector3D does not retain enough accuracy for perspective ellipse calculations.
    std::array<Eigen::Vector3d, 3> rows;
    rows[0] = Eigen::Vector3d(transform.m11(), transform.m12(), transform.m13());
    rows[1] = Eigen::Vector3d(transform.m21(), transform.m22(), transform.m23());
    rows[2] = Eigen::Vector3d(transform.m31(), transform.m32(), transform.m33());

    if (!qFuzzyCompare(transform.m33(), 1.0)) {
        const qreal inverseM33 = 1.0 / transform.m33();
        for (auto &row : rows) {
            row *= inverseM33;
        }
    }

    dx = rows[2].x();
    dy = rows[2].y();
    rows[2] = Eigen::Vector3d(0, 0, 1);

    scaleX = rows[0].norm();
    rows[0] *= 1.0 / scaleX;

    shearXY = rows[0].dot(rows[1]);
    rows[1] = rows[1] - shearXY * rows[0];

    scaleY = rows[1].norm();
    rows[1] *= 1.0 / scaleY;
    shearXY *= 1.0 / scaleY;

    const qreal determinant = rows[0].x() * rows[1].y() - rows[0].y() * rows[1].x();
    if (determinant < 0) {
        if (rows[0].x() < rows[1].y()) {
            scaleX = -scaleX;
            rows[0] = -rows[0];
        } else {
            scaleY = -scaleY;
            rows[1] = -rows[1];
        }
        shearXY = -shearXY;
    }

    angle = kisRadiansToDegrees(std::atan2(rows[0].y(), rows[0].x()));

    if (angle != 0.0) {
        const qreal sine = -rows[0].y();
        const qreal cosine = rows[0].x();
        const qreal m11 = rows[0].x();
        const qreal m12 = rows[0].y();
        const qreal m21 = rows[1].x();
        const qreal m22 = rows[1].y();
        rows[0][0] = cosine * m11 + sine * m21;
        rows[0][1] = cosine * m12 + sine * m22;
        rows[1][0] = -sine * m11 + cosine * m21;
        rows[1][1] = -sine * m12 + cosine * m22;
    }

    const QTransform leftOver(rows[0].x(),
                              rows[0].y(),
                              rows[0].z(),
                              rows[1].x(),
                              rows[1].y(),
                              rows[1].z(),
                              rows[2].x(),
                              rows[2].y(),
                              rows[2].z());

    if (!fuzzyMatrixEqual(leftOver, QTransform(), 1e-4)) {
        ENTER_FUNCTION() << "FAILING THE ASSERT BELOW!";
        ENTER_FUNCTION() << ppVar(leftOver);
        ENTER_FUNCTION() << "matrix to decompose was: " << ppVar(originalTransform);
        ENTER_FUNCTION() << ppVar(transform.m33()) << ppVar(originalTransform.determinant());
        const Eigen::Matrix3d identityMatrix = fromQTransformStraight(QTransform());
        const Eigen::Matrix3d leftOverMatrix = fromQTransformStraight(leftOver);
        const Eigen::Matrix3d difference = leftOverMatrix - identityMatrix;
        ENTER_FUNCTION() << difference(0, 0) << difference(0, 1) << difference(0, 2);
        ENTER_FUNCTION() << difference(1, 0) << difference(1, 1) << difference(1, 2);
        ENTER_FUNCTION() << difference(2, 0) << difference(2, 1) << difference(2, 2);
    }

    KIS_SAFE_ASSERT_RECOVER_NOOP(fuzzyMatrixEqual(leftOver, QTransform(), 1e-4));
    KIS_ASSERT(fuzzyMatrixEqual(leftOver, QTransform(), 1e-4));
}

} // namespace KisAlgebra2D
