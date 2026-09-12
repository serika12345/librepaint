/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/KisIdleTaskStrokeStrategy.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_IDLE_TASK_STRATEGY_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisIdleTaskStrokeStrategy::method)), signature>)
} // namespace

class KisIdleTaskStrokeStrategySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void idleTaskStrategyTypeConstructionAndLifetimeSchemaRemainStable();
    void idleTaskStrategyCloneAndCookieSchemaRemainStable();
    void idleTaskStrategyTimingAndNotificationSchemaRemainStable();
};

void KisIdleTaskStrokeStrategySchemaContractTest::idleTaskStrategyTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Strategy = KisIdleTaskStrokeStrategy;

    static_assert(std::is_class_v<Strategy>);
    static_assert(std::is_base_of_v<QObject, Strategy>);
    static_assert(std::is_base_of_v<KisRunnableBasedStrokeStrategy, Strategy>);
    static_assert(std::is_constructible_v<Strategy, const QLatin1String &>);
    static_assert(std::has_virtual_destructor_v<Strategy>);
    static_assert(std::is_same_v<KisIdleTaskStrokeStrategyFactory, std::function<Strategy *(KisImageSP)>>);

    QVERIFY(true);
}

void KisIdleTaskStrokeStrategySchemaContractTest::idleTaskStrategyCloneAndCookieSchemaRemainStable()
{
    ASSERT_IDLE_TASK_STRATEGY_SIGNATURE(createLodClone, KisStrokeStrategy * (KisIdleTaskStrokeStrategy::*)(int));
    ASSERT_IDLE_TASK_STRATEGY_SIGNATURE(idleTaskCookie, QWeakPointer<boost::none_t> (KisIdleTaskStrokeStrategy::*)());

    QVERIFY(true);
}

void KisIdleTaskStrokeStrategySchemaContractTest::idleTaskStrategyTimingAndNotificationSchemaRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisIdleTaskStrokeStrategy::preferredIdleTaskMaximumTime), int (*)()>);
    static_assert(std::is_same_v<decltype(&KisIdleTaskStrokeStrategy::preferredIdleWatcherInterval), int (*)()>);
    ASSERT_IDLE_TASK_STRATEGY_SIGNATURE(sigIdleTaskFinished, void (KisIdleTaskStrokeStrategy::*)());

    QVERIFY(true);
}

#undef ASSERT_IDLE_TASK_STRATEGY_SIGNATURE

QTEST_APPLESS_MAIN(KisIdleTaskStrokeStrategySchemaContractTest)

#include "KisIdleTaskStrokeStrategySchemaContractTest.moc"
