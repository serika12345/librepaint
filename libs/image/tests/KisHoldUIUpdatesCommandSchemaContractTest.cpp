/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisHoldUIUpdatesCommand.h"

#include <QTest>

#include <type_traits>

class KisHoldUIUpdatesCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void holdUiUpdatesCommandPublicInterfaceRemainsStable();
};

void KisHoldUIUpdatesCommandSchemaContractTest::holdUiUpdatesCommandPublicInterfaceRemainsStable()
{
    using Command = KisHoldUIUpdatesCommand;
    using Phase = void (Command::*)();

    static_assert(std::is_base_of_v<KisCommandUtils::FlipFlopCommand, Command>);
    static_assert(std::is_base_of_v<KisStrokeStrategyUndoCommandBased::MutatedCommandInterface, Command>);
    static_assert(std::is_constructible_v<Command, KisUpdatesFacade *, KisCommandUtils::FlipFlopCommand::State>);
    static_assert(std::is_same_v<decltype(&Command::partA), Phase>);
    static_assert(std::is_same_v<decltype(&Command::partB), Phase>);
}

QTEST_GUILESS_MAIN(KisHoldUIUpdatesCommandSchemaContractTest)

#include "KisHoldUIUpdatesCommandSchemaContractTest.moc"
