/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisMplTest.h"

#include <optional>
#include <QDebug>

#include <tuple>
#include <type_traits>
#include <variant>

#include <KisMpl.h>
#include <kis_shared.h>
#include <kis_shared_ptr.h>

void KisMplTest::testTupleAndTypeUtilities()
{
    static_assert(std::is_same_v<kismpl::make_index_sequence_from_1<4>, std::index_sequence<1, 2, 3, 4>>);
    static_assert(std::is_same_v<kismpl::detail::make_index_sequence_from_1_impl<0>::type, std::index_sequence<>>);
    static_assert(std::is_same_v<kismpl::detail::first_type_impl<int, qreal>::type, int>);
    static_assert(std::is_same_v<kismpl::first_type<int, qreal>::type, int>);
    static_assert(std::is_same_v<kismpl::first_type_t<int, qreal>, int>);

    const auto doubled = kismpl::apply_to_tuple([](int value) {
        return 2 * value;
    }, std::tuple{1, 3, 5});
    QCOMPARE(doubled, std::make_tuple(2, 6, 10));

    const auto incremented = kismpl::detail::apply_to_tuple_impl([](int value) {
        return value + 1;
    }, std::tuple{2, 4}, std::index_sequence<0, 1>{});
    QCOMPARE(incremented, std::make_tuple(3, 5));

    const auto addTuple = kismpl::unzip_wrapper([](int lhs, int rhs) {
        return lhs + rhs;
    });
    QCOMPARE(addTuple(std::tuple{7, 11}), 18);

    std::variant<int, QString> value = 13;
    const auto visitor = kismpl::overloaded{
        [](int number) {
            return QString::number(number);
        },
        [](const QString &text) {
            return text;
        }};
    QCOMPARE(std::visit(visitor, value), QStringLiteral("13"));
    value = QStringLiteral("text");
    QCOMPARE(std::visit(visitor, value), QStringLiteral("text"));
}


void KisMplTest::testFoldOptional()
{
    std::optional<int> a(0x1);
    std::optional<int> b(0x2);
    std::optional<int> c(0x4);
    std::optional<int> d;
    std::optional<int> e;

    QCOMPARE(kismpl::fold_optional(std::plus{}, a, b, c, d, e), std::optional<int>(7));
    QCOMPARE(kismpl::fold_optional(std::plus{}, e, a, b, c, d), std::optional<int>(7));
    QCOMPARE(kismpl::fold_optional(std::plus{}, d, e, a, b, c), std::optional<int>(7));
    QCOMPARE(kismpl::fold_optional(std::plus{}, c, d, e, a, b), std::optional<int>(7));
    QCOMPARE(kismpl::fold_optional(std::plus{}, b, c, d, e, a), std::optional<int>(7));

    QCOMPARE(kismpl::fold_optional(std::plus{}, b, c, d, e), std::optional<int>(6));
    QCOMPARE(kismpl::fold_optional(std::plus{}, c, d, e), std::optional<int>(4));
    QCOMPARE(kismpl::fold_optional(std::plus{}, d, e), std::optional<int>());
    QCOMPARE(kismpl::fold_optional(std::plus{}, e), std::optional<int>());

}

namespace {
struct Struct {
    Struct(int _id) : id(_id) {}

    int id = -1;
    int idFunc() {
        return id;
    }
    int idConstFunc() const {
        return id;
    }

    int idNoexceptFunc() noexcept {
        return id;
    }

    int idConstNoexceptFunc() const noexcept {
        return id;
    }

    int overloaded() const {
        return id;
    }

    int overloaded() {
        return id;
    }

};

struct StructExplicit {
    explicit StructExplicit (int _id) : id(_id) {}

    int id = -1;
    int idFunc() {
        return id;
    }
    int idConstFunc() const {
        return id;
    }
    int idNoexceptFunc() noexcept {
        return id;
    }
    int idConstNoexceptFunc() const noexcept {
        return id;
    }
};

struct StructWithShared : KisShared {
    StructWithShared (int _id) : id(_id) {}

    int id = -1;
    int idConstFunc() const {
        return id;
    }
};

}

