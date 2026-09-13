/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_activate_selection_mask_command.h"

#include <QTest>

#include <type_traits>

class KisActivateSelectionMaskCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void activationCommandPublicInterfaceRemainsStable();
};

void KisActivateSelectionMaskCommandSchemaContractTest::activationCommandPublicInterfaceRemainsStable()
{
    using Command = KisActivateSelectionMaskCommand;
    using Undo = void (Command::*)();
    using Redo = void (Command::*)();

    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_constructible_v<Command, KisSelectionMaskSP, bool>);
    static_assert(std::is_same_v<decltype(&Command::undo), Undo>);
    static_assert(std::is_same_v<decltype(&Command::redo), Redo>);
}

QTEST_GUILESS_MAIN(KisActivateSelectionMaskCommandSchemaContractTest)

#include "KisActivateSelectionMaskCommandSchemaContractTest.moc"
