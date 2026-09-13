/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_set_layer_style_command.h"

#include <QTest>

#include <type_traits>

class KisSetLayerStyleCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicInterfaceRemainsStable();
};

void KisSetLayerStyleCommandSchemaContractTest::publicInterfaceRemainsStable()
{
    using Command = KisSetLayerStyleCommand;
    using Undo = void (Command::*)();
    using Redo = void (Command::*)();
    using Update = void (*)(KisLayerSP, KisPSDLayerStyleSP);

    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(
        std::is_constructible_v<Command, KisLayerSP, KisPSDLayerStyleSP, KisPSDLayerStyleSP, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(&Command::undo), Undo>);
    static_assert(std::is_same_v<decltype(&Command::redo), Redo>);
    static_assert(std::is_same_v<decltype(&Command::updateLayerStyle), Update>);
}

QTEST_GUILESS_MAIN(KisSetLayerStyleCommandSchemaContractTest)

#include "KisSetLayerStyleCommandSchemaContractTest.moc"