void KisMplTest::testMemberOperatorsEqualTo()
{
    int v = 1;
    int &vref = v;
    const int &vconstref = v;


    std::vector<Struct> vec({{0},{1},{2},{3}});

    ////////////////////////////////////////
    // compare member variable against value

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::id, v));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare member variable against reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::id, vref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare member variable against const reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::id, vconstref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    ////////////////////////////////////////
    // compare member function against value

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::idConstFunc, v));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare member function against reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::id, vref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare member function against const reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::id, vconstref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    ////////////////////////////////////////
    // compare overloaded member function against value

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(qConstOverload<>(&Struct::overloaded), v));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare member function against reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(qConstOverload<>(&Struct::overloaded), vref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare member function against const reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(qConstOverload<>(&Struct::overloaded), vconstref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare non-const member function against const reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::idFunc, vconstref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare non-const noexcept member function against const reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::idNoexceptFunc, vconstref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }

    // compare const noexcept member function against const reference

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::idConstNoexceptFunc, vconstref));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }
}

void KisMplTest::testMemberOperatorsEqualToPointer()
{
    std::vector<Struct> vec_base({{0},{1},{2},{3},{4}});
    std::vector<Struct*> vec({&vec_base[0], &vec_base[1], &vec_base[2], &vec_base[3], &vec_base[3]});

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::id, 1));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }
}

void KisMplTest::testMemberOperatorsEqualToStdSharedPtr()
{
    std::vector<std::shared_ptr<Struct>> vec({std::make_shared<Struct>(0),
                                              std::make_shared<Struct>(1),
                                              std::make_shared<Struct>(2),
                                              std::make_shared<Struct>(3),
                                              std::make_shared<Struct>(4)});


    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::id, 1));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }
}

void KisMplTest::testMemberOperatorsEqualToQSharedPointer()
{
    std::vector<QSharedPointer<Struct>> vec({QSharedPointer<Struct>::create(0),
                                             QSharedPointer<Struct>::create(1),
                                             QSharedPointer<Struct>::create(2),
                                             QSharedPointer<Struct>::create(3),
                                             QSharedPointer<Struct>::create(4)});

    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&Struct::id, 1));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }
}

void KisMplTest::testMemberOperatorsEqualToKisSharedPtr()
{
    std::vector<KisSharedPtr<StructWithShared>> vec({new StructWithShared(0),
                                                     new StructWithShared(1),
                                                     new StructWithShared(2),
                                                     new StructWithShared(3),
                                                     new StructWithShared(4)});
    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&StructWithShared::id, 1));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }
}

void KisMplTest::testMemberOperatorsEqualToKisSharedPtrFunction()
{
    std::vector<KisSharedPtr<StructWithShared>> vec({new StructWithShared(0),
                                                     new StructWithShared(1),
                                                     new StructWithShared(2),
                                                     new StructWithShared(3),
                                                     new StructWithShared(4)});
    {
        auto it = std::find_if(vec.begin(), vec.end(), kismpl::mem_equal_to(&StructWithShared::idConstFunc, 1));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 1);
    }
}

void KisMplTest::testMemberOperatorsLess()
{
    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::id));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::idConstFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::idFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::idNoexceptFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::idConstNoexceptFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec_base({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});
        std::vector<StructExplicit*> vec({&vec_base[0], &vec_base[1], &vec_base[2], &vec_base[3], &vec_base[3]});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::id));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec_base({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});
        std::vector<StructExplicit*> vec({&vec_base[0], &vec_base[1], &vec_base[2], &vec_base[3], &vec_base[3]});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::idConstFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<std::shared_ptr<StructExplicit>> vec({std::make_shared<StructExplicit>(0),
                                                          std::make_shared<StructExplicit>(1),
                                                          std::make_shared<StructExplicit>(2),
                                                          std::make_shared<StructExplicit>(3),
                                                          std::make_shared<StructExplicit>(4)});


        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::id));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<std::shared_ptr<StructExplicit>> vec({std::make_shared<StructExplicit>(0),
                                                          std::make_shared<StructExplicit>(1),
                                                          std::make_shared<StructExplicit>(2),
                                                          std::make_shared<StructExplicit>(3),
                                                          std::make_shared<StructExplicit>(4)});


        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less(&StructExplicit::idConstFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }
}

