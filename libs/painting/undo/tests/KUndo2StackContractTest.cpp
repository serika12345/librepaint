/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <memory>
#include <utility>

#include <QAction>
#include <QColor>
#include <QIcon>
#include <QPointer>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>

#include <KisCumulativeUndoData.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>
#include <kundo2commandextradata.h>
#include <kundo2group.h>
#include <kundo2stack.h>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

class KisKActionCollection : public QObject
{
public:
    using QObject::QObject;

    QAction *addAction(const QString &name, QAction *action);

    QStringList names;
    QList<QAction *> actions;
};

QAction *KisKActionCollection::addAction(const QString &name, QAction *action)
{
    names.append(name);
    actions.append(action);
    action->setObjectName(name);
    return action;
}

namespace KisIconUtils
{
QStringList requestedIcons;

QIcon loadIcon(const QString &name)
{
    requestedIcons.append(name);
    QPixmap image(2, 2);
    image.fill(Qt::black);
    return QIcon(image);
}
} // namespace KisIconUtils

namespace KStandardAction
{
const char *name(StandardAction id)
{
    return id == Undo ? "edit_undo" : "edit_redo";
}
} // namespace KStandardAction

namespace
{
struct CommandState {
    int value{0};
    int destructions{0};
    int merges{0};
    QStringList calls;
};

class RecordingCommand : public KUndo2Command
{
public:
    RecordingCommand(CommandState *state, QString name, int delta, KUndo2Command *parent = nullptr)
        : KUndo2Command(kundo2_noi18n(name), parent)
        , m_state(state)
        , m_name(std::move(name))
        , m_delta(delta)
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

protected:
    CommandState *m_state;
    QString m_name;
    int m_delta;
};

class MergeCommand final : public RecordingCommand
{
public:
    MergeCommand(CommandState *state, QString name, int delta, bool annihilate = false)
        : RecordingCommand(state, std::move(name), delta)
        , m_annihilate(annihilate)
    {
    }

    int id() const override
    {
        return 41;
    }

    bool mergeWith(const KUndo2Command *other) override
    {
        const auto *command = dynamic_cast<const MergeCommand *>(other);
        if (!command || m_annihilate || command->m_annihilate) {
            return false;
        }
        m_delta += command->m_delta;
        ++m_state->merges;
        return true;
    }

    bool canAnnihilateWith(const KUndo2Command *other) const override
    {
        const auto *command = dynamic_cast<const MergeCommand *>(other);
        return m_annihilate && command && command->m_annihilate && m_delta + command->m_delta == 0;
    }

private:
    bool m_annihilate;
};

class NotifyingStack final : public KUndo2QStack
{
public:
    using KUndo2QStack::KUndo2QStack;

    int notifications{0};

protected:
    void notifySetIndexChangedOneCommand() override
    {
        ++notifications;
    }
};

class StackDestructionProbe final : public KUndo2QStack
{
public:
    explicit StackDestructionProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~StackDestructionProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

class ExtraDataProbe final : public KUndo2CommandExtraData
{
public:
    ExtraDataProbe(int value, int *destructions)
        : value(value)
        , destructions(destructions)
    {
    }

    ~ExtraDataProbe() override
    {
        ++*destructions;
    }

    KUndo2CommandExtraData *clone() const override
    {
        return new ExtraDataProbe(value, destructions);
    }

    int value;
    int *destructions;
};

class CommandDestructionProbe final : public KUndo2Command
{
public:
    explicit CommandDestructionProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~CommandDestructionProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};
} // namespace

class KUndo2StackContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void commandDefaultsHierarchyAndLifetime();
    void commandTimingAndMergedExecution();
    void commandExtraDataOwnership();
    void stackStartsEmptyAndUsesQObjectLifetime();
    void stackPushUndoRedoTextSignalsAndActions();
    void setIndexClampsAndNotifiesPerCommand();
    void cleanStatePurgeAndClearRemainConsistent();
    void macrosGroupChildrenAndExecution();
    void undoLimitDiscardsOldestCommands();
    void commandCompressionAndAnnihilation();
    void activeStateFollowsTheOwningGroup();
    void cumulativeUndoConfigurationRoundTrips();
    void specializedActionsRegisterNamesIconsAndShortcuts();
};

