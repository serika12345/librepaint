/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "bsplines/kis_nu_bspline_2d.h"

#include "bspline_base.h"

#include <QTest>

#include <cmath>

namespace
{

constexpr float tolerance = 1e-4f;

const QVector<double> xSamples{-2.0, -0.5, 1.5, 4.0};
const QVector<double> ySamples{1.0, 2.5, 5.0, 9.0};

float affineValue(double x, double y)
{
    return static_cast<float>(2.0 * x - 3.0 * y + 7.0);
}

void compareValue(float actual, float expected)
{
    QVERIFY2(std::abs(actual - expected) <= tolerance, "spline value differs");
}

struct RecordingAffineFunction {
    QVector<QPointF> *visitedSamples;

    float operator()(double x, double y) const
    {
        visitedSamples->append(QPointF(x, y));
        return affineValue(x, y);
    }
};

struct AffineFunction {
    float operator()(double x, double y) const
    {
        return affineValue(x, y);
    }
};

struct ConstantFunction {
    float operator()(double, double) const
    {
        return 4.25f;
    }
};

} // namespace

void kis_assert_recoverable(const char *, const char *, int)
{
}

void destroy_multi_UBspline(Bspline *)
{
    qFatal("The non-uniform two-dimensional spline contract reached the multi-spline destroy path");
}

class KisNUBSpline2DContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionOwnsSamplesAndPreservesBoundsAndBoundaryConditions();
    void initializationVisitsTheNonUniformGridInXMajorOrder();
    void affineSurfaceInterpolatesSamplesInteriorAndExactEnd();
    void initializedAndUninitializedLifetimesAreSafe();
};

void KisNUBSpline2DContractTest::constructionOwnsSamplesAndPreservesBoundsAndBoundaryConditions()
{
    QVector<double> mutableXSamples = xSamples;
    QVector<double> mutableYSamples = ySamples;
    KisBSplines::KisNUBSpline2D spline(mutableXSamples, KisBSplines::Natural, mutableYSamples, KisBSplines::Flat);

    mutableXSamples.fill(100.0);
    mutableYSamples.fill(200.0);

    QCOMPARE(spline.topLeft(), QPointF(-2.0, 1.0));
    QCOMPARE(spline.bottomRight(), QPointF(4.0, 9.0));
    QCOMPARE(spline.borderConditionX(), KisBSplines::Natural);
    QCOMPARE(spline.borderConditionY(), KisBSplines::Flat);
}

void KisNUBSpline2DContractTest::initializationVisitsTheNonUniformGridInXMajorOrder()
{
    QVector<double> mutableXSamples = xSamples;
    QVector<double> mutableYSamples = ySamples;
    KisBSplines::KisNUBSpline2D spline(mutableXSamples, KisBSplines::Natural, mutableYSamples, KisBSplines::Natural);
    mutableXSamples.fill(100.0);
    mutableYSamples.fill(200.0);

    QVector<QPointF> visitedSamples;
    spline.initializeSpline(RecordingAffineFunction{&visitedSamples});

    QVector<QPointF> expectedSamples;
    for (double x : xSamples) {
        for (double y : ySamples) {
            expectedSamples.append(QPointF(x, y));
        }
    }
    QCOMPARE(visitedSamples, expectedSamples);
}

void KisNUBSpline2DContractTest::affineSurfaceInterpolatesSamplesInteriorAndExactEnd()
{
    KisBSplines::KisNUBSpline2D spline(xSamples, KisBSplines::Natural, ySamples, KisBSplines::Natural);
    spline.initializeSpline(AffineFunction{});

    const QVector<QPointF> points{
        QPointF(-2.0, 1.0),
        QPointF(-0.5, 2.5),
        QPointF(1.5, 5.0),
        QPointF(-1.25, 1.75),
        QPointF(0.5, 3.25),
        QPointF(3.25, 7.0),
        QPointF(4.0, 9.0),
    };

    for (const QPointF &point : points) {
        const float expected = affineValue(point.x(), point.y());
        compareValue(spline.value(point.x(), point.y()), expected);
        compareValue(spline.value(point.x(), point.y()), expected);
    }
}

void KisNUBSpline2DContractTest::initializedAndUninitializedLifetimesAreSafe()
{
    {
        KisBSplines::KisNUBSpline2D spline(xSamples, KisBSplines::Natural, ySamples, KisBSplines::Natural);
    }

    {
        KisBSplines::KisNUBSpline2D spline(xSamples, KisBSplines::Natural, ySamples, KisBSplines::Natural);
        spline.initializeSpline(ConstantFunction{});
        compareValue(spline.value(0.5f, 3.25f), 4.25f);
    }
}

QTEST_GUILESS_MAIN(KisNUBSpline2DContractTest)

#include "KisNUBSpline2DContractTest.moc"
