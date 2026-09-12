/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSimpleModifyTransformMaskCommand.h"

#include <QTest>

#include <type_traits>

class KisSimpleModifyTransformMaskCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndConstructionSchemaRemainStable();
    void identityAndMergeSignaturesRemainStable();
    void undoAndRedoSignaturesRemainStable();
};

void KisSimpleModifyTransformMaskCommandSchemaContractTest::typeAndConstructionSchemaRemainStable()
{
    using Command = KisSimpleModifyTransformMaskCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_constructible_v<Command,
                                          KisTransformMaskSP,
                                          KisTransformMaskParamsInterfaceSP,
                                          QWeakPointer<boost::none_t>,
                                          KUndo2Command *>);
}

void KisSimpleModifyTransformMaskCommandSchemaContractTest::identityAndMergeSignaturesRemainStable()
{
    using Command = KisSimpleModifyTransformMaskCommand;
    using Id = int (Command::*)() const;
    using Merge = bool (Command::*)(const KUndo2Command *);

    static_assert(std::is_same_v<decltype(&Command::id), Id>);
    static_assert(std::is_same_v<decltype(&Command::mergeWith), Merge>);
}

void KisSimpleModifyTransformMaskCommandSchemaContractTest::undoAndRedoSignaturesRemainStable()
{
    using Command = KisSimpleModifyTransformMaskCommand;
    using Operation = void (Command::*)();

    static_assert(std::is_same_v<decltype(&Command::undo), Operation>);
    static_assert(std::is_same_v<decltype(&Command::redo), Operation>);
}

QTEST_GUILESS_MAIN(KisSimpleModifyTransformMaskCommandSchemaContractTest)

#include "KisSimpleModifyTransformMaskCommandSchemaContractTest.moc"
