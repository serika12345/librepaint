/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QAbstractItemModel>
#include <QAction>
#include <QSignalSpy>
#include <QTest>

#include <memory>

#include <kundo2command.h>
#include <kundo2group.h>
#include <kundo2stack.h>
#include <undo/kis_document_undo_store.h>
#include <undo/kundo2view.h>

class NamedCountingCommand final : public KUndo2Command
{
public:
    NamedCountingCommand(int *value, int amount, const KUndo2MagicString &name)
        : KUndo2Command(name)
        , m_value(value)
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

class KisDocumentUndoUiTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void connectsDocumentHistoryToActionsAndPresentation();
    void switchesObservedStacks();
    void followsTheActiveUndoGroup();
    void storesPresentationProperties();
};

void KisDocumentUndoUiTest::connectsDocumentHistoryToActionsAndPresentation()
{
    KUndo2Stack stack;
    KisDocumentUndoStore store(&stack);
    KUndo2View view(&stack);
    int value = 0;

    view.setEmptyLabel(QStringLiteral("Document opened"));
    QSignalSpy historySpy(&store, &KisUndoStore::historyStateChanged);
    KUndo2QStack &qtStack = stack;
    std::unique_ptr<QAction> undoAction(qtStack.createUndoAction(this));
    std::unique_ptr<QAction> redoAction(qtStack.createRedoAction(this));

    QCOMPARE(view.model()->rowCount(), 1);
    QCOMPARE(view.model()->index(0, 0).data().toString(), QStringLiteral("Document opened"));
    QVERIFY(!undoAction->isEnabled());
    QVERIFY(!redoAction->isEnabled());

    store.addCommand(new NamedCountingCommand(&value, 3, kundo2_noi18n("Paint stroke")));

    QCOMPARE(value, 3);
    QCOMPARE(historySpy.count(), 1);
    QCOMPARE(view.model()->rowCount(), 2);
    QCOMPARE(view.model()->index(1, 0).data().toString(), QStringLiteral("Paint stroke"));
    QCOMPARE(view.selectionModel()->currentIndex().row(), 1);
    QVERIFY(undoAction->isEnabled());
    QVERIFY(undoAction->text().contains(QStringLiteral("Paint stroke")));
    QVERIFY(!redoAction->isEnabled());

    undoAction->trigger();

    QCOMPARE(value, 0);
    QCOMPARE(view.selectionModel()->currentIndex().row(), 0);
    QVERIFY(!undoAction->isEnabled());
    QVERIFY(redoAction->isEnabled());
    QVERIFY(redoAction->text().contains(QStringLiteral("Paint stroke")));

    view.selectionModel()->setCurrentIndex(view.model()->index(1, 0), QItemSelectionModel::ClearAndSelect);

    QCOMPARE(value, 3);
    QCOMPARE(view.selectionModel()->currentIndex().row(), 1);
    QVERIFY(undoAction->isEnabled());
    QVERIFY(!redoAction->isEnabled());
}

void KisDocumentUndoUiTest::switchesObservedStacks()
{
    KUndo2Stack stack;
    KUndo2View view;

    QVERIFY(!view.stack());
    QVERIFY(!view.group());

    view.setStack(&stack);
    QCOMPARE(view.stack(), static_cast<KUndo2QStack *>(&stack));
    QVERIFY(!view.group());

    view.setStack(nullptr);
    QVERIFY(!view.stack());
}

void KisDocumentUndoUiTest::followsTheActiveUndoGroup()
{
    KUndo2Stack firstStack;
    KUndo2Stack secondStack;
    KUndo2Group group;
    group.addStack(&firstStack);
    group.addStack(&secondStack);
    group.setActiveStack(&firstStack);

    KUndo2View view(&group);
    QCOMPARE(view.group(), &group);
    QCOMPARE(view.stack(), static_cast<KUndo2QStack *>(&firstStack));

    group.setActiveStack(&secondStack);
    QCOMPARE(view.stack(), static_cast<KUndo2QStack *>(&secondStack));

    view.setGroup(nullptr);
    QVERIFY(!view.group());
    QVERIFY(!view.stack());
}

void KisDocumentUndoUiTest::storesPresentationProperties()
{
    KUndo2View view;
    const QString label = QStringLiteral("Document opened");
    QPixmap pixmap(4, 4);
    pixmap.fill(Qt::green);
    const QIcon icon(pixmap);

    view.setEmptyLabel(label);
    view.setCleanIcon(icon);

    QCOMPARE(view.emptyLabel(), label);
    QCOMPARE(view.cleanIcon().cacheKey(), icon.cacheKey());
}

QTEST_MAIN(KisDocumentUndoUiTest)

#include "kis_document_undo_ui_test.moc"
