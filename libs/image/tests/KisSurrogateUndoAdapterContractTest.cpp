/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <memory>
#include <type_traits>
#include <utility>

#include <QStringList>
#include <QTest>

#include "kis_surrogate_undo_adapter.h"
#include <kundo2stack.h>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace
{
struct CommandState {
    int value{0};
    int destructions{0};
    QStringList calls;
};

class TrackedCommand final : public KUndo2Command
{
public:
    TrackedCommand(CommandState *state, QString name, int delta)
        : KUndo2Command(kundo2_noi18n(name))
        , m_state(state)
        , m_name(std::move(name))
        , m_delta(delta)
    {
    }

    ~TrackedCommand() override
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

private:
    CommandState *m_state;
    QString m_name;
    int m_delta;
};
} // namespace

class KisSurrogateUndoAdapterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeLifetimeAndEmptyHistoryRemainStable();
    void commandAdditionAndLastUndoRemainStable();
    void singleStepUndoAndRedoRemainStable();
    void macroGroupsCommandsIntoOneHistoryStep();
    void wholeHistoryUndoAndRedoRemainStable();
};

void KisSurrogateUndoAdapterContractTest::typeLifetimeAndEmptyHistoryRemainStable()
{
    static_assert(std::is_base_of_v<KisUndoAdapter, KisSurrogateUndoAdapter>);
    static_assert(std::is_default_constructible_v<KisSurrogateUndoAdapter>);
    static_assert(std::has_virtual_destructor_v<KisSurrogateUndoAdapter>);

    std::unique_ptr<KisUndoAdapter> adapter = std::make_unique<KisSurrogateUndoAdapter>();
    QVERIFY(!adapter->presentCommand());
}

void KisSurrogateUndoAdapterContractTest::commandAdditionAndLastUndoRemainStable()
{
    CommandState state;

    {
        KisSurrogateUndoAdapter adapter;
        auto *command = new TrackedCommand(&state, QStringLiteral("first"), 3);

        adapter.addCommand(command);
        QCOMPARE(state.value, 3);
        QCOMPARE(state.calls, QStringList({QStringLiteral("redo:first")}));
        QCOMPARE(adapter.presentCommand(), command);
        QCOMPARE(state.destructions, 0);

        adapter.undoLastCommand();
        QCOMPARE(state.value, 0);
        QVERIFY(!adapter.presentCommand());
        QCOMPARE(state.calls, QStringList({QStringLiteral("redo:first"), QStringLiteral("undo:first")}));
    }

    QCOMPARE(state.destructions, 1);
}

void KisSurrogateUndoAdapterContractTest::singleStepUndoAndRedoRemainStable()
{
    CommandState state;
    KisSurrogateUndoAdapter adapter;
    auto *first = new TrackedCommand(&state, QStringLiteral("one"), 1);
    auto *second = new TrackedCommand(&state, QStringLiteral("two"), 2);

    adapter.addCommand(first);
    adapter.addCommand(second);
    QCOMPARE(state.value, 3);
    QCOMPARE(adapter.presentCommand(), second);

    state.calls.clear();
    adapter.undo();
    QCOMPARE(state.value, 1);
    QCOMPARE(adapter.presentCommand(), first);

    adapter.redo();
    QCOMPARE(state.value, 3);
    QCOMPARE(adapter.presentCommand(), second);
    QCOMPARE(state.calls, QStringList({QStringLiteral("undo:two"), QStringLiteral("redo:two")}));
}

void KisSurrogateUndoAdapterContractTest::macroGroupsCommandsIntoOneHistoryStep()
{
    CommandState state;
    KisSurrogateUndoAdapter adapter;
    auto *first = new TrackedCommand(&state, QStringLiteral("first"), 2);
    auto *second = new TrackedCommand(&state, QStringLiteral("second"), 3);

    adapter.beginMacro(kundo2_noi18n("two changes"));
    adapter.addCommand(first);
    adapter.addCommand(second);
    adapter.endMacro();

    QCOMPARE(state.value, 5);
    const KUndo2Command *macro = adapter.presentCommand();
    QVERIFY(macro);
    QCOMPARE(macro->text().toString(), QStringLiteral("two changes"));
    QCOMPARE(macro->childCount(), 2);
    QCOMPARE(macro->child(0), first);
    QCOMPARE(macro->child(1), second);

    state.calls.clear();
    adapter.undo();
    QCOMPARE(state.value, 0);
    QCOMPARE(state.calls, QStringList({QStringLiteral("undo:second"), QStringLiteral("undo:first")}));

    adapter.redo();
    QCOMPARE(state.value, 5);
    QCOMPARE(state.calls,
             QStringList({QStringLiteral("undo:second"),
                          QStringLiteral("undo:first"),
                          QStringLiteral("redo:first"),
                          QStringLiteral("redo:second")}));
}

void KisSurrogateUndoAdapterContractTest::wholeHistoryUndoAndRedoRemainStable()
{
    CommandState state;
    KisSurrogateUndoAdapter adapter;
    adapter.addCommand(new TrackedCommand(&state, QStringLiteral("one"), 1));
    adapter.addCommand(new TrackedCommand(&state, QStringLiteral("two"), 2));
    auto *third = new TrackedCommand(&state, QStringLiteral("three"), 4);
    adapter.addCommand(third);
    QCOMPARE(state.value, 7);

    state.calls.clear();
    adapter.undoAll();
    QCOMPARE(state.value, 0);
    QVERIFY(!adapter.presentCommand());
    QCOMPARE(state.calls,
             QStringList({QStringLiteral("undo:three"), QStringLiteral("undo:two"), QStringLiteral("undo:one")}));

    adapter.redoAll();
    QCOMPARE(state.value, 7);
    QCOMPARE(adapter.presentCommand(), third);
    QCOMPARE(state.calls,
             QStringList({QStringLiteral("undo:three"),
                          QStringLiteral("undo:two"),
                          QStringLiteral("undo:one"),
                          QStringLiteral("redo:one"),
                          QStringLiteral("redo:two"),
                          QStringLiteral("redo:three")}));
}

QTEST_GUILESS_MAIN(KisSurrogateUndoAdapterContractTest)

#include "KisSurrogateUndoAdapterContractTest.moc"
