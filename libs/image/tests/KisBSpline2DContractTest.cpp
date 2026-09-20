/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "bsplines/kis_bspline_2d.h"

#include "einspline/bspline_base.h"

#include <QScopedPointer>
#include <QTest>

#include <cmath>

namespace
{

constexpr float tolerance = 1e-4f;

float affineValue(float x, float y)
{
    return 2.0f * x - 3.0f * y + 7.0f;
}

void compareValue(float actual, float expected)
{
    QVERIFY2(std::abs(actual - expected) <= tolerance, "spline value differs");
}

struct RecordingAffineFunction {
    QVector<QPointF> *visitedSamples;

    float operator()(float x, float y) const
    {
        visitedSamples->append(QPointF(x, y));
        return affineValue(x, y);
    }
};

struct AffineFunction {
    float operator()(float x, float y) const
    {
        return affineValue(x, y);
    }
};

struct SourceSpline {
    mutable QVector<QPointF> visitedSamples;

    QPointF topLeft() const
    {
        return QPointF(-2.0, 1.0);
    }

    QPointF bottomRight() const
    {
        return QPointF(4.0, 9.0);
    }

    KisBSplines::BorderCondition borderConditionX() const
    {
        return KisBSplines::Natural;
    }

    KisBSplines::BorderCondition borderConditionY() const
    {
        return KisBSplines::Flat;
    }

    float value(float x, float y) const
    {
        visitedSamples.append(QPointF(x, y));
        return 4.25f;
    }
};

} // namespace

void kis_assert_recoverable(const char *, const char *, int)
{
    qFatal("The two-dimensional spline contract evaluated outside its configured bounds");
}

void destroy_NUBspline(Bspline *)
{
    qFatal("The two-dimensional uniform spline contract reached the non-uniform destroy path");
}

void destroy_multi_UBspline(Bspline *)
{
    qFatal("The two-dimensional uniform spline contract reached the multi-spline destroy path");
}

class KisBSpline2DContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionPreservesBoundsBoundaryConditionsAndLifetime();
    void initializationSamplesGridInXMajorOrder();
    void affineSurfaceReturnsSamplesInteriorAndExactEnd();
    void convertSplineOpBorrowsAndDispatchesToSource();
    void resamplingPreservesBoundsConditionsAndValues();
};

void KisBSpline2DContractTest::constructionPreservesBoundsBoundaryConditionsAndLifetime()
{
    KisBSplines::KisBSpline2D spline(-2.0f, 4.0f, 4, KisBSplines::Natural, 1.0f, 9.0f, 4, KisBSplines::Flat);

    QCOMPARE(spline.topLeft(), QPointF(-2.0, 1.0));
    QCOMPARE(spline.bottomRight(), QPointF(4.0, 9.0));
    QCOMPARE(spline.borderConditionX(), KisBSplines::Natural);
    QCOMPARE(spline.borderConditionY(), KisBSplines::Flat);
}

void KisBSpline2DContractTest::initializationSamplesGridInXMajorOrder()
{
    KisBSplines::KisBSpline2D spline(-2.0f, 4.0f, 4, KisBSplines::Natural, 1.0f, 9.0f, 4, KisBSplines::Natural);
    QVector<QPointF> visitedSamples;

    spline.initializeSpline(RecordingAffineFunction{&visitedSamples});

    QVector<QPointF> expectedSamples;
    for (const float x : {-2.0f, 0.0f, 2.0f, 4.0f}) {
        for (const float y : {1.0f, 11.0f / 3.0f, 19.0f / 3.0f, 9.0f}) {
            expectedSamples.append(QPointF(x, y));
        }
    }
    QCOMPARE(visitedSamples.size(), 16);
    for (int i = 0; i < visitedSamples.size(); ++i) {
        compareValue(visitedSamples[i].x(), expectedSamples[i].x());
        compareValue(visitedSamples[i].y(), expectedSamples[i].y());
    }
}

void KisBSpline2DContractTest::affineSurfaceReturnsSamplesInteriorAndExactEnd()
{
    KisBSplines::KisBSpline2D spline(-2.0f, 4.0f, 4, KisBSplines::Natural, 1.0f, 9.0f, 4, KisBSplines::Natural);
    spline.initializeSpline(AffineFunction{});

    for (const QPointF &point : {QPointF(-2.0, 1.0), QPointF(0.0, 11.0 / 3.0), QPointF(1.25, 5.5), QPointF(4.0, 9.0)}) {
        const float expected = affineValue(point.x(), point.y());
        compareValue(spline.value(point.x(), point.y()), expected);
        compareValue(spline.value(point.x(), point.y()), expected);
    }
}

void KisBSpline2DContractTest::convertSplineOpBorrowsAndDispatchesToSource()
{
    SourceSpline source;
    KisBSplines::ConvertSplineOp<SourceSpline> operation(source);

    QCOMPARE(&operation.m_spline, &source);
    QCOMPARE(operation(1.25f, 5.5f), 4.25f);
    QCOMPARE(source.visitedSamples, QVector<QPointF>({QPointF(1.25, 5.5)}));
}

void KisBSpline2DContractTest::resamplingPreservesBoundsConditionsAndValues()
{
    SourceSpline source;
    QScopedPointer<KisBSplines::KisBSpline2D> spline(KisBSplines::KisBSpline2D::createResampledSpline(source, 4, 4));

    QCOMPARE(source.visitedSamples.size(), 16);
    QCOMPARE(spline->topLeft(), source.topLeft());
    QCOMPARE(spline->bottomRight(), source.bottomRight());
    QCOMPARE(spline->borderConditionX(), source.borderConditionX());
    QCOMPARE(spline->borderConditionY(), source.borderConditionY());
    compareValue(spline->value(1.25f, 5.5f), 4.25f);
}

QTEST_GUILESS_MAIN(KisBSpline2DContractTest)

#include "KisBSpline2DContractTest.moc"