void KUndo2StackContractTest::commandDefaultsHierarchyAndLifetime()
{
    CommandState state;
    KUndo2Command parent(kundo2_noi18n(QStringLiteral("parent")));
    auto *first = new RecordingCommand(&state, QStringLiteral("first"), 2, &parent);
    auto *second = new RecordingCommand(&state, QStringLiteral("second"), 3, &parent);

    QCOMPARE(parent.text().toString(), QStringLiteral("parent"));
    QCOMPARE(parent.actionText(), QStringLiteral("parent"));
    QCOMPARE(parent.childCount(), 2);
    QCOMPARE(parent.child(0), first);
    QCOMPARE(parent.child(1), second);
    QCOMPARE(parent.child(-1), nullptr);
    QCOMPARE(parent.child(2), nullptr);
    QVERIFY(!parent.hasParent());
    QVERIFY(first->hasParent());

    parent.redo();
    QCOMPARE(state.value, 5);
    QCOMPARE(state.calls, QStringList({QStringLiteral("redo:first"), QStringLiteral("redo:second")}));
    state.calls.clear();
    parent.undo();
    QCOMPARE(state.value, 0);
    QCOMPARE(state.calls, QStringList({QStringLiteral("undo:second"), QStringLiteral("undo:first")}));

    parent.setText(kundo2_noi18n(QStringLiteral("renamed")));
    QCOMPARE(parent.text().toString(), QStringLiteral("renamed"));
    QCOMPARE(parent.actionText(), QStringLiteral("renamed"));
    QCOMPARE(parent.id(), -1);
    QVERIFY(!parent.mergeWith(first));
    QVERIFY(!parent.canAnnihilateWith(first));

    KUndo2Command empty;
    QVERIFY(empty.text().isEmpty());
    QVERIFY(empty.actionText().isEmpty());

    bool destroyed = false;
    KUndo2Command *base = new CommandDestructionProbe(&destroyed);
    delete base;
    QVERIFY(destroyed);
}

void KUndo2StackContractTest::commandTimingAndMergedExecution()
{
    CommandState state;
    RecordingCommand destination(&state, QStringLiteral("destination"), 1);
    RecordingCommand older(&state, QStringLiteral("older"), 2);
    RecordingCommand newer(&state, QStringLiteral("newer"), 4);
    RecordingCommand mismatch(&state, QStringLiteral("mismatch"), 8);

    destination.setTimedID(7);
    older.setTimedID(7);
    newer.setTimedID(7);
    mismatch.setTimedID(9);
    QCOMPARE(destination.timedId(), 7);

    destination.setTime(QTime(1, 2, 3, 4));
    destination.setEndTime(QTime(1, 2, 5, 6));
    QCOMPARE(destination.time(), QTime(1, 2, 3, 4));
    QCOMPARE(destination.endTime(), QTime(1, 2, 5, 6));

    older.setTime(QTime(2, 0, 0));
    newer.setTime(QTime(3, 0, 0));
    QVERIFY(destination.timedMergeWith(&older));
    QVERIFY(destination.timedMergeWith(&newer));
    QVERIFY(!destination.timedMergeWith(&mismatch));
    QCOMPARE(destination.time(), newer.time());
    QVERIFY(destination.isMerged());
    QCOMPARE(destination.mergeCommandsVector(), QVector<KUndo2Command *>({&older, &newer}));

    destination.undoMergedCommands();
    QCOMPARE(state.value, -7);
    QCOMPARE(
        state.calls,
        QStringList({QStringLiteral("undo:destination"), QStringLiteral("undo:older"), QStringLiteral("undo:newer")}));
    state.calls.clear();
    destination.redoMergedCommands();
    QCOMPARE(state.value, 0);
    QCOMPARE(
        state.calls,
        QStringList({QStringLiteral("redo:newer"), QStringLiteral("redo:older"), QStringLiteral("redo:destination")}));

    KUndo2Command currentTimes;
    currentTimes.setTime(QTime());
    currentTimes.setEndTime(QTime());
    QVERIFY(currentTimes.time().isNull());
    QVERIFY(currentTimes.endTime().isNull());
    currentTimes.setTime();
    currentTimes.setEndTime();
    QVERIFY(currentTimes.time().isValid());
    QVERIFY(currentTimes.endTime().isValid());
}

