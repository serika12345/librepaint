/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QPointer>
#include <QTest>

#include <kundo2command.h>
#include <kundo2stack.h>
#include <undo/kis_document_undo_store.h>

class CountingCommand final : public KUndo2Command
{
public:
    CountingCommand(int *value, int amount)
        : m_value(value)
        , m_amount(amount)
    {
    }

    void redo() override
    {
        *m_value += m_amount;
    }

    void undo() override
    {
        *m_value -= m_amount;
    }

private:
    int *m_value;
    int m_amount;
};

class KisDocumentUndoStoreTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void forwardsHistoryOperations();
    void groupsMacrosAndPurgesRedoHistory();
    void forwardsChangesSynchronouslyWithoutOwningTheStack();
};

void KisDocumentUndoStoreTest::forwardsHistoryOperations()
{
    KUndo2Stack stack;
    KisDocumentUndoStore store(&stack);
    int value = 0;

    QVERIFY(!store.presentCommand());

    auto *command = new CountingCommand(&value, 2);
    store.addCommand(command);

    QCOMPARE(value, 2);
    QCOMPARE(store.presentCommand(), command);

    store.addCommand(nullptr);
    QCOMPARE(stack.count(), 1);

    store.undoLastCommand();
    QCOMPARE(value, 0);
    QVERIFY(!store.presentCommand());
}

void KisDocumentUndoStoreTest::groupsMacrosAndPurgesRedoHistory()
{
    KUndo2Stack stack;
    KisDocumentUndoStore store(&stack);
    int value = 0;

    store.beginMacro(kundo2_noi18n("two changes"));
    store.addCommand(new CountingCommand(&value, 2));
    store.addCommand(new CountingCommand(&value, 3));
    store.endMacro();

    QCOMPARE(value, 5);
    QCOMPARE(stack.count(), 1);

    store.undoLastCommand();
    QCOMPARE(value, 0);
    QCOMPARE(stack.count(), 1);

    store.purgeRedoState();
    QCOMPARE(stack.count(), 0);
}

void KisDocumentUndoStoreTest::forwardsChangesSynchronouslyWithoutOwningTheStack()
{
    auto *stack = new KUndo2Stack;
    QPointer<KUndo2Stack> stackGuard(stack);
    auto *store = new KisDocumentUndoStore(stack);
    bool notified = false;
    int value = 0;

    QObject::connect(store,
                     &KisUndoStore::historyStateChanged,
                     store,
                     [&notified]() {
                         notified = true;
                     },
                     Qt::DirectConnection);

    store->addCommand(new CountingCommand(&value, 1));
    QVERIFY(notified);
    QCOMPARE(store->thread(), stack->thread());

    delete store;
    QVERIFY(stackGuard);
    delete stack;
}

QTEST_GUILESS_MAIN(KisDocumentUndoStoreTest)

#include "kis_document_undo_store_test.moc"
