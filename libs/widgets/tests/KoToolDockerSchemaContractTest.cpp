/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoToolDocker.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TOOL_DOCKER_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoToolDocker::method)), signature>)
} // namespace

class KoToolDockerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolDockerTypeConstructionAndLifetimeSchemaRemainStable();
    void toolDockerCanvasLifecycleSchemaRemainStable();
    void toolDockerOptionWidgetAndScrollerSchemaRemainStable();
};

void KoToolDockerSchemaContractTest::toolDockerTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_base_of_v<QDockWidget, KoToolDocker>);
    static_assert(std::is_base_of_v<KoCanvasObserverBase, KoToolDocker>);
    static_assert(std::is_constructible_v<KoToolDocker, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KoToolDocker>);

    QVERIFY(true);
}

void KoToolDockerSchemaContractTest::toolDockerCanvasLifecycleSchemaRemainStable()
{
    ASSERT_TOOL_DOCKER_SIGNATURE(resetWidgets, void (KoToolDocker::*)());
    ASSERT_TOOL_DOCKER_SIGNATURE(setCanvas, void (KoToolDocker::*)(KoCanvasBase *));
    ASSERT_TOOL_DOCKER_SIGNATURE(unsetCanvas, void (KoToolDocker::*)());

    QVERIFY(true);
}

void KoToolDockerSchemaContractTest::toolDockerOptionWidgetAndScrollerSchemaRemainStable()
{
    ASSERT_TOOL_DOCKER_SIGNATURE(setOptionWidgets, void (KoToolDocker::*)(const QList<QPointer<QWidget>> &));
    ASSERT_TOOL_DOCKER_SIGNATURE(slotScrollerStateChange, void (KoToolDocker::*)(QScroller::State));
    ASSERT_TOOL_DOCKER_SIGNATURE(hasOptionWidget, bool (KoToolDocker::*)());

    QVERIFY(true);
}

#undef ASSERT_TOOL_DOCKER_SIGNATURE

QTEST_APPLESS_MAIN(KoToolDockerSchemaContractTest)

#include "KoToolDockerSchemaContractTest.moc"