void KUndo2StackContractTest::commandExtraDataOwnership()
{
    int destructions = 0;
    {
        KUndo2Command command;
        QCOMPARE(command.extraData(), nullptr);
        command.setExtraData(new ExtraDataProbe(3, &destructions));
        QCOMPARE(static_cast<ExtraDataProbe *>(command.extraData())->value, 3);
        command.setExtraData(new ExtraDataProbe(7, &destructions));
        QCOMPARE(destructions, 1);
        QCOMPARE(static_cast<ExtraDataProbe *>(command.extraData())->value, 7);
    }
    QCOMPARE(destructions, 2);
}

void KUndo2StackContractTest::stackStartsEmptyAndUsesQObjectLifetime()
{
    QObject owner;
    auto *stack = new KUndo2QStack(&owner);
    QPointer<KUndo2QStack> guard(stack);

    QCOMPARE(stack->parent(), &owner);
    QCOMPARE(stack->count(), 0);
    QCOMPARE(stack->index(), 0);
    QCOMPARE(stack->cleanIndex(), 0);
    QVERIFY(stack->isClean());
    QVERIFY(stack->isActive());
    QVERIFY(!stack->canUndo());
    QVERIFY(!stack->canRedo());
    QVERIFY(stack->undoText().isEmpty());
    QVERIFY(stack->redoText().isEmpty());
    QVERIFY(stack->text(-1).isEmpty());
    QVERIFY(stack->actionText(0).isEmpty());
    QCOMPARE(stack->command(0), nullptr);
    QCOMPARE(stack->undoLimit(), 0);
    QVERIFY(!stack->useCumulativeUndoRedo());

    stack->undo();
    stack->redo();
    stack->setIndex(9);
    stack->purgeRedoState();
    stack->clear();
    stack->setActive(false);
    QVERIFY(stack->isActive());

    delete stack;
    QVERIFY(guard.isNull());

    bool destroyed = false;
    KUndo2QStack *base = new StackDestructionProbe(&destroyed);
    delete base;
    QVERIFY(destroyed);
}

void KUndo2StackContractTest::stackPushUndoRedoTextSignalsAndActions()
{
    CommandState state;
    KUndo2QStack stack;
    QObject actionOwner;
    std::unique_ptr<QAction> undoAction(stack.createUndoAction(&actionOwner));
    std::unique_ptr<QAction> redoAction(stack.createRedoAction(&actionOwner));
    QSignalSpy indexSpy(&stack, &KUndo2QStack::indexChanged);
    QSignalSpy cleanSpy(&stack, &KUndo2QStack::cleanChanged);
    QSignalSpy canUndoSpy(&stack, &KUndo2QStack::canUndoChanged);
    QSignalSpy canRedoSpy(&stack, &KUndo2QStack::canRedoChanged);
    QSignalSpy undoTextSpy(&stack, &KUndo2QStack::undoTextChanged);
    QSignalSpy redoTextSpy(&stack, &KUndo2QStack::redoTextChanged);

    QVERIFY(!undoAction->isEnabled());
    QVERIFY(!redoAction->isEnabled());
    auto *command = new RecordingCommand(&state, QStringLiteral("increment"), 5);
    stack.push(command);

    QCOMPARE(state.value, 5);
    QCOMPARE(stack.count(), 1);
    QCOMPARE(stack.index(), 1);
    QCOMPARE(stack.command(0), command);
    QCOMPARE(stack.text(0), QStringLiteral("increment"));
    QCOMPARE(stack.actionText(0), QStringLiteral("increment"));
    QCOMPARE(stack.undoText(), QStringLiteral("increment"));
    QVERIFY(stack.redoText().isEmpty());
    QVERIFY(stack.canUndo());
    QVERIFY(!stack.canRedo());
    QVERIFY(!stack.isClean());
    QVERIFY(undoAction->isEnabled());
    QVERIFY(!redoAction->isEnabled());
    QVERIFY(undoAction->text().contains(QStringLiteral("increment")));
    QCOMPARE(indexSpy.count(), 1);
    QCOMPARE(cleanSpy.count(), 1);
    QCOMPARE(canUndoSpy.count(), 1);
    QCOMPARE(canRedoSpy.count(), 1);
    QCOMPARE(undoTextSpy.count(), 1);
    QCOMPARE(redoTextSpy.count(), 1);

    undoAction->trigger();
    QCOMPARE(state.value, 0);
    QCOMPARE(stack.index(), 0);
    QVERIFY(!undoAction->isEnabled());
    QVERIFY(redoAction->isEnabled());
    QVERIFY(redoAction->text().contains(QStringLiteral("increment")));

    redoAction->trigger();
    QCOMPARE(state.value, 5);
    QCOMPARE(stack.index(), 1);

    undoAction.release();
    redoAction.release();
}

