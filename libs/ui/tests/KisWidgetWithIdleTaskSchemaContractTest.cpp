/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/KisWidgetWithIdleTask.h>

#include <QTest>

#include <type_traits>

namespace
{
using IdleTaskWidget = KisWidgetWithIdleTask<QWidget>;

class IdleTaskWidgetConstructorProbe final : public IdleTaskWidget
{
public:
    using IdleTaskWidget::IdleTaskWidget;

private:
    KisIdleTasksManager::TaskGuard registerIdleTask(KisCanvas2 *) override;
    void clearCachedState() override;
};

#define ASSERT_IDLE_TASK_WIDGET_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&IdleTaskWidget::method)), signature>)
} // namespace

class KisWidgetWithIdleTaskSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void idleTaskWidgetTypeConstructionAndLifetimeSchemaRemainStable();
    void idleTaskWidgetCanvasRegistrationAndCacheSchemaRemainStable();
    void idleTaskWidgetVisibilityAndCacheClearingSchemaRemainStable();
};

void KisWidgetWithIdleTaskSchemaContractTest::idleTaskWidgetTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<IdleTaskWidget>);
    static_assert(std::is_base_of_v<QWidget, IdleTaskWidget>);
    static_assert(std::is_abstract_v<IdleTaskWidget>);
    static_assert(std::is_constructible_v<IdleTaskWidgetConstructorProbe, QWidget *, Qt::WindowFlags>);
    static_assert(std::has_virtual_destructor_v<IdleTaskWidget>);

    QVERIFY(true);
}

void KisWidgetWithIdleTaskSchemaContractTest::idleTaskWidgetCanvasRegistrationAndCacheSchemaRemainStable()
{
    ASSERT_IDLE_TASK_WIDGET_SIGNATURE(setCanvas, void (IdleTaskWidget::*)(KisCanvas2 *));
    ASSERT_IDLE_TASK_WIDGET_SIGNATURE(registerIdleTask,
                                      KisIdleTasksManager::TaskGuard (IdleTaskWidget::*)(KisCanvas2 *));
    ASSERT_IDLE_TASK_WIDGET_SIGNATURE(triggerCacheUpdate, void (IdleTaskWidget::*)());

    QVERIFY(true);
}

void KisWidgetWithIdleTaskSchemaContractTest::idleTaskWidgetVisibilityAndCacheClearingSchemaRemainStable()
{
    ASSERT_IDLE_TASK_WIDGET_SIGNATURE(showEvent, void (IdleTaskWidget::*)(QShowEvent *));
    ASSERT_IDLE_TASK_WIDGET_SIGNATURE(hideEvent, void (IdleTaskWidget::*)(QHideEvent *));
    ASSERT_IDLE_TASK_WIDGET_SIGNATURE(clearCachedState, void (IdleTaskWidget::*)());

    QVERIFY(true);
}

#undef ASSERT_IDLE_TASK_WIDGET_SIGNATURE

QTEST_APPLESS_MAIN(KisWidgetWithIdleTaskSchemaContractTest)

#include "KisWidgetWithIdleTaskSchemaContractTest.moc"
