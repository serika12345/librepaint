/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <memory>
#include <utility>

#include <QSignalSpy>
#include <QStringList>
#include <QTest>

#include <kis_undo_stores.h>
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

class KisUndoStoresContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void surrogateStartsEmptyExecutesImmediatelyAndOwnsCommands();
    void surrogateNavigatesSingleStepsAndWholeHistory();
    void surrogateGroupsMacrosAndPurgesTheRedoBranch();
    void surrogateClearDropsHistoryWithoutUndoingAndDestructionStaysSilent();
    void dumbStoreExecutesAndDiscardsThroughTheBaseInterface();
};

void KisUndoStoresContractTest::surrogateStartsEmptyExecutesImmediatelyAndOwnsCommands()
{
    CommandState state;
    int notifications = 0;
    QObject observer;

    {
        auto store = std::make_unique<KisSurrogateUndoStore>();
        QObject::connect(
            store.get(),
            &KisUndoStore::historyStateChanged,
            &observer,
            [&notifications]() {
                ++notifications;
            },
            Qt::DirectConnection);

        QVERIFY(!store->presentCommand());

        store->addCommand(nullptr);
        QCOMPARE(notifications, 0);

        auto *command = new TrackedCommand(&state, QStringLiteral("first"), 2);
        store->addCommand(command);

        QCOMPARE(state.value, 2);
        QCOMPARE(state.calls, QStringList({QStringLiteral("redo:first")}));
        QCOMPARE(store->presentCommand(), command);
        QCOMPARE(notifications, 1);
        QCOMPARE(state.destructions, 0);
    }

    QCOMPARE(state.value, 2);
    QCOMPARE(state.destructions, 1);
    QCOMPARE(notifications, 1);
}

void KisUndoStoresContractTest::surrogateNavigatesSingleStepsAndWholeHistory()
{
    CommandState state;
    KisSurrogateUndoStore store;

    store.addCommand(new TrackedCommand(&state, QStringLiteral("one"), 1));
    store.addCommand(new TrackedCommand(&state, QStringLiteral("two"), 2));
    store.addCommand(new TrackedCommand(&state, QStringLiteral("three"), 4));
    QCOMPARE(state.value, 7);

    state.calls.clear();
    store.undoLastCommand();
    QCOMPARE(state.value, 3);

    store.undo();
    QCOMPARE(state.value, 1);

    store.redo();
    QCOMPARE(state.value, 3);

    store.undoAll();
    QCOMPARE(state.value, 0);
    QVERIFY(!store.presentCommand());

    store.redoAll();
    QCOMPARE(state.value, 7);

    QCOMPARE(state.calls,
             QStringList({QStringLiteral("undo:three"),
                          QStringLiteral("undo:two"),
                          QStringLiteral("redo:two"),
                          QStringLiteral("undo:two"),
                          QStringLiteral("undo:one"),
                          QStringLiteral("redo:one"),
                          QStringLiteral("redo:two"),
                          QStringLiteral("redo:three")}));
}

void KisUndoStoresContractTest::surrogateGroupsMacrosAndPurgesTheRedoBranch()
{
    CommandState state;
    KisSurrogateUndoStore store;
    QSignalSpy historySpy(&store, &KisUndoStore::historyStateChanged);

    store.beginMacro(kundo2_noi18n("two changes"));
    auto *first = new TrackedCommand(&state, QStringLiteral("first"), 2);
    auto *second = new TrackedCommand(&state, QStringLiteral("second"), 3);
    store.addCommand(first);
    store.addCommand(second);

    QCOMPARE(state.value, 5);
    QCOMPARE(historySpy.count(), 0);

    store.endMacro();
    QCOMPARE(historySpy.count(), 1);

    const KUndo2Command *macro = store.presentCommand();
    QVERIFY(macro);
    QCOMPARE(macro->childCount(), 2);
    QCOMPARE(macro->child(0), first);
    QCOMPARE(macro->child(1), second);

    store.undo();
    QCOMPARE(state.value, 0);
    QCOMPARE(historySpy.count(), 2);

    store.redo();
    QCOMPARE(state.value, 5);
    QCOMPARE(historySpy.count(), 3);

    state.calls.clear();
    store.undo();
    QCOMPARE(state.calls, QStringList({QStringLiteral("undo:second"), QStringLiteral("undo:first")}));
    QCOMPARE(historySpy.count(), 4);

    store.purgeRedoState();
    QVERIFY(!store.presentCommand());
    QCOMPARE(state.destructions, 2);
    QCOMPARE(historySpy.count(), 4);

    store.redo();
    QCOMPARE(state.value, 0);
    QCOMPARE(historySpy.count(), 4);
}

void KisUndoStoresContractTest::surrogateClearDropsHistoryWithoutUndoingAndDestructionStaysSilent()
{
    CommandState state;
    int notifications = 0;
    QObject observer;

    auto store = std::make_unique<KisSurrogateUndoStore>();
    QObject::connect(
        store.get(),
        &KisUndoStore::historyStateChanged,
        &observer,
        [&notifications]() {
            ++notifications;
        },
        Qt::DirectConnection);

    store->addCommand(new TrackedCommand(&state, QStringLiteral("cleared"), 5));
    QCOMPARE(notifications, 1);

    store->clear();
    QCOMPARE(state.value, 5);
    QCOMPARE(state.destructions, 1);
    QVERIFY(!store->presentCommand());
    QCOMPARE(notifications, 2);

    store->clear();
    QCOMPARE(notifications, 2);

    store->addCommand(new TrackedCommand(&state, QStringLiteral("remaining"), 2));
    QCOMPARE(notifications, 3);

    store.reset();
    QCOMPARE(state.value, 7);
    QCOMPARE(state.destructions, 2);
    QCOMPARE(notifications, 3);
}

void KisUndoStoresContractTest::dumbStoreExecutesAndDiscardsThroughTheBaseInterface()
{
    CommandState state;
    std::unique_ptr<KisUndoStore> store = std::make_unique<KisDumbUndoStore>();
    QSignalSpy historySpy(store.get(), &KisUndoStore::historyStateChanged);

    QVERIFY(!store->presentCommand());

    store->addCommand(new TrackedCommand(&state, QStringLiteral("immediate"), 4));
    QCOMPARE(state.value, 4);
    QCOMPARE(state.destructions, 1);
    QCOMPARE(state.calls, QStringList({QStringLiteral("redo:immediate"), QStringLiteral("destroy:immediate")}));
    QCOMPARE(historySpy.count(), 1);
    QVERIFY(!store->presentCommand());

    store->beginMacro(kundo2_noi18n("ignored"));
    QCOMPARE(historySpy.count(), 1);

    store->endMacro();
    QCOMPARE(historySpy.count(), 2);

    store->undoLastCommand();
    QCOMPARE(historySpy.count(), 3);
    QCOMPARE(state.value, 4);

    store->purgeRedoState();
    QCOMPARE(historySpy.count(), 3);
    QCOMPARE(state.value, 4);
}

QTEST_GUILESS_MAIN(KisUndoStoresContractTest)

#include "KisUndoStoresContractTest.moc"