void KUndo2StackContractTest::setIndexClampsAndNotifiesPerCommand()
{
    CommandState state;
    NotifyingStack stack;
    stack.push(new RecordingCommand(&state, QStringLiteral("one"), 1));
    stack.push(new RecordingCommand(&state, QStringLiteral("two"), 2));
    stack.push(new RecordingCommand(&state, QStringLiteral("three"), 3));
    QCOMPARE(state.value, 6);

    stack.setIndex(1);
    QCOMPARE(stack.index(), 1);
    QCOMPARE(stack.notifications, 2);
    QCOMPARE(state.value, 1);

    stack.setIndex(99);
    QCOMPARE(stack.index(), 3);
    QCOMPARE(stack.notifications, 4);
    QCOMPARE(state.value, 6);

    stack.setIndex(-5);
    QCOMPARE(stack.index(), 0);
    QCOMPARE(stack.notifications, 7);
    QCOMPARE(state.value, 0);
}

void KUndo2StackContractTest::cleanStatePurgeAndClearRemainConsistent()
{
    CommandState state;
    KUndo2QStack stack;
    QSignalSpy cleanSpy(&stack, &KUndo2QStack::cleanChanged);
    stack.push(new RecordingCommand(&state, QStringLiteral("one"), 1));
    stack.push(new RecordingCommand(&state, QStringLiteral("two"), 2));
    stack.setClean();
    QCOMPARE(stack.cleanIndex(), 2);
    QVERIFY(stack.isClean());

    stack.undo();
    QVERIFY(!stack.isClean());
    QVERIFY(stack.canRedo());
    stack.purgeRedoState();
    QCOMPARE(stack.count(), 1);
    QCOMPARE(stack.index(), 1);
    QCOMPARE(stack.cleanIndex(), -1);
    QVERIFY(!stack.canRedo());

    stack.setClean();
    QVERIFY(stack.isClean());
    stack.clear();
    QCOMPARE(stack.count(), 0);
    QCOMPARE(stack.index(), 0);
    QCOMPARE(stack.cleanIndex(), 0);
    QVERIFY(stack.isClean());
    QVERIFY(cleanSpy.count() >= 4);
}

void KUndo2StackContractTest::macrosGroupChildrenAndExecution()
{
    CommandState state;
    KUndo2QStack stack;
    QSignalSpy indexSpy(&stack, &KUndo2QStack::indexChanged);

    stack.beginMacro(kundo2_noi18n(QStringLiteral("outer")));
    QVERIFY(!stack.canUndo());
    QVERIFY(!stack.canRedo());
    stack.push(new RecordingCommand(&state, QStringLiteral("one"), 1));
    stack.beginMacro(kundo2_noi18n(QStringLiteral("inner")));
    stack.push(new RecordingCommand(&state, QStringLiteral("two"), 2));
    stack.endMacro();
    QCOMPARE(indexSpy.count(), 0);
    stack.endMacro();

    QCOMPARE(state.value, 3);
    QCOMPARE(stack.count(), 1);
    QCOMPARE(stack.index(), 1);
    QCOMPARE(stack.text(0), QStringLiteral("outer"));
    QCOMPARE(stack.command(0)->childCount(), 2);
    QCOMPARE(stack.command(0)->child(1)->childCount(), 1);
    QCOMPARE(indexSpy.count(), 1);

    stack.undo();
    QCOMPARE(state.value, 0);
    stack.redo();
    QCOMPARE(state.value, 3);
}

void KUndo2StackContractTest::undoLimitDiscardsOldestCommands()
{
    CommandState state;
    KUndo2QStack stack;
    stack.setUndoLimit(2);
    QCOMPARE(stack.undoLimit(), 2);

    stack.push(new RecordingCommand(&state, QStringLiteral("one"), 1));
    stack.push(new RecordingCommand(&state, QStringLiteral("two"), 2));
    stack.push(new RecordingCommand(&state, QStringLiteral("three"), 3));
    QCOMPARE(stack.count(), 2);
    QCOMPARE(stack.index(), 2);
    QCOMPARE(stack.text(0), QStringLiteral("two"));
    QCOMPARE(state.destructions, 1);

    stack.setUndoLimit(1);
    QCOMPARE(stack.undoLimit(), 2);
}

