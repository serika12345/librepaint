/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisZug.h"

#include <QTest>

#include <zug/into_vector.hpp>

#include <cmath>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{

template<typename Xform, typename T>
auto transformOne(Xform &&xform, T value)
{
    const auto result = zug::into_vector(std::forward<Xform>(xform), std::vector<T>{value});
    using Result = typename decltype(result)::value_type;
    return Result(result.front());
}

} // namespace

class KisZugContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void castAndMultiplyMappingsTransformEveryValue();
    void comparisonMappingsHonorStrictAndFuzzyBoundaries();
    void roundMappingUsesQtRounding();
    void functorAndTupleMappingsTransformEachInput();
};

void KisZugContractTest::castAndMultiplyMappingsTransformEveryValue()
{
    const auto cast = zug::into_vector(kiszug::map_static_cast<qreal>, std::vector<int>{1, 2, 3});
    QVERIFY(cast == std::vector<qreal>({1.0, 2.0, 3.0}));

    const auto multiplied = zug::into_vector(kiszug::map_multiply<int>(3), std::vector<int>{1, 2, 3});
    QVERIFY(multiplied == std::vector<int>({3, 6, 9}));
}

void KisZugContractTest::comparisonMappingsHonorStrictAndFuzzyBoundaries()
{
    QVERIFY(transformOne(kiszug::map_equal<int>(5), 5));
    QVERIFY(transformOne(kiszug::map_not_equal<int>(5), 4));
    QVERIFY(transformOne(kiszug::map_greater<int>(5), 6));
    QVERIFY(transformOne(kiszug::map_greater_equal<int>(5), 5));
    QVERIFY(transformOne(kiszug::map_less<int>(5), 4));
    QVERIFY(transformOne(kiszug::map_less_equal<int>(5), 5));

    const qreal nearAbove = std::nextafter(1.0, 2.0);
    const qreal nearBelow = std::nextafter(1.0, 0.0);
    QVERIFY(qFuzzyCompare(nearAbove, 1.0));
    QVERIFY(qFuzzyCompare(nearBelow, 1.0));
    QVERIFY(transformOne(kiszug::map_equal<qreal>(1.0), nearAbove));
    QVERIFY(!transformOne(kiszug::map_not_equal<qreal>(1.0), nearBelow));
    QVERIFY(transformOne(kiszug::map_greater_equal<qreal>(1.0), nearBelow));
    QVERIFY(transformOne(kiszug::map_less_equal<qreal>(1.0), nearAbove));
}

void KisZugContractTest::roundMappingUsesQtRounding()
{
    const auto rounded = zug::into_vector(kiszug::map_round, std::vector<qreal>{1.4, 1.5, -1.5});
    QVERIFY(rounded == std::vector<int>({1, 2, -2}));
}

void KisZugContractTest::functorAndTupleMappingsTransformEachInput()
{
    QVERIFY(std::is_empty_v<kiszug::empty_t>);

    const auto timesTwo = kiszug::to_functor(kiszug::map_multiply<int>(2));
    QCOMPARE(timesTwo(3), 6);

    const auto tupleResults = zug::into_vector(kiszug::foreach_tuple(kiszug::map_multiply<int>(2)),
                                               std::vector<std::tuple<int, int>>{{2, 3}});
    QCOMPARE(tupleResults.size(), std::size_t(1));
    QCOMPARE(std::get<0>(tupleResults.front()), 4);
    QCOMPARE(std::get<1>(tupleResults.front()), 6);

    const auto argumentResults =
        zug::into_vector(kiszug::foreach_arg(kiszug::map_multiply<int>(2)), std::vector<int>{2}, std::vector<int>{3});
    QCOMPARE(argumentResults.size(), std::size_t(1));
    QCOMPARE(std::get<0>(argumentResults.front()), 4);
    QCOMPARE(std::get<1>(argumentResults.front()), 6);
}

QTEST_GUILESS_MAIN(KisZugContractTest)

#include "KisZugContractTest.moc"
