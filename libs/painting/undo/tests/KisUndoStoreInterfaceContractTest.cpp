/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_undo_store.h"

#include <QSignalSpy>
#include <QTest>

#include <memory>
#include <type_traits>

class KUndo2MagicString
{
};

namespace
{
class RecordingUndoStore final : public KisUndoStore
{
public:
    enum Call {
        Present,
        Add,
        Begin,
        Undo,
        End,
        Purge,
    };

    explicit RecordingUndoStore(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingUndoStore() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    const KUndo2Command *presentCommand() override
    {
        calls.append(Present);
        return presentResult;
    }

    void undoLastCommand() override
    {
        calls.append(Undo);
    }

    void addCommand(KUndo2Command *command) override
    {
        calls.append(Add);
        addedCommand = command;
    }

    void beginMacro(const KUndo2MagicString &macroName) override
    {
        calls.append(Begin);
        begunMacro = &macroName;
    }

    void endMacro() override
    {
        calls.append(End);
    }

    void purgeRedoState() override
    {
        calls.append(Purge);
    }

    void notifyHistoryStateChanged()
    {
        Q_EMIT historyStateChanged();
    }

    QList<Call> calls;
    const KUndo2Command *presentResult{nullptr};
    KUndo2Command *addedCommand{nullptr};
    const KUndo2MagicString *begunMacro{nullptr};

private:
    int *m_destructionCount;
};

class KisUndoStoreInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionAndHistorySignalRemainObservable();
    void commandPointersDispatchWithoutOwnership();
    void macroAndHistoryOperationsPreserveReferenceAndOrder();
    void baseOwnershipHasVirtualLifetime();
};

void KisUndoStoreInterfaceContractTest::constructionAndHistorySignalRemainObservable()
{
    static_assert(std::is_abstract_v<KisUndoStore>);
    static_assert(std::is_polymorphic_v<KisUndoStore>);
    static_assert(std::has_virtual_destructor_v<KisUndoStore>);

    RecordingUndoStore store;
    QVERIFY(!store.parent());

    QSignalSpy historySpy(&store, &KisUndoStore::historyStateChanged);
    store.notifyHistoryStateChanged();
    store.notifyHistoryStateChanged();

    QCOMPARE(historySpy.count(), 2);
}

void KisUndoStoreInterfaceContractTest::commandPointersDispatchWithoutOwnership()
{
    char commandToken = 0;
    auto *command = reinterpret_cast<KUndo2Command *>(&commandToken);

    RecordingUndoStore store;
    store.presentResult = command;
    KisUndoStore &interface = store;

    QCOMPARE(interface.presentCommand(), command);
    interface.addCommand(command);

    QCOMPARE(store.addedCommand, command);
    QCOMPARE(store.calls, QList<RecordingUndoStore::Call>({RecordingUndoStore::Present, RecordingUndoStore::Add}));
}

void KisUndoStoreInterfaceContractTest::macroAndHistoryOperationsPreserveReferenceAndOrder()
{
    KUndo2MagicString macroName;

    RecordingUndoStore store;
    KisUndoStore &interface = store;

    interface.beginMacro(macroName);
    interface.undoLastCommand();
    interface.endMacro();
    interface.purgeRedoState();

    QCOMPARE(store.begunMacro, &macroName);
    QCOMPARE(
        store.calls,
        QList<RecordingUndoStore::Call>(
            {RecordingUndoStore::Begin, RecordingUndoStore::Undo, RecordingUndoStore::End, RecordingUndoStore::Purge}));
}

void KisUndoStoreInterfaceContractTest::baseOwnershipHasVirtualLifetime()
{
    int destructionCount = 0;
    {
        std::unique_ptr<KisUndoStore> store = std::make_unique<RecordingUndoStore>(&destructionCount);
        QCOMPARE(destructionCount, 0);
    }
    QCOMPARE(destructionCount, 1);
}
} // namespace

QTEST_GUILESS_MAIN(KisUndoStoreInterfaceContractTest)

#include "KisUndoStoreInterfaceContractTest.moc"
