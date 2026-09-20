/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "bsplines/kis_bspline_1d.h"

#include "einspline/bspline_base.h"

#include <QTest>

#include <cmath>

namespace
{

constexpr float tolerance = 1e-4f;

struct LinearFunction {
    float operator()(float x) const
    {
        return 2.0f * x - 3.0f;
    }
};

struct ConstantFunction {
    float operator()(float) const
    {
        return 4.25f;
    }
};

void compareValue(float actual, float expected)
{
    QVERIFY2(std::abs(actual - expected) <= tolerance, "spline value differs");
}

} // namespace

void kis_assert_recoverable(const char *, const char *, int)
{
}

void destroy_NUBspline(Bspline *)
{
    qFatal("The one-dimensional uniform spline contract reached the non-uniform destroy path");
}

void destroy_multi_UBspline(Bspline *)
{
    qFatal("The one-dimensional uniform spline contract reached the multi-spline destroy path");
}

class KisBSpline1DContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionKeepsIntervalAndUninitializedLifetimeIsSafe();
    void linearFunctionInterpolatesInteriorAndExactEnd();
    void constantFunctionIsStableAcrossInterval();
};

void KisBSpline1DContractTest::constructionKeepsIntervalAndUninitializedLifetimeIsSafe()
{
    KisBSplines::KisBSpline1D spline(-3.5f, 4.5f, 9, KisBSplines::Natural);

    QCOMPARE(spline.gridStart(), -3.5f);
    QCOMPARE(spline.gridEnd(), 4.5f);
}

void KisBSpline1DContractTest::linearFunctionInterpolatesInteriorAndExactEnd()
{
    KisBSplines::KisBSpline1D spline(-2.0f, 6.0f, 9, KisBSplines::Natural);
    spline.initializeSpline<LinearFunction>();

    const LinearFunction expected;
    for (const float x : {-2.0f, -1.5f, 0.25f, 3.75f, 6.0f}) {
        compareValue(spline.value(x), expected(x));
    }
}

void KisBSpline1DContractTest::constantFunctionIsStableAcrossInterval()
{
    KisBSplines::KisBSpline1D spline(1.0f, 9.0f, 9, KisBSplines::Natural);
    spline.initializeSpline<ConstantFunction>();

    for (const float x : {1.0f, 2.5f, 5.0f, 8.75f, 9.0f}) {
        compareValue(spline.value(x), 4.25f);
    }
}

QTEST_GUILESS_MAIN(KisBSpline1DContractTest)

#include "KisBSpline1DContractTest.moc"
