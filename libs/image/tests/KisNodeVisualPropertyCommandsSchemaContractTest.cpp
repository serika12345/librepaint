/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/kis_node_compositeop_command.h>
#include <commands/kis_node_opacity_command.h>

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_COMMAND_SIGNATURE(command, method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&command::method)), signature>)

} // namespace

class KisNodeVisualPropertyCommandsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeLifetimeAndConstructionSchemaRemainStable();
    void reversibleExecutionSignaturesRemainStable();
    void historyIdentityAndMergeSignaturesRemainStable();
    void annihilationSignaturesRemainStable();
};

void KisNodeVisualPropertyCommandsSchemaContractTest::typeLifetimeAndConstructionSchemaRemainStable()
{
    using CompositeCommand = KisNodeCompositeOpCommand;
    using OpacityCommand = KisNodeOpacityCommand;

    static_assert(std::is_class_v<CompositeCommand>);
    static_assert(std::is_base_of_v<KisNodeCommand, CompositeCommand>);
    static_assert(std::is_base_of_v<KisAsynchronouslyMergeableCommandInterface, CompositeCommand>);
    static_assert(std::has_virtual_destructor_v<CompositeCommand>);
    static_assert(std::is_constructible_v<CompositeCommand, KisNodeSP, const QString &>);

    static_assert(std::is_class_v<OpacityCommand>);
    static_assert(std::is_base_of_v<KisNodeCommand, OpacityCommand>);
    static_assert(std::is_base_of_v<KisAsynchronouslyMergeableCommandInterface, OpacityCommand>);
    static_assert(std::has_virtual_destructor_v<OpacityCommand>);
    static_assert(std::is_constructible_v<OpacityCommand, KisNodeSP, quint8>);

    QVERIFY(true);
}

void KisNodeVisualPropertyCommandsSchemaContractTest::reversibleExecutionSignaturesRemainStable()
{
    ASSERT_COMMAND_SIGNATURE(KisNodeCompositeOpCommand, redo, void (KisNodeCompositeOpCommand::*)());
    ASSERT_COMMAND_SIGNATURE(KisNodeCompositeOpCommand, undo, void (KisNodeCompositeOpCommand::*)());
    ASSERT_COMMAND_SIGNATURE(KisNodeOpacityCommand, redo, void (KisNodeOpacityCommand::*)());
    ASSERT_COMMAND_SIGNATURE(KisNodeOpacityCommand, undo, void (KisNodeOpacityCommand::*)());

    QVERIFY(true);
}

void KisNodeVisualPropertyCommandsSchemaContractTest::historyIdentityAndMergeSignaturesRemainStable()
{
    ASSERT_COMMAND_SIGNATURE(KisNodeCompositeOpCommand, id, int (KisNodeCompositeOpCommand::*)() const);
    ASSERT_COMMAND_SIGNATURE(KisNodeCompositeOpCommand,
                             mergeWith,
                             bool (KisNodeCompositeOpCommand::*)(const KUndo2Command *));
    ASSERT_COMMAND_SIGNATURE(KisNodeCompositeOpCommand,
                             canMergeWith,
                             bool (KisNodeCompositeOpCommand::*)(const KUndo2Command *) const);

    ASSERT_COMMAND_SIGNATURE(KisNodeOpacityCommand, id, int (KisNodeOpacityCommand::*)() const);
    ASSERT_COMMAND_SIGNATURE(KisNodeOpacityCommand, mergeWith, bool (KisNodeOpacityCommand::*)(const KUndo2Command *));
    ASSERT_COMMAND_SIGNATURE(KisNodeOpacityCommand,
                             canMergeWith,
                             bool (KisNodeOpacityCommand::*)(const KUndo2Command *) const);

    QVERIFY(true);
}

void KisNodeVisualPropertyCommandsSchemaContractTest::annihilationSignaturesRemainStable()
{
    ASSERT_COMMAND_SIGNATURE(KisNodeCompositeOpCommand,
                             canAnnihilateWith,
                             bool (KisNodeCompositeOpCommand::*)(const KUndo2Command *) const);
    ASSERT_COMMAND_SIGNATURE(KisNodeOpacityCommand,
                             canAnnihilateWith,
                             bool (KisNodeOpacityCommand::*)(const KUndo2Command *) const);

    QVERIFY(true);
}

#undef ASSERT_COMMAND_SIGNATURE

QTEST_APPLESS_MAIN(KisNodeVisualPropertyCommandsSchemaContractTest)

#include "KisNodeVisualPropertyCommandsSchemaContractTest.moc"
