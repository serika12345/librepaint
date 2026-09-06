/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/KisChangeDeselectedMaskCommand.h>
#include <commands/KisDeselectActiveSelectionCommand.h>
#include <commands/KisNotifySelectionChangedCommand.h>
#include <commands/KisReselectActiveSelectionCommand.h>
#include <commands/kis_deselect_global_selection_command.h>
#include <commands/kis_reselect_global_selection_command.h>
#include <commands/kis_set_global_selection_command.h>

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_COMMAND_SIGNATURE(command, method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&command::method)), signature>)

} // namespace

class KisSelectionStateCommandsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void deselectedMaskChangeSchemaRemainStable();
    void globalSelectionSetAndNotificationSchemaRemainStable();
    void globalSelectionDeselectSchemaRemainStable();
    void globalSelectionReselectSchemaRemainStable();
    void activeSelectionDeselectAndReselectSchemaRemainStable();
};

void KisSelectionStateCommandsSchemaContractTest::deselectedMaskChangeSchemaRemainStable()
{
    using Command = KisChangeDeselectedMaskCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_constructible_v<Command, KisImageWSP>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KisSelectionMaskSP>);
    ASSERT_COMMAND_SIGNATURE(Command, redo, void (Command::*)());
    ASSERT_COMMAND_SIGNATURE(Command, undo, void (Command::*)());

    QVERIFY(true);
}

void KisSelectionStateCommandsSchemaContractTest::globalSelectionSetAndNotificationSchemaRemainStable()
{
    using SetCommand = KisSetGlobalSelectionCommand;
    using SetEmptyCommand = KisSetEmptyGlobalSelectionCommand;
    using NotifyCommand = KisNotifySelectionChangedCommand;
    using NotificationState = KisCommandUtils::FlipFlopCommand::State;

    static_assert(std::is_class_v<SetCommand>);
    static_assert(std::is_base_of_v<KisCommandUtils::AggregateCommand, SetCommand>);
    static_assert(std::is_constructible_v<SetCommand, KisImageWSP, KisSelectionSP>);
    ASSERT_COMMAND_SIGNATURE(SetCommand, populateChildCommands, void (SetCommand::*)());

    static_assert(std::is_class_v<SetEmptyCommand>);
    static_assert(std::is_base_of_v<SetCommand, SetEmptyCommand>);
    static_assert(std::is_constructible_v<SetEmptyCommand, KisImageWSP>);

    static_assert(std::is_class_v<NotifyCommand>);
    static_assert(std::is_base_of_v<KisCommandUtils::FlipFlopCommand, NotifyCommand>);
    static_assert(std::is_constructible_v<NotifyCommand, KisImageWSP, NotificationState>);
    ASSERT_COMMAND_SIGNATURE(NotifyCommand, partB, void (NotifyCommand::*)());

    QVERIFY(true);
}

void KisSelectionStateCommandsSchemaContractTest::globalSelectionDeselectSchemaRemainStable()
{
    using Command = KisDeselectGlobalSelectionCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KisCommandUtils::AggregateCommand, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KisImageWSP>);
    ASSERT_COMMAND_SIGNATURE(Command, populateChildCommands, void (Command::*)());

    QVERIFY(true);
}

void KisSelectionStateCommandsSchemaContractTest::globalSelectionReselectSchemaRemainStable()
{
    using Command = KisReselectGlobalSelectionCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KisCommandUtils::AggregateCommand, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KisImageWSP>);
    ASSERT_COMMAND_SIGNATURE(Command, populateChildCommands, void (Command::*)());

    QVERIFY(true);
}

void KisSelectionStateCommandsSchemaContractTest::activeSelectionDeselectAndReselectSchemaRemainStable()
{
    using DeselectCommand = KisDeselectActiveSelectionCommand;
    using ReselectCommand = KisReselectActiveSelectionCommand;

    static_assert(std::is_class_v<DeselectCommand>);
    static_assert(std::is_base_of_v<KisDeselectGlobalSelectionCommand, DeselectCommand>);
    static_assert(std::has_virtual_destructor_v<DeselectCommand>);
    static_assert(std::is_constructible_v<DeselectCommand, KisSelectionSP, KisImageWSP, KUndo2Command *>);
    static_assert(std::is_constructible_v<DeselectCommand, KisSelectionSP, KisImageWSP>);
    ASSERT_COMMAND_SIGNATURE(DeselectCommand, redo, void (DeselectCommand::*)());
    ASSERT_COMMAND_SIGNATURE(DeselectCommand, undo, void (DeselectCommand::*)());

    static_assert(std::is_class_v<ReselectCommand>);
    static_assert(std::is_base_of_v<KisReselectGlobalSelectionCommand, ReselectCommand>);
    static_assert(std::is_constructible_v<ReselectCommand, KisNodeSP, KisImageWSP, KUndo2Command *>);
    static_assert(std::is_constructible_v<ReselectCommand, KisNodeSP, KisImageWSP>);
    ASSERT_COMMAND_SIGNATURE(ReselectCommand, redo, void (ReselectCommand::*)());
    ASSERT_COMMAND_SIGNATURE(ReselectCommand, undo, void (ReselectCommand::*)());

    QVERIFY(true);
}

#undef ASSERT_COMMAND_SIGNATURE

QTEST_APPLESS_MAIN(KisSelectionStateCommandsSchemaContractTest)

#include "KisSelectionStateCommandsSchemaContractTest.moc"
