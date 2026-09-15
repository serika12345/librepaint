/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "actions/KisTransformToolActivationCommand.h"

#include <QTest>

#include <type_traits>

class KisTransformToolActivationCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void transformToolActivationCommandSchemaRemainStable();
};

void KisTransformToolActivationCommandSchemaContractTest::transformToolActivationCommandSchemaRemainStable()
{
    using Command = KisTransformToolActivationCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<QObject, Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_constructible_v<Command, KisViewManager *, KUndo2Command *>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_same_v<decltype(&Command::redo), void (Command::*)()>);
    static_assert(std::is_same_v<decltype(&Command::undo), void (Command::*)()>);
    static_assert(std::is_same_v<decltype(&Command::requestTransformTool), void (Command::*)()>);
}

QTEST_GUILESS_MAIN(KisTransformToolActivationCommandSchemaContractTest)

#include "KisTransformToolActivationCommandSchemaContractTest.moc"
