/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPostExecutionUndoAdapterDelivery_p.h"
#include "kis_post_execution_undo_adapter.h"

#include <QTest>

namespace
{
struct StoreToken {
};
struct FacadeToken {
};
struct CommandToken {
};
struct SavedCommandToken {
};
struct MacroToken {
};
struct MacroNameToken {
};

struct DeliveryState {
    KUndo2CommandSP command;
    KisStrokesFacade *facade = nullptr;
    KisUndoStore *store = nullptr;
    KUndo2Command *storedCommand = nullptr;
    const KUndo2MagicString *macroName = nullptr;
    int savedCommandCreations = 0;
    int macroCreations = 0;
    int storeDeliveries = 0;

    void reset()
    {
        command.clear();
        facade = nullptr;
        store = nullptr;
        storedCommand = nullptr;
        macroName = nullptr;
        savedCommandCreations = 0;
        macroCreations = 0;
        storeDeliveries = 0;
    }
};

DeliveryState deliveryState;
SavedCommandToken savedCommandToken;
MacroToken macroToken;
} // namespace

KUndo2Command *KisPostExecutionUndoAdapterDelivery::createSavedCommand(KUndo2CommandSP command,
                                                                       KisStrokesFacade *strokesFacade)
{
    ++deliveryState.savedCommandCreations;
    deliveryState.command = command;
    deliveryState.facade = strokesFacade;
    return reinterpret_cast<KUndo2Command *>(&savedCommandToken);
}

KisSavedMacroCommand *KisPostExecutionUndoAdapterDelivery::createSavedMacro(const KUndo2MagicString &macroName,
                                                                            KisStrokesFacade *strokesFacade)
{
    ++deliveryState.macroCreations;
    deliveryState.macroName = &macroName;
    deliveryState.facade = strokesFacade;
    return reinterpret_cast<KisSavedMacroCommand *>(&macroToken);
}

void KisPostExecutionUndoAdapterDelivery::addCommand(KisUndoStore *undoStore, KUndo2Command *command)
{
    ++deliveryState.storeDeliveries;
    deliveryState.store = undoStore;
    deliveryState.storedCommand = command;
}

void KisPostExecutionUndoAdapterDelivery::addMacro(KisUndoStore *undoStore, KisSavedMacroCommand *macro)
{
    ++deliveryState.storeDeliveries;
    deliveryState.store = undoStore;
    deliveryState.storedCommand = reinterpret_cast<KUndo2Command *>(macro);
}

class KisPostExecutionUndoAdapterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void constructionKeepsBorrowedStoreAndFacade();
    void addCommandWrapsSharedOwnershipAndForwardsOnce();
    void setUndoStoreRedirectsSubsequentCommands();
    void createAndAddMacroPreserveNameFacadeAndPointer();
};

void KisPostExecutionUndoAdapterContractTest::init()
{
    deliveryState.reset();
}

void KisPostExecutionUndoAdapterContractTest::constructionKeepsBorrowedStoreAndFacade()
{
    StoreToken storeToken;
    FacadeToken facadeToken;
    auto *store = reinterpret_cast<KisUndoStore *>(&storeToken);
    auto *facade = reinterpret_cast<KisStrokesFacade *>(&facadeToken);

    KisPostExecutionUndoAdapter adapter(store, facade);

    QCOMPARE(adapter.strokesFacade(), facade);
    QCOMPARE(deliveryState.storeDeliveries, 0);
}

void KisPostExecutionUndoAdapterContractTest::addCommandWrapsSharedOwnershipAndForwardsOnce()
{
    StoreToken storeToken;
    FacadeToken facadeToken;
    CommandToken commandToken;
    int deletionCount = 0;
    auto *store = reinterpret_cast<KisUndoStore *>(&storeToken);
    auto *facade = reinterpret_cast<KisStrokesFacade *>(&facadeToken);
    KisPostExecutionUndoAdapter adapter(store, facade);
    KUndo2CommandSP command(reinterpret_cast<KUndo2Command *>(&commandToken), [&deletionCount](KUndo2Command *) {
        ++deletionCount;
    });

    adapter.addCommand(command);
    QCOMPARE(deliveryState.savedCommandCreations, 1);
    QCOMPARE(deliveryState.storeDeliveries, 1);
    QCOMPARE(deliveryState.facade, facade);
    QCOMPARE(deliveryState.store, store);
    QCOMPARE(deliveryState.storedCommand, reinterpret_cast<KUndo2Command *>(&savedCommandToken));

    command.clear();
    QCOMPARE(deletionCount, 0);
    deliveryState.command.clear();
    QCOMPARE(deletionCount, 1);

    adapter.addCommand(KUndo2CommandSP());
    QCOMPARE(deliveryState.savedCommandCreations, 1);
    QCOMPARE(deliveryState.storeDeliveries, 1);
}

void KisPostExecutionUndoAdapterContractTest::setUndoStoreRedirectsSubsequentCommands()
{
    StoreToken firstStoreToken;
    StoreToken secondStoreToken;
    FacadeToken facadeToken;
    MacroToken commandToken;
    auto *firstStore = reinterpret_cast<KisUndoStore *>(&firstStoreToken);
    auto *secondStore = reinterpret_cast<KisUndoStore *>(&secondStoreToken);
    KisPostExecutionUndoAdapter adapter(firstStore, reinterpret_cast<KisStrokesFacade *>(&facadeToken));

    adapter.addMacro(reinterpret_cast<KisSavedMacroCommand *>(&commandToken));
    QCOMPARE(deliveryState.store, firstStore);
    adapter.setUndoStore(secondStore);
    adapter.addMacro(reinterpret_cast<KisSavedMacroCommand *>(&commandToken));

    QCOMPARE(deliveryState.store, secondStore);
    QCOMPARE(deliveryState.storeDeliveries, 2);
}

void KisPostExecutionUndoAdapterContractTest::createAndAddMacroPreserveNameFacadeAndPointer()
{
    StoreToken storeToken;
    FacadeToken facadeToken;
    MacroNameToken macroNameToken;
    auto *store = reinterpret_cast<KisUndoStore *>(&storeToken);
    auto *facade = reinterpret_cast<KisStrokesFacade *>(&facadeToken);
    auto *macroName = reinterpret_cast<KUndo2MagicString *>(&macroNameToken);
    KisPostExecutionUndoAdapter adapter(store, facade);

    KisSavedMacroCommand *macro = adapter.createMacro(*macroName);
    QCOMPARE(macro, reinterpret_cast<KisSavedMacroCommand *>(&macroToken));
    QCOMPARE(deliveryState.macroName, macroName);
    QCOMPARE(deliveryState.facade, facade);
    QCOMPARE(deliveryState.macroCreations, 1);

    adapter.addMacro(macro);
    QCOMPARE(deliveryState.store, store);
    QCOMPARE(deliveryState.storedCommand, reinterpret_cast<KUndo2Command *>(macro));
    QCOMPARE(deliveryState.storeDeliveries, 1);
}

QTEST_GUILESS_MAIN(KisPostExecutionUndoAdapterContractTest)

#include "KisPostExecutionUndoAdapterContractTest.moc"
