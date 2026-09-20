/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_undo_adapter.h"

#include <QSignalSpy>
#include <QTest>

namespace
{
struct StoreToken {
};

struct CommandToken {
};

struct MacroNameToken {
};

class UndoAdapterProbe : public KisUndoAdapter
{
public:
    UndoAdapterProbe(KisUndoStore *store, QObject *parent = nullptr, int *destructionCount = nullptr)
        : KisUndoAdapter(store, parent)
        , destructionCount(destructionCount)
    {
    }

    ~UndoAdapterProbe() override
    {
        if (destructionCount) {
            ++*destructionCount;
        }
    }

    const KUndo2Command *presentCommand() override
    {
        calls.append(QStringLiteral("present"));
        return presentResult;
    }

    void undoLastCommand() override
    {
        calls.append(QStringLiteral("undoLast"));
    }

    void addCommand(KUndo2Command *command) override
    {
        calls.append(QStringLiteral("add"));
        addedCommand = command;
    }

    void beginMacro(const KUndo2MagicString &macroName) override
    {
        calls.append(QStringLiteral("beginMacro"));
        receivedMacroName = &macroName;
    }

    void endMacro() override
    {
        calls.append(QStringLiteral("endMacro"));
    }

    KisUndoStore *currentStore()
    {
        return undoStore();
    }

    QStringList calls;
    const KUndo2Command *presentResult = nullptr;
    KUndo2Command *addedCommand = nullptr;
    const KUndo2MagicString *receivedMacroName = nullptr;
    int *destructionCount = nullptr;
};
} // namespace

class KisUndoAdapterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionKeepsBorrowedStoreAndQObjectParent();
    void setUndoStoreReplacesBorrowedStore();
    void abstractOperationsDispatchThroughBase();
    void selectionSignalAndVirtualLifetime();
};

void KisUndoAdapterContractTest::constructionKeepsBorrowedStoreAndQObjectParent()
{
    QObject parent;
    StoreToken storeToken;
    auto *store = reinterpret_cast<KisUndoStore *>(&storeToken);
    auto *adapter = new UndoAdapterProbe(store, &parent);

    QCOMPARE(adapter->parent(), &parent);
    QCOMPARE(adapter->currentStore(), store);
    delete adapter;
}

void KisUndoAdapterContractTest::setUndoStoreReplacesBorrowedStore()
{
    StoreToken firstStoreToken;
    StoreToken secondStoreToken;
    auto *firstStore = reinterpret_cast<KisUndoStore *>(&firstStoreToken);
    auto *secondStore = reinterpret_cast<KisUndoStore *>(&secondStoreToken);
    UndoAdapterProbe adapter(firstStore);

    QCOMPARE(adapter.currentStore(), firstStore);
    adapter.setUndoStore(secondStore);
    QCOMPARE(adapter.currentStore(), secondStore);
}

void KisUndoAdapterContractTest::abstractOperationsDispatchThroughBase()
{
    StoreToken storeToken;
    CommandToken commandToken;
    CommandToken resultToken;
    MacroNameToken macroNameToken;
    UndoAdapterProbe adapter(reinterpret_cast<KisUndoStore *>(&storeToken));
    KisUndoAdapter &base = adapter;
    auto *command = reinterpret_cast<KUndo2Command *>(&commandToken);
    auto *result = reinterpret_cast<KUndo2Command *>(&resultToken);
    auto *macroName = reinterpret_cast<KUndo2MagicString *>(&macroNameToken);
    adapter.presentResult = result;

    QCOMPARE(base.presentCommand(), result);
    base.undoLastCommand();
    base.addCommand(command);
    base.beginMacro(*macroName);
    base.endMacro();

    QCOMPARE(adapter.addedCommand, command);
    QCOMPARE(adapter.receivedMacroName, macroName);
    QCOMPARE(adapter.calls,
             QStringList({QStringLiteral("present"),
                          QStringLiteral("undoLast"),
                          QStringLiteral("add"),
                          QStringLiteral("beginMacro"),
                          QStringLiteral("endMacro")}));
}

void KisUndoAdapterContractTest::selectionSignalAndVirtualLifetime()
{
    StoreToken storeToken;
    int destructionCount = 0;
    KisUndoAdapter *adapter =
        new UndoAdapterProbe(reinterpret_cast<KisUndoStore *>(&storeToken), nullptr, &destructionCount);
    QSignalSpy selectionSpy(adapter, &KisUndoAdapter::selectionChanged);

    adapter->emitSelectionChanged();
    QCOMPARE(selectionSpy.count(), 1);

    delete adapter;
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisUndoAdapterContractTest)

#include "KisUndoAdapterContractTest.moc"
