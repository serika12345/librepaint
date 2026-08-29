/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_command_utils.h"

#include <QStringList>
#include <QTest>

#include <memory>
#include <utility>

namespace
{

int safeAssertCount = 0;

struct CommandState {
    int value = 0;
    int destructions = 0;
    int merges = 0;
    QStringList calls;
};

class RecordingCommand : public KUndo2Command
{
public:
    RecordingCommand(CommandState *state, QString name, int delta, bool mergeable = false)
        : KUndo2Command(kundo2_noi18n(name))
        , m_state(state)
        , m_name(std::move(name))
        , m_delta(delta)
        , m_mergeable(mergeable)
    {
    }

    ~RecordingCommand() override
    {
        ++m_state->destructions;
        m_state->calls.append(QStringLiteral("destroy:%1").arg(m_name));
    }

    void redo() override
    {
        m_state->value += m_delta;
        m_state->calls.append(QStringLiteral("redo:%1").arg(m_name));
    }

    void undo() override
    {
        m_state->value -= m_delta;
        m_state->calls.append(QStringLiteral("undo:%1").arg(m_name));
    }

    int id() const override
    {
        return m_mergeable ? 42 : -1;
    }

    bool mergeWith(const KUndo2Command *other) override
    {
        const auto *command = dynamic_cast<const RecordingCommand *>(other);
        if (!m_mergeable || !command || !command->m_mergeable) {
            return false;
        }

        m_delta += command->m_delta;
        ++m_state->merges;
        return true;
    }

private:
    CommandState *m_state;
    QString m_name;
    int m_delta;
    bool m_mergeable;
};

class AggregateProbe : public KisCommandUtils::AggregateCommand
{
public:
    explicit AggregateProbe(CommandState *state, KUndo2Command *parent = nullptr)
        : AggregateCommand(parent)
        , m_state(state)
    {
    }

    AggregateProbe(const KUndo2MagicString &text, CommandState *state, KUndo2Command *parent = nullptr)
        : AggregateCommand(text, parent)
        , m_state(state)
    {
    }

    int populationCount() const
    {
        return m_populationCount;
    }

protected:
    void populateChildCommands() override
    {
        ++m_populationCount;
        addCommand(nullptr);
        addCommand(new RecordingCommand(m_state, QStringLiteral("first"), 2));
        addCommand(new RecordingCommand(m_state, QStringLiteral("second"), 3));
    }

private:
    CommandState *m_state;
    int m_populationCount = 0;
};

class SkipFirstRedoProbe : public KisCommandUtils::SkipFirstRedoBase
{
public:
    SkipFirstRedoProbe(bool skipFirstRedo, CommandState *state, KUndo2Command *parent = nullptr)
        : SkipFirstRedoBase(skipFirstRedo, parent)
        , m_state(state)
    {
    }

    SkipFirstRedoProbe(bool skipFirstRedo,
                       const KUndo2MagicString &text,
                       CommandState *state,
                       KUndo2Command *parent = nullptr)
        : SkipFirstRedoBase(skipFirstRedo, text, parent)
        , m_state(state)
    {
    }

protected:
    void redoImpl() override
    {
        ++m_state->value;
        m_state->calls.append(QStringLiteral("redoImpl"));
    }

    void undoImpl() override
    {
        --m_state->value;
        m_state->calls.append(QStringLiteral("undoImpl"));
    }

private:
    CommandState *m_state;
};

class FlipFlopProbe : public KisCommandUtils::FlipFlopCommand
{
public:
    FlipFlopProbe(State state, QStringList *calls)
        : FlipFlopCommand(state)
        , m_calls(calls)
    {
    }

    FlipFlopProbe(bool finalizing, QStringList *calls)
        : FlipFlopCommand(finalizing)
        , m_calls(calls)
    {
    }

    State state() const
    {
        return getState();
    }

    bool firstRedo() const
    {
        return isFirstRedo();
    }

protected:
    void partA() override
    {
        m_calls->append(QStringLiteral("A"));
    }

    void partB() override
    {
        m_calls->append(QStringLiteral("B"));
    }

private:
    QStringList *m_calls;
};

} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

class KisCommandUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aggregatePopulatesOnceAndReplaysInUndoOrder();
    void lambdaConstructorsPopulateOnceAndReleaseFactories();
    void wrapperSkipsTheAlreadyAppliedFirstRedo();
    void skipBaseControlsOneRedoAtATime();
    void flipFlopDispatchesByItsStableInitialState();
    void compositeOwnsAndOrdersItsCommands();
    void compositionHandlesEmptySimpleAndExistingParents();
    void accumulationRedoesMergesAndOwnsCommands();
    void failedAccumulationDropsUndoCoverageDefect();
};

