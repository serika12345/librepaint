/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisAnimAutoKey.h>
#include <commands_new/kis_switch_current_time_command.h>

#include <QTest>

#include <type_traits>
#include <utility>

#define ASSERT_COMMAND_SIGNATURE(command, method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&command::method)), signature>)
#define ASSERT_FUNCTION_SIGNATURE(function, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&function)), signature>)

class KisAnimationTimeCommandsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void currentTimeCommandTypeConstructionAndLifetimeSchemaRemainStable();
    void currentTimeCommandHistoryAndExecutionSchemaRemainStable();
    void keyframeTimeCommandSchemaRemainStable();
    void autoKeyModesAndFlagsRemainStable();
    void autoKeyModeAccessSchemaRemainStable();
    void autoKeyFrameCreationSchemaRemainStable();
};

void KisAnimationTimeCommandsSchemaContractTest::currentTimeCommandTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Command = KisSwitchCurrentTimeCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KisImageAnimationInterface *, int, int, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KisImageAnimationInterface *, int, int>);

    QVERIFY(true);
}

void KisAnimationTimeCommandsSchemaContractTest::currentTimeCommandHistoryAndExecutionSchemaRemainStable()
{
    using Command = KisSwitchCurrentTimeCommand;

    ASSERT_COMMAND_SIGNATURE(Command, id, int (Command::*)() const);
    ASSERT_COMMAND_SIGNATURE(Command, mergeWith, bool (Command::*)(const KUndo2Command *));
    ASSERT_COMMAND_SIGNATURE(Command, redo, void (Command::*)());
    ASSERT_COMMAND_SIGNATURE(Command, undo, void (Command::*)());

    QVERIFY(true);
}

void KisAnimationTimeCommandsSchemaContractTest::keyframeTimeCommandSchemaRemainStable()
{
    using Command = KisSwitchCurrentTimeToKeyframeCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command,
                                          KisImageAnimationInterface *,
                                          int,
                                          KisNodeSP,
                                          KoID,
                                          KisKeyframeSP,
                                          KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KisImageAnimationInterface *, int, KisNodeSP, KoID, KisKeyframeSP>);
    ASSERT_COMMAND_SIGNATURE(Command, id, int (Command::*)() const);
    ASSERT_COMMAND_SIGNATURE(Command, mergeWith, bool (Command::*)(const KUndo2Command *));
    ASSERT_COMMAND_SIGNATURE(Command, redo, void (Command::*)());
    ASSERT_COMMAND_SIGNATURE(Command, undo, void (Command::*)());

    QVERIFY(true);
}

void KisAnimationTimeCommandsSchemaContractTest::autoKeyModesAndFlagsRemainStable()
{
    static_assert(std::is_enum_v<KisAutoKey::Mode>);
    static_assert(KisAutoKey::NONE == 0);
    static_assert(KisAutoKey::BLANK == 1);
    static_assert(KisAutoKey::DUPLICATE == 2);
    static_assert(std::is_enum_v<KisAutoKey::AutoCreateKeyframeFlag>);
    static_assert(KisAutoKey::None == 0x0);
    static_assert(KisAutoKey::AllowBlankMode == 0x1);
    static_assert(KisAutoKey::SupportsLod == 0x2);
    static_assert(std::is_same_v<KisAutoKey::AutoCreateKeyframeFlags, QFlags<KisAutoKey::AutoCreateKeyframeFlag>>);

    QVERIFY(true);
}

void KisAnimationTimeCommandsSchemaContractTest::autoKeyModeAccessSchemaRemainStable()
{
    ASSERT_FUNCTION_SIGNATURE(KisAutoKey::activeMode, KisAutoKey::Mode (*)());
    ASSERT_FUNCTION_SIGNATURE(KisAutoKey::testingSetActiveMode, void (*)(KisAutoKey::Mode));

    QVERIFY(true);
}

void KisAnimationTimeCommandsSchemaContractTest::autoKeyFrameCreationSchemaRemainStable()
{
    using CreateFunction = KUndo2Command *(*)(KisPaintDeviceSP, KisAutoKey::AutoCreateKeyframeFlags);

    ASSERT_FUNCTION_SIGNATURE(KisAutoKey::tryAutoCreateDuplicatedFrame, CreateFunction);
    static_assert(std::is_same_v<decltype(KisAutoKey::tryAutoCreateDuplicatedFrame(std::declval<KisPaintDeviceSP>())),
                                 KUndo2Command *>);

    QVERIFY(true);
}

#undef ASSERT_FUNCTION_SIGNATURE
#undef ASSERT_COMMAND_SIGNATURE

QTEST_APPLESS_MAIN(KisAnimationTimeCommandsSchemaContractTest)

#include "KisAnimationTimeCommandsSchemaContractTest.moc"
