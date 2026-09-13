/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_resize_command.h"

#include <QTest>

#include <type_traits>

class KisImageResizeCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageResizeCommandPublicInterfaceRemainsStable();
};

void KisImageResizeCommandSchemaContractTest::imageResizeCommandPublicInterfaceRemainsStable()
{
    using Command = KisImageResizeCommand;
    using Undo = void (Command::*)();
    using Redo = void (Command::*)();

    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, const QSize &, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(&Command::undo), Undo>);
    static_assert(std::is_same_v<decltype(&Command::redo), Redo>);
}

QTEST_GUILESS_MAIN(KisImageResizeCommandSchemaContractTest)

#include "KisImageResizeCommandSchemaContractTest.moc"
