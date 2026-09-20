/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLazyValueWrapper.h"

#include <QTest>

#include <memory>
#include <type_traits>

namespace
{

using IntWrapper = KisLazyValueWrapper<int>;

static_assert(std::is_same_v<IntWrapper::value_type, int>);
static_assert(!std::is_copy_constructible_v<IntWrapper>);
static_assert(!std::is_copy_assignable_v<IntWrapper>);

class TrackedValue
{
public:
    explicit TrackedValue(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~TrackedValue()
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

} // namespace

class KisLazyValueWrapperTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndTypeProperties();
    void factoryConstructionConvertsToStoredValue();
    void moveTransfersOwnership();
};

void KisLazyValueWrapperTest::defaultStateAndTypeProperties()
{
    IntWrapper wrapper;

    QVERIFY((std::is_same_v<IntWrapper::value_type, int>));
    QVERIFY(!std::is_copy_constructible_v<IntWrapper>);
    QVERIFY(!std::is_copy_assignable_v<IntWrapper>);
    QCOMPARE(wrapper.value, 0);
}

void KisLazyValueWrapperTest::factoryConstructionConvertsToStoredValue()
{
    int factoryCalls = 0;
    IntWrapper wrapper([&factoryCalls] {
        ++factoryCalls;
        return 42;
    });

    QCOMPARE(factoryCalls, 1);
    QCOMPARE(wrapper.value, 42);

    const int &converted = wrapper;
    QCOMPARE(&converted, &wrapper.value);

    bool destroyed = false;
    {
        KisLazyValueWrapper<std::unique_ptr<TrackedValue>> owned([&destroyed] {
            return std::make_unique<TrackedValue>(&destroyed);
        });
        QVERIFY(owned.value);
    }
    QVERIFY(destroyed);
}

void KisLazyValueWrapperTest::moveTransfersOwnership()
{
    KisLazyValueWrapper<std::unique_ptr<int>> source([] {
        return std::make_unique<int>(13);
    });
    KisLazyValueWrapper<std::unique_ptr<int>> moved(std::move(source));

    QVERIFY(!source.value);
    QCOMPARE(*moved.value, 13);

    KisLazyValueWrapper<std::unique_ptr<int>> assigned;
    assigned = std::move(moved);

    QVERIFY(!moved.value);
    QCOMPARE(*assigned.value, 13);
}

QTEST_GUILESS_MAIN(KisLazyValueWrapperTest)

#include "KisLazyValueWrapperTest.moc"
