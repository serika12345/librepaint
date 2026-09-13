/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_move_command2.h"

#include <QTest>

#include <type_traits>

class KisNodeMoveCommand2SchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeMoveCommandPublicInterfaceRemainsStable();
};

void KisNodeMoveCommand2SchemaContractTest::nodeMoveCommandPublicInterfaceRemainsStable()
{
    using Command = KisNodeMoveCommand2;
    using Undo = void (Command::*)();
    using Redo = void (Command::*)();
    using Notify = void (*)(KisNodeSP);

    static_assert(std::is_base_of_v<KisMoveCommandCommon<KisNodeSP>, Command>);
    static_assert(std::is_constructible_v<Command, KisNodeSP, const QPoint &, const QPoint &, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(&Command::undo), Undo>);
    static_assert(std::is_same_v<decltype(&Command::redo), Redo>);
    static_assert(std::is_same_v<decltype(&Command::tryNotifySelection), Notify>);
}

QTEST_GUILESS_MAIN(KisNodeMoveCommand2SchemaContractTest)

#include "KisNodeMoveCommand2SchemaContractTest.moc"
