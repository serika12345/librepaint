/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_invocation_action.h"

#include <QTest>

#include <type_traits>

class KisToolInvocationActionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolInvocationTypeAndLifecycleSchemaRemainStable();
    void toolInvocationPrimaryShortcutOrdinalsRemainStable();
    void toolInvocationTemporaryToolShortcutOrdinalsRemainStable();
    void toolInvocationActivationAndEventSignaturesRemainStable();
    void toolInvocationPolicySignaturesRemainStable();
};

void KisToolInvocationActionSchemaContractTest::toolInvocationTypeAndLifecycleSchemaRemainStable()
{
    using Action = KisToolInvocationAction;
    using Shortcut = Action::Shortcut;

    static_assert(std::is_class_v<Action>);
    static_assert(std::is_base_of_v<KisAbstractInputAction, Action>);
    static_assert(std::is_enum_v<Shortcut>);
    static_assert(std::is_default_constructible_v<Action>);
    static_assert(std::is_destructible_v<Action>);
    static_assert(std::has_virtual_destructor_v<Action>);

    QVERIFY(true);
}

void KisToolInvocationActionSchemaContractTest::toolInvocationPrimaryShortcutOrdinalsRemainStable()
{
    using Shortcut = KisToolInvocationAction::Shortcut;

    QCOMPARE(static_cast<int>(Shortcut::ActivateShortcut), 0);
    QCOMPARE(static_cast<int>(Shortcut::ConfirmShortcut), 1);
    QCOMPARE(static_cast<int>(Shortcut::CancelShortcut), 2);
    QCOMPARE(static_cast<int>(Shortcut::ActivateWithOtherColorShortcut), 14);
}

void KisToolInvocationActionSchemaContractTest::toolInvocationTemporaryToolShortcutOrdinalsRemainStable()
{
    using Shortcut = KisToolInvocationAction::Shortcut;

    QCOMPARE(static_cast<int>(Shortcut::LineToolShortcut), 3);
    QCOMPARE(static_cast<int>(Shortcut::EllipseToolShortcut), 4);
    QCOMPARE(static_cast<int>(Shortcut::RectToolShortcut), 5);
    QCOMPARE(static_cast<int>(Shortcut::MoveToolShortcut), 6);
    QCOMPARE(static_cast<int>(Shortcut::FillToolShortcut), 7);
    QCOMPARE(static_cast<int>(Shortcut::GradientToolShortcut), 8);
    QCOMPARE(static_cast<int>(Shortcut::MeasureToolShortcut), 9);
    QCOMPARE(static_cast<int>(Shortcut::EllipseSelToolShortcut), 10);
    QCOMPARE(static_cast<int>(Shortcut::RectSelToolShortcut), 11);
    QCOMPARE(static_cast<int>(Shortcut::ContigSelToolShortcut), 12);
    QCOMPARE(static_cast<int>(Shortcut::FreehandSelToolShortcut), 13);
}

void KisToolInvocationActionSchemaContractTest::toolInvocationActivationAndEventSignaturesRemainStable()
{
    using Action = KisToolInvocationAction;
    using ShortcutOperation = void (Action::*)(int);
    using BeginOperation = void (Action::*)(int, QEvent *);
    using EventOperation = void (Action::*)(QEvent *);

    static_assert(std::is_same_v<decltype(static_cast<ShortcutOperation>(&Action::activate)), ShortcutOperation>);
    static_assert(std::is_same_v<decltype(static_cast<ShortcutOperation>(&Action::deactivate)), ShortcutOperation>);
    static_assert(std::is_same_v<decltype(static_cast<BeginOperation>(&Action::begin)), BeginOperation>);
    static_assert(std::is_same_v<decltype(static_cast<EventOperation>(&Action::end)), EventOperation>);
    static_assert(std::is_same_v<decltype(static_cast<EventOperation>(&Action::inputEvent)), EventOperation>);
    static_assert(
        std::is_same_v<decltype(static_cast<EventOperation>(&Action::processUnhandledEvent)), EventOperation>);

    QVERIFY(true);
}

void KisToolInvocationActionSchemaContractTest::toolInvocationPolicySignaturesRemainStable()
{
    using Action = KisToolInvocationAction;
    using PriorityQuery = int (Action::*)() const;
    using BooleanQuery = bool (Action::*)() const;
    using ShortcutBooleanQuery = bool (Action::*)(int) const;
    using GroupQuery = KisInputActionGroup (Action::*)(int) const;

    static_assert(std::is_same_v<decltype(static_cast<PriorityQuery>(&Action::priority)), PriorityQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Action::canIgnoreModifiers)), BooleanQuery>);
    static_assert(
        std::is_same_v<decltype(static_cast<ShortcutBooleanQuery>(&Action::isShortcutRequired)), ShortcutBooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<ShortcutBooleanQuery>(&Action::supportsHiResInputEvents)),
                                 ShortcutBooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<GroupQuery>(&Action::inputActionGroup)), GroupQuery>);

    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KisToolInvocationActionSchemaContractTest)

#include "KisToolInvocationActionSchemaContractTest.moc"
