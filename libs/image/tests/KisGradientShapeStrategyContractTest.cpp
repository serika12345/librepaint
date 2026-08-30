/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_cached_gradient_shape_strategy.h"
#include "kis_gradient_shape_strategy.h"
#include "kis_polygonal_gradient_shape_strategy.h"

#include "einspline/bspline_base.h"
#include "kis_algebra_2d.h"

#include <QLoggingCategory>
#include <QRect>
#include <QTest>

#include <cmath>
#include <memory>

namespace
{

class GradientShapeStrategyProbe final : public KisGradientShapeStrategy
{
public:
    explicit GradientShapeStrategyProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    GradientShapeStrategyProbe(const QPointF &start, const QPointF &end)
        : KisGradientShapeStrategy(start, end)
    {
    }

    ~GradientShapeStrategyProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    double valueAt(double x, double y) const override
    {
        ++valueCalls;
        receivedX = x;
        receivedY = y;
        return result;
    }

    QPointF start() const
    {
        return m_gradientVectorStart;
    }

    QPointF end() const
    {
        return m_gradientVectorEnd;
    }

    mutable int valueCalls{0};
    mutable double receivedX{0.0};
    mutable double receivedY{0.0};
    double result{0.0};

private:
    int *m_destructionCount{nullptr};
};

class AffineGradientShapeStrategy final : public KisGradientShapeStrategy
{
public:
    explicit AffineGradientShapeStrategy(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~AffineGradientShapeStrategy() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    double valueAt(double x, double y) const override
    {
        ++valueCalls;
        return expectedValue(x, y);
    }

    static double expectedValue(double x, double y)
    {
        return 0.25 * x - 0.5 * y + 3.0;
    }

    mutable int valueCalls{0};

private:
    int *m_destructionCount{nullptr};
};

class PolygonalGradientShapeStrategyProbe final : public KisPolygonalGradientShapeStrategy
{
public:
    PolygonalGradientShapeStrategyProbe(const QPainterPath &path, int *destructionCount)
        : KisPolygonalGradientShapeStrategy(path, 2.0)
        , m_destructionCount(destructionCount)
    {
    }

    ~PolygonalGradientShapeStrategyProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

QPainterPath rectanglePath()
{
    QPainterPath path;
    path.addRect(QRectF(10.0, 20.0, 20.0, 20.0));
    return path;
}

void compareCachedValue(double actual, double expected)
{
    constexpr double tolerance = 1e-4;
    QVERIFY2(std::abs(actual - expected) <= tolerance, "cached gradient value differs");
}

} // namespace

namespace KritaUtils
{

qreal maxDimensionPortion(const QRectF &bounds, qreal portion, qreal minValue)
{
    return qMax(portion * qMax(bounds.width(), bounds.height()), minValue);
}

} // namespace KritaUtils

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

const QLoggingCategory &_41000()
{
    static const QLoggingCategory category("librepaint.test.cached-gradient");
    return category;
}

void destroy_NUBspline(Bspline *)
{
    qFatal("The cached gradient contract reached the non-uniform spline destroy path");
}

void destroy_multi_UBspline(Bspline *)
{
    qFatal("The cached gradient contract reached the multi-spline destroy path");
}

namespace KisAlgebra2D
{

QPointF ensureInRect(QPointF point, const QRectF &bounds)
{
    point.setX(qBound(bounds.left(), point.x(), bounds.right()));
    point.setY(qBound(bounds.top(), point.y(), bounds.bottom()));
    return point;
}

} // namespace KisAlgebra2D

class KisGradientShapeStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultConstructsZeroEndpoints();
    void constructorPreservesEndpoints();
    void valueAtDispatchesCoordinatesAndResult();
    void baseOwnershipDestroysDerivedStrategy();
    void cachedStrategySamplesAndReusesAffineValues();
    void cachedStrategyClampsQueriesAndOwnsBase();
    void polygonalRectangleCalculatesStableCenters();
    void polygonalRectangleHasStableValues();
    void polygonalStrategyOwnsInputPath();
    void polygonalStrategyHasVirtualLifetime();
};

void KisGradientShapeStrategyContractTest::defaultConstructsZeroEndpoints()
{
    const GradientShapeStrategyProbe strategy;

    QCOMPARE(strategy.start(), QPointF(0.0, 0.0));
    QCOMPARE(strategy.end(), QPointF(0.0, 0.0));
}

void KisGradientShapeStrategyContractTest::constructorPreservesEndpoints()
{
    const QPointF start(-12.5, 3.25);
    const QPointF end(8.75, -4.5);
    const GradientShapeStrategyProbe strategy(start, end);

    QCOMPARE(strategy.start(), start);
    QCOMPARE(strategy.end(), end);
}

void KisGradientShapeStrategyContractTest::valueAtDispatchesCoordinatesAndResult()
{
    GradientShapeStrategyProbe strategy;
    strategy.result = -0.625;
    const KisGradientShapeStrategy &base = strategy;

    QCOMPARE(base.valueAt(3.25, -7.5), -0.625);
    QCOMPARE(strategy.valueCalls, 1);
    QCOMPARE(strategy.receivedX, 3.25);
    QCOMPARE(strategy.receivedY, -7.5);
}

void KisGradientShapeStrategyContractTest::baseOwnershipDestroysDerivedStrategy()
{
    int destructions = 0;
    std::unique_ptr<KisGradientShapeStrategy> strategy = std::make_unique<GradientShapeStrategyProbe>(&destructions);

    strategy.reset();

    QCOMPARE(destructions, 1);
}

void KisGradientShapeStrategyContractTest::cachedStrategySamplesAndReusesAffineValues()
{
    auto *baseStrategy = new AffineGradientShapeStrategy;
    const QRect bounds(10, 20, 9, 7);
    const KisCachedGradientShapeStrategy strategy(bounds, 2.0, 2.0, baseStrategy);

    QCOMPARE(baseStrategy->valueCalls, 20);

    compareCachedValue(strategy.valueAt(13.5, 23.25), -5.25);
    compareCachedValue(strategy.valueAt(13.5, 23.25), -5.25);
    QCOMPARE(baseStrategy->valueCalls, 20);
}

void KisGradientShapeStrategyContractTest::cachedStrategyClampsQueriesAndOwnsBase()
{
    int destructions = 0;

    {
        auto *baseStrategy = new AffineGradientShapeStrategy(&destructions);
        const QRect bounds(10, 20, 9, 7);
        const KisCachedGradientShapeStrategy strategy(bounds, 2.0, 2.0, baseStrategy);

        compareCachedValue(strategy.valueAt(0.0, 40.0), AffineGradientShapeStrategy::expectedValue(10.0, 27.0));
        QCOMPARE(baseStrategy->valueCalls, 20);
        QCOMPARE(destructions, 0);
    }

    QCOMPARE(destructions, 1);
}

void KisGradientShapeStrategyContractTest::polygonalRectangleCalculatesStableCenters()
{
    const QPainterPath path = rectanglePath();

    QCOMPARE(KisPolygonalGradientShapeStrategy::testingCalculatePathCenter(4, path, 2.0, true), QPointF(18.0, 28.0));
    QCOMPARE(KisPolygonalGradientShapeStrategy::testingCalculatePathCenter(4, path, 2.0, false), QPointF(14.0, 24.0));
}

void KisGradientShapeStrategyContractTest::polygonalRectangleHasStableValues()
{
    const KisPolygonalGradientShapeStrategy strategy(rectanglePath(), 2.0);

    const double first = strategy.valueAt(15.0, 25.0);
    const double repeated = strategy.valueAt(15.0, 25.0);
    const double reflected = strategy.valueAt(25.0, 35.0);

    QVERIFY(std::isfinite(first));
    QVERIFY(first > 0.0);
    QCOMPARE(repeated, first);
    QVERIFY(std::abs(reflected - first) <= 1e-8);
    QCOMPARE(strategy.valueAt(0.0, 0.0), 0.0);
}

void KisGradientShapeStrategyContractTest::polygonalStrategyOwnsInputPath()
{
    QPainterPath path = rectanglePath();
    const KisPolygonalGradientShapeStrategy strategy(path, 2.0);
    const double originalValue = strategy.valueAt(15.0, 25.0);

    path = QPainterPath();
    path.addRect(QRectF(-100.0, -100.0, 10.0, 10.0));

    QCOMPARE(strategy.valueAt(15.0, 25.0), originalValue);
}

void KisGradientShapeStrategyContractTest::polygonalStrategyHasVirtualLifetime()
{
    int destructions = 0;
    std::unique_ptr<KisGradientShapeStrategy> strategy =
        std::make_unique<PolygonalGradientShapeStrategyProbe>(rectanglePath(), &destructions);

    QVERIFY(strategy->valueAt(15.0, 25.0) > 0.0);
    strategy.reset();

    QCOMPARE(destructions, 1);
}

QTEST_GUILESS_MAIN(KisGradientShapeStrategyContractTest)

#include "KisGradientShapeStrategyContractTest.moc"
