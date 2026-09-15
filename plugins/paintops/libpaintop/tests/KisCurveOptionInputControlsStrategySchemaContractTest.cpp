/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCurveOptionInputControlsStrategy.h>

#include <QTest>

#include <type_traits>

namespace
{

template<typename SpinBox>
void verifyCurveOptionInputControlsStrategySchema()
{
    using Strategy = KisCurveOptionInputControlsStrategy<SpinBox>;

    static_assert(std::is_class_v<Strategy>);
    static_assert(std::is_base_of_v<KisCurveOptionInputControlsStrategyInterface, Strategy>);
    static_assert(
        std::is_constructible_v<Strategy, KisCurveRangeModelInterface *, KisCurveWidget *, QWidget *, QWidget *>);
    static_assert(std::is_destructible_v<Strategy>);
    static_assert(std::is_same_v<decltype(&Strategy::factory), KisCurveOptionInputControlsStrategyFactory (*)()>);
}

} // namespace

class KisCurveOptionInputControlsStrategySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void curveOptionInputControlsStrategySchemaRemainStable();
};

void KisCurveOptionInputControlsStrategySchemaContractTest::curveOptionInputControlsStrategySchemaRemainStable()
{
    verifyCurveOptionInputControlsStrategySchema<QSpinBox>();
    verifyCurveOptionInputControlsStrategySchema<QDoubleSpinBox>();
    static_assert(std::is_class_v<KisCurveOptionInputControlsStrategyInt>);
    static_assert(std::is_class_v<KisCurveOptionInputControlsStrategyDouble>);
}

QTEST_GUILESS_MAIN(KisCurveOptionInputControlsStrategySchemaContractTest)

#include "KisCurveOptionInputControlsStrategySchemaContractTest.moc"