void KisCommandUtilsContractTest::aggregatePopulatesOnceAndReplaysInUndoOrder()
{
    CommandState state;
    {
        AggregateProbe command(&state);
        QCOMPARE(command.populationCount(), 0);

        command.redo();
        QCOMPARE(command.populationCount(), 1);
        QCOMPARE(state.value, 5);

        command.undo();
        QCOMPARE(state.value, 0);

        command.redo();
        QCOMPARE(command.populationCount(), 1);
        QCOMPARE(state.value, 5);
        QCOMPARE(state.calls,
                 QStringList({QStringLiteral("redo:first"),
                              QStringLiteral("redo:second"),
                              QStringLiteral("undo:second"),
                              QStringLiteral("undo:first"),
                              QStringLiteral("redo:first"),
                              QStringLiteral("redo:second")}));
    }
    QCOMPARE(state.destructions, 2);

    CommandState namedState;
    AggregateProbe named(kundo2_noi18n("named aggregate"), &namedState);
    QCOMPARE(named.text().toString(), QStringLiteral("named aggregate"));
    named.redo();
    QCOMPARE(namedState.value, 5);
}

void KisCommandUtilsContractTest::lambdaConstructorsPopulateOnceAndReleaseFactories()
{
    CommandState state;
    int factoryCalls = 0;

    auto capture = std::make_shared<int>(17);
    std::weak_ptr<int> weakCapture = capture;
    KisCommandUtils::LambdaCommand plain([capture, &factoryCalls, &state]() {
        ++factoryCalls;
        return new RecordingCommand(&state, QStringLiteral("plain"), 1);
    });
    capture.reset();
    QVERIFY(!weakCapture.expired());
    plain.redo();
    QCOMPARE(factoryCalls, 1);
    QVERIFY(weakCapture.expired());
    plain.undo();
    plain.redo();
    QCOMPARE(factoryCalls, 1);

    KisCommandUtils::LambdaCommand named(kundo2_noi18n("named"), [&factoryCalls, &state]() {
        ++factoryCalls;
        return new RecordingCommand(&state, QStringLiteral("named"), 2);
    });
    QCOMPARE(named.text().toString(), QStringLiteral("named"));
    named.redo();

    KUndo2Command parent;
    auto *parented = new KisCommandUtils::LambdaCommand(&parent, [&factoryCalls, &state]() {
        ++factoryCalls;
        return new RecordingCommand(&state, QStringLiteral("parented"), 4);
    });
    QVERIFY(parented->hasParent());
    parented->redo();

    KUndo2Command namedParent;
    auto *namedParented =
        new KisCommandUtils::LambdaCommand(kundo2_noi18n("named parented"), &namedParent, [&factoryCalls, &state]() {
            ++factoryCalls;
            return new RecordingCommand(&state, QStringLiteral("named-parented"), 8);
        });
    QVERIFY(namedParented->hasParent());
    QCOMPARE(namedParented->text().toString(), QStringLiteral("named parented"));
    namedParented->redo();

    QCOMPARE(factoryCalls, 4);
    QCOMPARE(state.value, 15);
}

void KisCommandUtilsContractTest::wrapperSkipsTheAlreadyAppliedFirstRedo()
{
    CommandState state;
    state.value = 5;

    {
        KisCommandUtils::SkipFirstRedoWrapper command(new RecordingCommand(&state, QStringLiteral("wrapped"), 5));
        QCOMPARE(command.text().toString(), QStringLiteral("wrapped"));

        command.redo();
        QCOMPARE(state.value, 5);

        command.undo();
        QCOMPARE(state.value, 0);

        command.redo();
        QCOMPARE(state.value, 5);
        QCOMPARE(state.calls, QStringList({QStringLiteral("undo:wrapped"), QStringLiteral("redo:wrapped")}));
    }
    QCOMPARE(state.destructions, 1);

    KisCommandUtils::SkipFirstRedoWrapper empty;
    QCOMPARE(empty.text().toString(), QStringLiteral("<bug: unnamed command>"));
    empty.redo();
    empty.undo();
}

void KisCommandUtilsContractTest::skipBaseControlsOneRedoAtATime()
{
    CommandState state;
    state.value = 1;
    SkipFirstRedoProbe skipped(true, &state);

    skipped.redo();
    QCOMPARE(state.value, 1);
    skipped.undo();
    QCOMPARE(state.value, 0);
    skipped.redo();
    QCOMPARE(state.value, 1);

    skipped.setSkipOneRedo(true);
    skipped.redo();
    QCOMPARE(state.value, 1);
    skipped.setSkipOneRedo(false);
    skipped.redo();
    QCOMPARE(state.value, 2);

    CommandState namedState;
    SkipFirstRedoProbe named(false, kundo2_noi18n("named base"), &namedState);
    QCOMPARE(named.text().toString(), QStringLiteral("named base"));
    named.redo();
    named.undo();
    QCOMPARE(namedState.value, 0);
}