void KUndo2StackContractTest::commandCompressionAndAnnihilation()
{
    CommandState mergedState;
    KUndo2QStack mergedStack;
    mergedStack.push(new MergeCommand(&mergedState, QStringLiteral("one"), 1));
    mergedStack.push(new MergeCommand(&mergedState, QStringLiteral("two"), 2));
    QCOMPARE(mergedState.value, 3);
    QCOMPARE(mergedState.merges, 1);
    QCOMPARE(mergedStack.count(), 1);
    QCOMPARE(mergedState.destructions, 1);
    mergedStack.undo();
    QCOMPARE(mergedState.value, 0);

    CommandState annihilatedState;
    KUndo2QStack annihilatedStack;
    annihilatedStack.push(new MergeCommand(&annihilatedState, QStringLiteral("plus"), 4, true));
    annihilatedStack.push(new MergeCommand(&annihilatedState, QStringLiteral("minus"), -4, true));
    QCOMPARE(annihilatedState.value, 0);
    QCOMPARE(annihilatedStack.count(), 0);
    QCOMPARE(annihilatedStack.index(), 0);
    QCOMPARE(annihilatedState.destructions, 2);
}

void KUndo2StackContractTest::activeStateFollowsTheOwningGroup()
{
    KUndo2Group group;
    KUndo2QStack first(&group);
    KUndo2QStack second(&group);
    first.setActive();
    QVERIFY(first.isActive());
    QVERIFY(!second.isActive());
    second.setActive(true);
    QVERIFY(!first.isActive());
    QVERIFY(second.isActive());
    second.setActive(false);
    QCOMPARE(group.activeStack(), nullptr);
    QVERIFY(!first.isActive());
    QVERIFY(!second.isActive());
}

void KUndo2StackContractTest::cumulativeUndoConfigurationRoundTrips()
{
    KUndo2QStack stack;
    KisCumulativeUndoData data;
    data.excludeFromMerge = 2;
    data.mergeTimeout = 3000;
    data.maxGroupSeparation = 400;
    data.maxGroupDuration = 6000;

    stack.setUseCumulativeUndoRedo(true);
    stack.setCumulativeUndoData(data);
    QVERIFY(stack.useCumulativeUndoRedo());
    const KisCumulativeUndoData actual = stack.cumulativeUndoData();
    QCOMPARE(actual.excludeFromMerge, 2);
    QCOMPARE(actual.mergeTimeout, 3000);
    QCOMPARE(actual.maxGroupSeparation, 400);
    QCOMPARE(actual.maxGroupDuration, 6000);
}

void KUndo2StackContractTest::specializedActionsRegisterNamesIconsAndShortcuts()
{
    CommandState state;
    QObject owner;
    KUndo2Stack stack(&owner);
    KisKActionCollection collection;
    KisIconUtils::requestedIcons.clear();

    QAction *undoAction = stack.createUndoAction(&collection);
    QAction *redoAction = stack.createRedoAction(&collection, QStringLiteral("custom_redo"));
    QCOMPARE(stack.parent(), &owner);
    QCOMPARE(collection.names, QStringList({QStringLiteral("edit_undo"), QStringLiteral("custom_redo")}));
    QCOMPARE(collection.actions, QList<QAction *>({undoAction, redoAction}));
    QCOMPARE(undoAction->objectName(), QStringLiteral("edit_undo"));
    QCOMPARE(redoAction->objectName(), QStringLiteral("custom_redo"));
    QCOMPARE(KisIconUtils::requestedIcons, QStringList({QStringLiteral("edit-undo"), QStringLiteral("edit-redo")}));
    QVERIFY(!undoAction->icon().isNull());
    QVERIFY(!redoAction->icon().isNull());
    QVERIFY(!undoAction->iconText().isEmpty());
    QVERIFY(!redoAction->iconText().isEmpty());
    QCOMPARE(undoAction->shortcuts(), KStandardShortcut::undo());
    QCOMPARE(redoAction->shortcuts(), KStandardShortcut::redo());

    stack.push(new RecordingCommand(&state, QStringLiteral("change"), 9));
    undoAction->trigger();
    QCOMPARE(state.value, 0);
    redoAction->trigger();
    QCOMPARE(state.value, 9);
}

QTEST_MAIN(KUndo2StackContractTest)

#include "KUndo2StackContractTest.moc"
