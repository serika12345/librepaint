/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLager.h"

#include <QTest>

#include <lager/constant.hpp>

#include <functional>
#include <optional>

namespace
{

struct BaseValue {
    int value = 0;
};

struct DerivedValue : BaseValue {
    int extra = 0;
};

enum class Mode {
    First = 1,
    Second = 2,
};

} // namespace

class KisLagerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void foldsPresentOptionalReaders();
    void scaleLensesRoundTripValues();
    void conversionLensesRoundTripValues();
    void baseLensesPreserveDerivedState();
};

void KisLagerContractTest::foldsPresentOptionalReaders()
{
    std::optional<lager::reader<int>> first{lager::reader<int>{lager::make_constant(3)}};
    std::optional<lager::reader<int>> second{lager::reader<int>{lager::make_constant(4)}};
    std::optional<lager::reader<int>> missing;

    const auto sum = kislager::fold_optional_cursors(std::plus<int>{}, first, missing, second);
    QVERIFY(sum);
    QCOMPARE(**sum, 7);

    const auto onlyPresent = kislager::fold_optional_cursors(std::plus<int>{}, missing, first);
    QVERIFY(onlyPresent);
    QCOMPARE(**onlyPresent, 3);

    const auto allMissing = kislager::fold_optional_cursors(std::plus<int>{}, missing, missing);
    QVERIFY(!allMissing);
}

void KisLagerContractTest::scaleLensesRoundTripValues()
{
    const auto scale = kislager::lenses::scale<qreal>(8.0);
    QCOMPARE(lager::view(scale, 1.25), 10.0);
    QCOMPARE(lager::set(scale, 1.25, 20.0), 2.5);

    const auto intToReal = kislager::lenses::scale_int_to_real(0.25);
    QCOMPARE(lager::view(intToReal, 3), 0.75);
    QCOMPARE(lager::set(intToReal, 3, 1.25), 5);

    const auto realToInt = kislager::lenses::scale_real_to_int(4.0);
    QCOMPARE(lager::view(realToInt, 1.25), 5);
    QCOMPARE(lager::set(realToInt, 1.25, 10), 2.5);
}

void KisLagerContractTest::conversionLensesRoundTripValues()
{
    const auto staticCast = kislager::lenses::do_static_cast<Mode, int>;
    QCOMPARE(lager::view(staticCast, Mode::Second), 2);
    QCOMPARE(lager::set(staticCast, Mode::First, 2), Mode::Second);

    const auto variant = kislager::lenses::variant_to<int>;
    QCOMPARE(lager::view(variant, QVariant(13)), 13);
    const QVariant updatedVariant = lager::set(variant, QVariant(QStringLiteral("old")), 42);
    QCOMPARE(updatedVariant.value<int>(), 42);

    const auto logicalNot = kislager::lenses::logical_not();
    QCOMPARE(lager::view(logicalNot, true), false);
    QCOMPARE(lager::set(logicalNot, true, true), false);
}

void KisLagerContractTest::baseLensesPreserveDerivedState()
{
    DerivedValue source;
    source.value = 3;
    source.extra = 7;

    BaseValue replacement;
    replacement.value = 11;

    const auto explicitBase = kislager::lenses::to_base2<DerivedValue, BaseValue>;
    QCOMPARE(lager::view(explicitBase, source).value, 3);
    const DerivedValue explicitResult = lager::set(explicitBase, source, replacement);
    QCOMPARE(explicitResult.value, 11);
    QCOMPARE(explicitResult.extra, 7);

    const auto inferredDerived = kislager::lenses::to_base<BaseValue>;
    QCOMPARE(lager::view(inferredDerived, source).value, 3);
    const DerivedValue inferredResult = lager::set(inferredDerived, source, replacement);
    QCOMPARE(inferredResult.value, 11);
    QCOMPARE(inferredResult.extra, 7);
}

QTEST_GUILESS_MAIN(KisLagerContractTest)

#include "KisLagerContractTest.moc"