void KisCommandUtilsContractTest::flipFlopDispatchesByItsStableInitialState()
{
    QCOMPARE(int(KisCommandUtils::FlipFlopCommand::INITIALIZING), 0);
    QCOMPARE(int(KisCommandUtils::FlipFlopCommand::FINALIZING), 1);

    QStringList initializingCalls;
    FlipFlopProbe initializing(KisCommandUtils::FlipFlopCommand::INITIALIZING, &initializingCalls);
    QCOMPARE(initializing.state(), KisCommandUtils::FlipFlopCommand::INITIALIZING);
    QVERIFY(initializing.firstRedo());
    initializing.redo();
    QVERIFY(!initializing.firstRedo());
    initializing.undo();
    QCOMPARE(initializingCalls, QStringList({QStringLiteral("A"), QStringLiteral("B")}));

    QStringList finalizingCalls;
    FlipFlopProbe finalizing(true, &finalizingCalls);
    QCOMPARE(finalizing.state(), KisCommandUtils::FlipFlopCommand::FINALIZING);
    finalizing.redo();
    finalizing.undo();
    QCOMPARE(finalizingCalls, QStringList({QStringLiteral("B"), QStringLiteral("A")}));

    QStringList defaultCalls;
    FlipFlopProbe defaultState(false, &defaultCalls);
    QCOMPARE(defaultState.state(), KisCommandUtils::FlipFlopCommand::INITIALIZING);
}

void KisCommandUtilsContractTest::compositeOwnsAndOrdersItsCommands()
{
    CommandState state;
    {
        KisCommandUtils::CompositeCommand command;
        command.addCommand(nullptr);
        command.addCommand(new RecordingCommand(&state, QStringLiteral("first"), 2));
        command.addCommand(new RecordingCommand(&state, QStringLiteral("second"), 3));

        command.redo();
        QCOMPARE(state.value, 5);
        command.undo();
        QCOMPARE(state.value, 0);
        QCOMPARE(state.calls,
                 QStringList({QStringLiteral("redo:first"),
                              QStringLiteral("redo:second"),
                              QStringLiteral("undo:second"),
                              QStringLiteral("undo:first")}));
    }
    QCOMPARE(state.destructions, 2);
}

void KisCommandUtilsContractTest::compositionHandlesEmptySimpleAndExistingParents()
{
    CommandState directState;
    auto *direct = new RecordingCommand(&directState, QStringLiteral("direct"), 1);
    QCOMPARE(KisCommandUtils::composeCommands(nullptr, direct), direct);
    delete direct;

    CommandState state;
    auto *parent = new RecordingCommand(&state, QStringLiteral("parent"), 2);
    auto *child = new RecordingCommand(&state, QStringLiteral("child"), 3);
    std::unique_ptr<KUndo2Command> composed(KisCommandUtils::composeCommands(parent, child));
    QVERIFY(composed.get() != parent);
    QCOMPARE(composed->text().toString(), QStringLiteral("parent"));

    auto *third = new RecordingCommand(&state, QStringLiteral("third"), 4);
    QCOMPARE(KisCommandUtils::composeCommands(composed.get(), third), composed.get());

    composed->redo();
    QCOMPARE(state.value, 9);
    composed->undo();
    QCOMPARE(state.value, 0);
    QCOMPARE(state.calls,
             QStringList({QStringLiteral("redo:parent"),
                          QStringLiteral("redo:child"),
                          QStringLiteral("redo:third"),
                          QStringLiteral("undo:third"),
                          QStringLiteral("undo:child"),
                          QStringLiteral("undo:parent")}));

    safeAssertCount = 0;
    std::unique_ptr<KUndo2Command> recovered(KisCommandUtils::composeCommands(nullptr, nullptr));
    QVERIFY(recovered);
    QCOMPARE(recovered->text().toString(), QStringLiteral("failed"));
    QCOMPARE(safeAssertCount, 1);
}

void KisCommandUtilsContractTest::accumulationRedoesMergesAndOwnsCommands()
{
    CommandState state;
    std::unique_ptr<KUndo2Command> accumulated;

    KisCommandUtils::redoAndMergeIntoAccumulatingCommand(new RecordingCommand(&state, QStringLiteral("first"), 2, true),
                                                         accumulated);
    QCOMPARE(state.value, 2);
    QVERIFY(accumulated);

    KisCommandUtils::redoAndMergeIntoAccumulatingCommand(
        new RecordingCommand(&state, QStringLiteral("second"), 3, true),
        accumulated);
    QCOMPARE(state.value, 5);
    QCOMPARE(state.merges, 1);
    QCOMPARE(state.destructions, 1);

    accumulated->undo();
    QCOMPARE(state.value, 0);
    accumulated->redo();
    QCOMPARE(state.value, 5);
    accumulated.reset();
    QCOMPARE(state.destructions, 2);
}

void KisCommandUtilsContractTest::failedAccumulationDropsUndoCoverageDefect()
{
    CommandState state;
    std::unique_ptr<KUndo2Command> accumulated;

    KisCommandUtils::redoAndMergeIntoAccumulatingCommand(new RecordingCommand(&state, QStringLiteral("first"), 2),
                                                         accumulated);
    safeAssertCount = 0;
    KisCommandUtils::redoAndMergeIntoAccumulatingCommand(new RecordingCommand(&state, QStringLiteral("unmerged"), 3),
                                                         accumulated);

    QCOMPARE(safeAssertCount, 1);
    QCOMPARE(state.value, 5);
    QCOMPARE(state.destructions, 1);

    accumulated->undo();
    QCOMPARE(state.value, 3);
}

QTEST_GUILESS_MAIN(KisCommandUtilsContractTest)

#include "KisCommandUtilsContractTest.moc"
