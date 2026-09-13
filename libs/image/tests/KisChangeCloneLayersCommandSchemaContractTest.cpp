/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisChangeCloneLayersCommand.h"

#include <QTest>

#include <type_traits>

class KisChangeCloneLayersCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void changeCloneLayersCommandPublicInterfaceRemainsStable();
};

void KisChangeCloneLayersCommandSchemaContractTest::changeCloneLayersCommandPublicInterfaceRemainsStable()
{
    using Command = KisChangeCloneLayersCommand;
    using Undo = void (Command::*)();
    using Redo = void (Command::*)();
    using Merge = bool (Command::*)(const KUndo2Command *);

    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_constructible_v<Command, QList<KisCloneLayerSP>, KisLayerSP, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(&Command::undo), Undo>);
    static_assert(std::is_same_v<decltype(&Command::redo), Redo>);
    static_assert(std::is_same_v<decltype(&Command::mergeWith), Merge>);
}

QTEST_GUILESS_MAIN(KisChangeCloneLayersCommandSchemaContractTest)

#include "KisChangeCloneLayersCommandSchemaContractTest.moc"
