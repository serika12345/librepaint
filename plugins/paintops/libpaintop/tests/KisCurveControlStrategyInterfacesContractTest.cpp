/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCurveOptionInputControlsStrategyInterface.h>
#include <KisCurveOptionRangeControlsStrategyInterface.h>

#include <QTest>

namespace
{
class InputControlsStrategy final : public KisCurveOptionInputControlsStrategyInterface
{
public:
    ~InputControlsStrategy() override
    {
        *destroyed = true;
    }

    bool *destroyed {nullptr};
};

class RangeControlsStrategy final : public KisCurveOptionRangeControlsStrategyInterface
{
public:
    ~RangeControlsStrategy() override
    {
        *destroyed = true;
    }

    bool *destroyed {nullptr};
};
}

class KisCurveControlStrategyInterfacesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createsInputControlsWithForwardedArguments();
    void createsRangeControlsWithForwardedArguments();
};

void KisCurveControlStrategyInterfacesContractTest::createsInputControlsWithForwardedArguments()
{
    auto *range = reinterpret_cast<KisCurveRangeModelInterface *>(quintptr(1));
    auto *curve = reinterpret_cast<KisCurveWidget *>(quintptr(2));
    auto *input = reinterpret_cast<QWidget *>(quintptr(3));
    auto *output = reinterpret_cast<QWidget *>(quintptr(4));
    KisCurveRangeModelInterface *receivedRange = nullptr;
    KisCurveWidget *receivedCurve = nullptr;
    QWidget *receivedInput = nullptr;
    QWidget *receivedOutput = nullptr;
    bool destroyed = false;

    KisCurveOptionInputControlsStrategyFactory factory =
        [&](KisCurveRangeModelInterface *rangeArgument,
            KisCurveWidget *curveArgument,
            QWidget *inputArgument,
            QWidget *outputArgument) {
            receivedRange = rangeArgument;
            receivedCurve = curveArgument;
            receivedInput = inputArgument;
            receivedOutput = outputArgument;
            auto *strategy = new InputControlsStrategy;
            strategy->destroyed = &destroyed;
            return strategy;
        };

    KisCurveOptionInputControlsStrategyInterface *strategy =
        factory(range, curve, input, output);

    QCOMPARE(receivedRange, range);
    QCOMPARE(receivedCurve, curve);
    QCOMPARE(receivedInput, input);
    QCOMPARE(receivedOutput, output);
    delete strategy;
    QVERIFY(destroyed);
}

void KisCurveControlStrategyInterfacesContractTest::createsRangeControlsWithForwardedArguments()
{
    auto *range = reinterpret_cast<KisCurveRangeModelInterface *>(quintptr(5));
    auto *placeholder = reinterpret_cast<QWidget *>(quintptr(6));
    KisCurveRangeModelInterface *receivedRange = nullptr;
    QWidget *receivedPlaceholder = nullptr;
    bool destroyed = false;

    KisCurveOptionRangeControlsStrategyFactory factory =
        [&](KisCurveRangeModelInterface *rangeArgument, QWidget *placeholderArgument) {
            receivedRange = rangeArgument;
            receivedPlaceholder = placeholderArgument;
            auto *strategy = new RangeControlsStrategy;
            strategy->destroyed = &destroyed;
            return strategy;
        };

    KisCurveOptionRangeControlsStrategyInterface *strategy =
        factory(range, placeholder);

    QCOMPARE(receivedRange, range);
    QCOMPARE(receivedPlaceholder, placeholder);
    delete strategy;
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KisCurveControlStrategyInterfacesContractTest)

#include "KisCurveControlStrategyInterfacesContractTest.moc"
