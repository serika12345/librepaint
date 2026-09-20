/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLazyStorageTest.h"

#include "KisLazyStorage.h"

#include <QTest>

#include <type_traits>

namespace
{
struct TrackedValue {
    explicit TrackedValue(int value)
        : value(value)
    {
        ++constructionCount;
    }

    ~TrackedValue()
    {
        ++destructionCount;
    }

    int doubled() const
    {
        return value * 2;
    }

    static int constructionCount;
    static int destructionCount;
    int value;
};

int TrackedValue::constructionCount = 0;
int TrackedValue::destructionCount = 0;
} // namespace

void KisLazyStorageTest::testLazyConstructionAndLifetime()
{
    TrackedValue::constructionCount = 0;
    TrackedValue::destructionCount = 0;

    {
        KisLazyStorage<TrackedValue, int> storage(21);
        QCOMPARE(TrackedValue::constructionCount, 0);

        QCOMPARE(storage->doubled(), 42);
        QCOMPARE(TrackedValue::constructionCount, 1);
        QCOMPARE((*storage).value, 21);
        QCOMPARE(TrackedValue::constructionCount, 1);
    }

    QCOMPARE(TrackedValue::destructionCount, 1);
}

void KisLazyStorageTest::testImmediateConstruction()
{
    KisLazyStorage<float, float> storage(KisLazyStorage<float, float>::init_value_tag(), 42.0f);
    QCOMPARE(*storage, 42.0f);
}

void KisLazyStorageTest::testMove()
{
    KisLazyStorage<int, int> source(13);
    QCOMPARE(*source, 13);

    KisLazyStorage<int, int> moved(std::move(source));
    QCOMPARE(*moved, 13);

    KisLazyStorage<int, int> assigned(42);
    QCOMPARE(*assigned, 42);
    assigned = std::move(moved);
    QCOMPARE(*assigned, 13);
}

void KisLazyStorageTest::testCopyDisabled()
{
    using Storage = KisLazyStorage<int, int>;

    QVERIFY(!std::is_copy_constructible_v<Storage>);
    QVERIFY(!std::is_copy_assignable_v<Storage>);
}

QTEST_GUILESS_MAIN(KisLazyStorageTest)
