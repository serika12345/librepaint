/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_stroke_strategy_undo_command_based.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_UNDO_COMMAND_STRATEGY_SIGNATURE(method, signature)                                                      \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&KisStrokeStrategyUndoCommandBased::method)), signature>)
#define ASSERT_UNDO_COMMAND_MUTATION_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(                                                      \
                                     &KisStrokeStrategyUndoCommandBased::MutatedCommandInterface::method)),            \
                                 signature>)
} // namespace

class KisStrokeStrategyUndoCommandBasedSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void undoCommandStrategyTypeAndConstructionSchemaRemainStable();
    void undoCommandDataTypeAndOwnershipSchemaRemainStable();
    void undoCommandDataConstructionSignaturesRemainStable();
    void undoCommandMutationInterfaceSchemaRemainStable();
    void undoCommandLifecycleAndConfigurationSignaturesRemainStable();
};

void KisStrokeStrategyUndoCommandBasedSchemaContractTest::undoCommandStrategyTypeAndConstructionSchemaRemainStable()
{
    using Strategy = KisStrokeStrategyUndoCommandBased;

    static_assert(std::is_class_v<Strategy>);
    static_assert(std::is_constructible_v<Strategy,
                                          const KUndo2MagicString &,
                                          bool,
                                          KisStrokeUndoFacade *,
                                          KUndo2CommandSP,
                                          KUndo2CommandSP>);
    static_assert(
        std::is_same_v<
            decltype(Strategy(std::declval<const KUndo2MagicString &>(), false, std::declval<KisStrokeUndoFacade *>())),
            Strategy>);
}

void KisStrokeStrategyUndoCommandBasedSchemaContractTest::undoCommandDataTypeAndOwnershipSchemaRemainStable()
{
    using Data = KisStrokeStrategyUndoCommandBased::Data;

    static_assert(std::is_class_v<Data>);
    static_assert(std::has_virtual_destructor_v<Data>);
    static_assert(std::is_same_v<decltype(Data::command), KUndo2CommandSP>);
    static_assert(std::is_same_v<decltype(Data::undo), bool>);
    static_assert(std::is_same_v<decltype(Data::shouldGoToHistory), bool>);
}

void KisStrokeStrategyUndoCommandBasedSchemaContractTest::undoCommandDataConstructionSignaturesRemainStable()
{
    using Data = KisStrokeStrategyUndoCommandBased::Data;
    using Sequentiality = KisStrokeJobData::Sequentiality;
    using Exclusivity = KisStrokeJobData::Exclusivity;

    static_assert(std::is_constructible_v<Data, KUndo2Command *, bool, Sequentiality, Exclusivity, bool>);
    static_assert(std::is_constructible_v<Data, KUndo2CommandSP, bool, Sequentiality, Exclusivity, bool>);
    static_assert(std::is_same_v<decltype(Data(std::declval<KUndo2Command *>())), Data>);
    static_assert(std::is_same_v<decltype(Data(std::declval<KUndo2CommandSP>())), Data>);
}

void KisStrokeStrategyUndoCommandBasedSchemaContractTest::undoCommandMutationInterfaceSchemaRemainStable()
{
    using Mutation = KisStrokeStrategyUndoCommandBased::MutatedCommandInterface;

    static_assert(std::is_class_v<Mutation>);
    static_assert(std::has_virtual_destructor_v<Mutation>);
    ASSERT_UNDO_COMMAND_MUTATION_SIGNATURE(setRunnableJobsInterface,
                                           void (Mutation::*)(KisRunnableStrokeJobsInterface *));
    ASSERT_UNDO_COMMAND_MUTATION_SIGNATURE(runnableJobsInterface,
                                           KisRunnableStrokeJobsInterface * (Mutation::*)() const);
}

void KisStrokeStrategyUndoCommandBasedSchemaContractTest::undoCommandLifecycleAndConfigurationSignaturesRemainStable()
{
    using Strategy = KisStrokeStrategyUndoCommandBased;

    ASSERT_UNDO_COMMAND_STRATEGY_SIGNATURE(initStrokeCallback, void (Strategy::*)());
    ASSERT_UNDO_COMMAND_STRATEGY_SIGNATURE(finishStrokeCallback, void (Strategy::*)());
    ASSERT_UNDO_COMMAND_STRATEGY_SIGNATURE(cancelStrokeCallback, void (Strategy::*)());
    ASSERT_UNDO_COMMAND_STRATEGY_SIGNATURE(doStrokeCallback, void (Strategy::*)(KisStrokeJobData *));
    ASSERT_UNDO_COMMAND_STRATEGY_SIGNATURE(setCommandExtraData, void (Strategy::*)(KUndo2CommandExtraData *));
    ASSERT_UNDO_COMMAND_STRATEGY_SIGNATURE(setMacroId, void (Strategy::*)(int));
    ASSERT_UNDO_COMMAND_STRATEGY_SIGNATURE(setUsedWhileUndoRedo, void (Strategy::*)(bool));
}

QTEST_APPLESS_MAIN(KisStrokeStrategyUndoCommandBasedSchemaContractTest)

#include "KisStrokeStrategyUndoCommandBasedSchemaContractTest.moc"
