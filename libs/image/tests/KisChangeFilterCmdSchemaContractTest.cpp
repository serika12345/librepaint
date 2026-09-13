/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_change_filter_command.h"

#include <QTest>

#include <type_traits>

class KisChangeFilterCmdSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicInterfaceRemainsStable();
};

void KisChangeFilterCmdSchemaContractTest::publicInterfaceRemainsStable()
{
    using Command = KisChangeFilterCmd;
    using Undo = void (Command::*)();
    using Redo = void (Command::*)();

    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_constructible_v<Command, KisNodeSP, KisFilterConfigurationSP, KisFilterConfigurationSP>);
    static_assert(std::is_same_v<decltype(&Command::undo), Undo>);
    static_assert(std::is_same_v<decltype(&Command::redo), Redo>);
}

QTEST_GUILESS_MAIN(KisChangeFilterCmdSchemaContractTest)

#include "KisChangeFilterCmdSchemaContractTest.moc"
