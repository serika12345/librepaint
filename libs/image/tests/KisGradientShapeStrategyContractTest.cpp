/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_gradient_shape_strategy.h"

#include <QTest>

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

} // namespace

class KisGradientShapeStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultConstructsZeroEndpoints();
    void constructorPreservesEndpoints();
    void valueAtDispatchesCoordinatesAndResult();
    void baseOwnershipDestroysDerivedStrategy();
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

QTEST_GUILESS_MAIN(KisGradientShapeStrategyContractTest)

#include "KisGradientShapeStrategyContractTest.moc"
