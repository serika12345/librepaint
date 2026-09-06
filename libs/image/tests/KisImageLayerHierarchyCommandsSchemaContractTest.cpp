/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/kis_image_command.h>
#include <commands/kis_image_layer_add_command.h>
#include <commands/kis_image_layer_move_command.h>
#include <commands/kis_image_layer_remove_command.h>
#include <commands/kis_image_layer_remove_command_impl.h>

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_COMMAND_SIGNATURE(command, method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&command::method)), signature>)

} // namespace

class KisImageLayerHierarchyCommandsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageCommandBaseAndUpdateTargetSchemaRemainStable();
    void layerAdditionTypeAndFlagsSchemaRemainStable();
    void layerAdditionConstructionAndExecutionSchemaRemainStable();
    void layerMoveAndRemovalSchemaRemainStable();
    void layerRemovalImplementationSchemaRemainStable();
};

void KisImageLayerHierarchyCommandsSchemaContractTest::imageCommandBaseAndUpdateTargetSchemaRemainStable()
{
    using Command = KisImageCommand;
    using UpdateTarget = Command::UpdateTarget;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, const KUndo2MagicString &, KisImageWSP, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, const KUndo2MagicString &, KisImageWSP>);

    static_assert(std::is_class_v<UpdateTarget>);
    static_assert(std::is_constructible_v<UpdateTarget, KisImageWSP, KisNodeSP, const QRect &>);
    ASSERT_COMMAND_SIGNATURE(UpdateTarget, update, void (UpdateTarget::*)());

    QVERIFY(true);
}

void KisImageLayerHierarchyCommandsSchemaContractTest::layerAdditionTypeAndFlagsSchemaRemainStable()
{
    using Command = KisImageLayerAddCommand;
    using Flag = Command::Flag;
    using Flags = Command::Flags;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KisImageCommand, Command>);
    static_assert(std::is_enum_v<Flag>);
    static_assert(std::is_same_v<Flags, QFlags<Flag>>);
    static_assert(Command::None == 0x0);
    static_assert(Command::DoRedoUpdates == 0x1);
    static_assert(Command::DoUndoUpdates == 0x2);
    static_assert(Command::DontActivateOnAddition == 0x4);

    QVERIFY(true);
}

void KisImageLayerHierarchyCommandsSchemaContractTest::layerAdditionConstructionAndExecutionSchemaRemainStable()
{
    using Command = KisImageLayerAddCommand;
    using Flags = Command::Flags;

    static_assert(std::is_constructible_v<Command, KisImageWSP, KisNodeSP, KisNodeSP, KisNodeSP, bool, bool>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KisNodeSP, KisNodeSP, KisNodeSP>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KisNodeSP, KisNodeSP, quint32, bool, bool>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KisNodeSP, KisNodeSP, quint32>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KisNodeSP, KisNodeSP, KisNodeSP, Flags>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KisNodeSP, KisNodeSP, quint32, Flags>);
    ASSERT_COMMAND_SIGNATURE(Command, redo, void (Command::*)());
    ASSERT_COMMAND_SIGNATURE(Command, undo, void (Command::*)());

    QVERIFY(true);
}

void KisImageLayerHierarchyCommandsSchemaContractTest::layerMoveAndRemovalSchemaRemainStable()
{
    using MoveCommand = KisImageLayerMoveCommand;
    using RemoveCommand = KisImageLayerRemoveCommand;

    static_assert(std::is_class_v<MoveCommand>);
    static_assert(std::is_base_of_v<KisImageCommand, MoveCommand>);
    static_assert(std::is_constructible_v<MoveCommand, KisImageWSP, KisNodeSP, KisNodeSP, KisNodeSP, bool>);
    static_assert(std::is_constructible_v<MoveCommand, KisImageWSP, KisNodeSP, KisNodeSP, KisNodeSP>);
    static_assert(std::is_constructible_v<MoveCommand, KisImageWSP, KisNodeSP, KisNodeSP, quint32>);
    ASSERT_COMMAND_SIGNATURE(MoveCommand, redo, void (MoveCommand::*)());
    ASSERT_COMMAND_SIGNATURE(MoveCommand, undo, void (MoveCommand::*)());

    static_assert(std::is_class_v<RemoveCommand>);
    static_assert(std::is_base_of_v<KisImageCommand, RemoveCommand>);
    static_assert(std::has_virtual_destructor_v<RemoveCommand>);
    static_assert(std::is_constructible_v<RemoveCommand, KisImageWSP, KisNodeSP, bool, bool>);
    static_assert(std::is_constructible_v<RemoveCommand, KisImageWSP, KisNodeSP>);
    ASSERT_COMMAND_SIGNATURE(RemoveCommand, redo, void (RemoveCommand::*)());
    ASSERT_COMMAND_SIGNATURE(RemoveCommand, undo, void (RemoveCommand::*)());

    QVERIFY(true);
}

void KisImageLayerHierarchyCommandsSchemaContractTest::layerRemovalImplementationSchemaRemainStable()
{
    using Command = KisImageLayerRemoveCommandImpl;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KisImageCommand, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KisNodeSP, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, KisNodeSP>);
    ASSERT_COMMAND_SIGNATURE(Command, redo, void (Command::*)());
    ASSERT_COMMAND_SIGNATURE(Command, undo, void (Command::*)());

    QVERIFY(true);
}

#undef ASSERT_COMMAND_SIGNATURE

QTEST_APPLESS_MAIN(KisImageLayerHierarchyCommandsSchemaContractTest)

#include "KisImageLayerHierarchyCommandsSchemaContractTest.moc"