void KisMplTest::testMemberOperatorsLessEqual()
{
    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less_equal(&StructExplicit::id));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less_equal(&StructExplicit::idNoexceptFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less_equal(&StructExplicit::idFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less_equal(&StructExplicit::idConstFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(0),StructExplicit(1),StructExplicit(2),StructExplicit(3),StructExplicit(4)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_less_equal(&StructExplicit::idConstNoexceptFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }
}

void KisMplTest::testMemberOperatorsGreater()
{
    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater(&StructExplicit::id));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater(&StructExplicit::idConstFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater(&StructExplicit::idFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater(&StructExplicit::idConstNoexceptFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater(&StructExplicit::idNoexceptFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 2);
    }
}

void KisMplTest::testMemberOperatorsGreaterEqual()
{
    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater_equal(&StructExplicit::id));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater_equal(&StructExplicit::idConstFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater_equal(&StructExplicit::idFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater_equal(&StructExplicit::idConstNoexceptFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }

    {
        std::vector<StructExplicit> vec({StructExplicit(4),StructExplicit(3),StructExplicit(2),StructExplicit(1),StructExplicit(0)});

        auto it = std::lower_bound(vec.begin(), vec.end(), 2, kismpl::mem_greater_equal(&StructExplicit::idNoexceptFunc));
        QVERIFY(it != vec.end());
        QCOMPARE(std::distance(vec.begin(), it), 3);
    }
}

void KisMplTest::testMemberBinaryComparators()
{
    Struct lower(1);
    Struct higher(2);

    const auto verifyAscending = [&lower, &higher](auto comparator) {
        QVERIFY(comparator(lower, higher));
        QVERIFY(comparator(lower, 2));
        QVERIFY(comparator(1, higher));
    };
    const auto verifyDescending = [&lower, &higher](auto comparator) {
        QVERIFY(comparator(higher, lower));
        QVERIFY(comparator(higher, 1));
        QVERIFY(comparator(2, lower));
    };

    verifyAscending(kismpl::mem_less(&Struct::id));
    verifyAscending(kismpl::mem_less(&Struct::idFunc));
    verifyAscending(kismpl::mem_less(&Struct::idConstFunc));
    verifyAscending(kismpl::mem_less(&Struct::idNoexceptFunc));
    verifyAscending(kismpl::mem_less(&Struct::idConstNoexceptFunc));

    verifyAscending(kismpl::mem_less_equal(&Struct::id));
    verifyAscending(kismpl::mem_less_equal(&Struct::idFunc));
    verifyAscending(kismpl::mem_less_equal(&Struct::idConstFunc));
    verifyAscending(kismpl::mem_less_equal(&Struct::idNoexceptFunc));
    verifyAscending(kismpl::mem_less_equal(&Struct::idConstNoexceptFunc));

    verifyDescending(kismpl::mem_greater(&Struct::id));
    verifyDescending(kismpl::mem_greater(&Struct::idFunc));
    verifyDescending(kismpl::mem_greater(&Struct::idConstFunc));
    verifyDescending(kismpl::mem_greater(&Struct::idNoexceptFunc));
    verifyDescending(kismpl::mem_greater(&Struct::idConstNoexceptFunc));

    verifyDescending(kismpl::mem_greater_equal(&Struct::id));
    verifyDescending(kismpl::mem_greater_equal(&Struct::idFunc));
    verifyDescending(kismpl::mem_greater_equal(&Struct::idConstFunc));
    verifyDescending(kismpl::mem_greater_equal(&Struct::idNoexceptFunc));
    verifyDescending(kismpl::mem_greater_equal(&Struct::idConstNoexceptFunc));
}

namespace kismpl2 {

template<typename Class, typename MemType>
inline auto mem_bit_or(MemType Class::*ptr) {
    return kismpl::detail::mem_compare<std::bit_or<>, Class, MemType, decltype(ptr)>{ptr};
}

template<typename Class, typename MemType>
inline auto mem_bit_or(MemType (Class::*ptr)()) {
    return kismpl::detail::mem_compare<std::bit_or<>, Class, MemType, decltype(ptr)>{ptr};
}

}

void KisMplTest::testMemberOperatorsAccumulate()
{
    struct RectStruct {
        QRect rect;
    };

    std::vector<RectStruct> vec({{QRect(0,0,10,10)},
                            {QRect(0,0,20,10)},
                            {QRect(0,0,10,20)},
                            {QRect(0,0,30,10)},
                            {QRect(0,0,10,30)}});
    {
        const QRect result = std::accumulate(vec.begin(), vec.end(), QRect(), kismpl::mem_bit_or(&RectStruct::rect));
        QCOMPARE(result, QRect(0,0,30,30));
    }

    {
        const QRect result = std::accumulate(vec.begin(), vec.end(), QRect(0,0,100,100), kismpl::mem_bit_and(&RectStruct::rect));
        QCOMPARE(result, QRect(0,0,10,10));
    }
}

void KisMplTest::testMemberOperatorsAccumulateToKisSharedPtr()
{
    std::vector<KisSharedPtr<StructWithShared>> vec({new StructWithShared(1),
                                                     new StructWithShared(2),
                                                     new StructWithShared(3),
                                                     new StructWithShared(4)});
    {
        const int result = std::accumulate(vec.begin(), vec.end(), 0, kismpl::mem_plus(&StructWithShared::idConstFunc));
        QCOMPARE(result, 10);
    }

    {
        const int result = std::accumulate(vec.begin(), vec.end(), 0, kismpl::mem_minus(&StructWithShared::idConstFunc));
        QCOMPARE(result, -10);
    }

    {
        const int result = std::accumulate(vec.begin(), vec.end(), 0, kismpl::mem_multiplies(&StructWithShared::idConstFunc));
        QCOMPARE(result, 0);
    }

    {
        const int result = std::accumulate(vec.begin(), vec.end(), 0, kismpl::mem_divides(&StructWithShared::idConstFunc));
        QCOMPARE(result, 0);
    }

    {
        const int result = std::accumulate(vec.begin(), vec.end(), 0, kismpl::mem_bit_xor(&StructWithShared::idConstFunc));
        QCOMPARE(result, 4);
    }
}

void KisMplTest::testMemberOperatorFactories()
{
    Struct lhs(6);
    Struct rhs(3);

    QCOMPARE(kismpl::mem_bit_or(&Struct::id)(lhs, rhs), 7);
    QCOMPARE(kismpl::mem_bit_or(&Struct::idConstFunc)(lhs, rhs), 7);
    QCOMPARE(kismpl::mem_bit_and(&Struct::id)(lhs, rhs), 2);
    QCOMPARE(kismpl::mem_bit_and(&Struct::idFunc)(lhs, rhs), 2);
    QCOMPARE(kismpl::mem_bit_xor(&Struct::id)(lhs, rhs), 5);
    QCOMPARE(kismpl::mem_bit_xor(&Struct::idFunc)(lhs, rhs), 5);
    QCOMPARE(kismpl::mem_plus(&Struct::id)(lhs, rhs), 9);
    QCOMPARE(kismpl::mem_plus(&Struct::idFunc)(lhs, rhs), 9);
    QCOMPARE(kismpl::mem_minus(&Struct::id)(lhs, rhs), 3);
    QCOMPARE(kismpl::mem_minus(&Struct::idFunc)(lhs, rhs), 3);
    QCOMPARE(kismpl::mem_multiplies(&Struct::id)(lhs, rhs), 18);
    QCOMPARE(kismpl::mem_multiplies(&Struct::idFunc)(lhs, rhs), 18);
    QCOMPARE(kismpl::mem_divides(&Struct::id)(lhs, rhs), 2);
    QCOMPARE(kismpl::mem_divides(&Struct::idFunc)(lhs, rhs), 2);
}

void KisMplTest::testFinally()
{
    bool called = false;
    auto cleanup = [&called] {
        called = true;
    };
    using Guard = kismpl::finally<decltype(cleanup)>;

    static_assert(!std::is_copy_constructible_v<Guard>);
    static_assert(!std::is_move_constructible_v<Guard>);

    {
        Guard guard(std::move(cleanup));
        QVERIFY(!called);
    }
    QVERIFY(called);
}


QTEST_GUILESS_MAIN(KisMplTest)
