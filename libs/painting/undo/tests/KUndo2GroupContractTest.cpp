/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <memory>
#include <utility>

#include <QAction>
#include <QPointer>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>

#include <kundo2group.h>
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

class RecordingCommand final : public KUndo2Command
{
public:
    RecordingCommand(CommandState *state, QString name, int delta)
        : KUndo2Command(kundo2_noi18n(name))
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

private:
    CommandState *m_state;
    QString m_name;
    int m_delta;
};

class DestructionProbe final : public KUndo2Group
{
public:
    explicit DestructionProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~DestructionProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};
} // namespace

class KUndo2GroupContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsEmptyAndUsesQObjectLifetime();
    void tracksMembershipTransfersAndDestruction();
    void selectsActiveStackAndForwardsSignals();
    void delegatesQueriesUndoAndRedo();
    void keepsActionsSynchronizedWithActiveStack();
    void acceptsUnregisteredActiveStackAsKnownDefect();
};

void KUndo2GroupContractTest::startsEmptyAndUsesQObjectLifetime()
{
    QObject owner;
    auto *group = new KUndo2Group(&owner);
    QPointer<KUndo2Group> guard(group);

    QCOMPARE(group->parent(), &owner);
    QVERIFY(group->stacks().isEmpty());
    QCOMPARE(group->activeStack(), nullptr);
    QVERIFY(!group->canUndo());
    QVERIFY(!group->canRedo());
    QVERIFY(group->undoText().isEmpty());
    QVERIFY(group->redoText().isEmpty());
    QVERIFY(group->isClean());

    group->undo();
    group->redo();
    delete group;
    QVERIFY(guard.isNull());

    bool destroyed = false;
    KUndo2Group *base = new DestructionProbe(&destroyed);
    delete base;
    QVERIFY(destroyed);
}

void KUndo2GroupContractTest::tracksMembershipTransfersAndDestruction()
{
    KUndo2Group firstGroup;
    KUndo2Group secondGroup;

    auto *parentOwned = new KUndo2QStack(&firstGroup);
    QCOMPARE(firstGroup.stacks(), QList<KUndo2QStack *>({parentOwned}));

    firstGroup.addStack(parentOwned);
    QCOMPARE(firstGroup.stacks(), QList<KUndo2QStack *>({parentOwned}));

    secondGroup.addStack(parentOwned);
    QVERIFY(firstGroup.stacks().isEmpty());
    QCOMPARE(secondGroup.stacks(), QList<KUndo2QStack *>({parentOwned}));

    secondGroup.removeStack(parentOwned);
    QVERIFY(secondGroup.stacks().isEmpty());
    secondGroup.removeStack(parentOwned);

    firstGroup.addStack(parentOwned);
    delete parentOwned;
    QVERIFY(firstGroup.stacks().isEmpty());

    KUndo2QStack externallyOwned;
    {
        auto temporaryGroup = std::make_unique<KUndo2Group>();
        temporaryGroup->addStack(&externallyOwned);
        temporaryGroup->setActiveStack(&externallyOwned);
        QCOMPARE(temporaryGroup->activeStack(), &externallyOwned);
    }

    secondGroup.addStack(&externallyOwned);
    QCOMPARE(secondGroup.stacks(), QList<KUndo2QStack *>({&externallyOwned}));
    secondGroup.removeStack(&externallyOwned);
}

void KUndo2GroupContractTest::selectsActiveStackAndForwardsSignals()
{
    CommandState firstState;
    KUndo2Group group;
    KUndo2QStack firstStack;
    KUndo2QStack secondStack;
    group.addStack(&firstStack);
    group.addStack(&secondStack);

    KUndo2QStack *lastActive = nullptr;
    int activeChanges = 0;
    QObject::connect(
        &group,
        &KUndo2Group::activeStackChanged,
        &group,
        [&lastActive, &activeChanges](KUndo2QStack *stack) {
            lastActive = stack;
            ++activeChanges;
        },
        Qt::DirectConnection);
    QSignalSpy indexSpy(&group, &KUndo2Group::indexChanged);
    QSignalSpy cleanSpy(&group, &KUndo2Group::cleanChanged);
    QSignalSpy canUndoSpy(&group, &KUndo2Group::canUndoChanged);
    QSignalSpy canRedoSpy(&group, &KUndo2Group::canRedoChanged);
    QSignalSpy undoTextSpy(&group, &KUndo2Group::undoTextChanged);
    QSignalSpy redoTextSpy(&group, &KUndo2Group::redoTextChanged);

    group.setActiveStack(&firstStack);
    QCOMPARE(group.activeStack(), &firstStack);
    QCOMPARE(lastActive, &firstStack);
    QCOMPARE(activeChanges, 1);
    QCOMPARE(indexSpy.last().at(0).toInt(), 0);
    QVERIFY(cleanSpy.last().at(0).toBool());
    QVERIFY(!canUndoSpy.last().at(0).toBool());
    QVERIFY(!canRedoSpy.last().at(0).toBool());
    QVERIFY(undoTextSpy.last().at(0).toString().isEmpty());
    QVERIFY(redoTextSpy.last().at(0).toString().isEmpty());

    firstStack.push(new RecordingCommand(&firstState, QStringLiteral("first"), 2));
    QCOMPARE(indexSpy.last().at(0).toInt(), 1);
    QVERIFY(!cleanSpy.last().at(0).toBool());
    QVERIFY(canUndoSpy.last().at(0).toBool());
    QVERIFY(!canRedoSpy.last().at(0).toBool());
    QCOMPARE(undoTextSpy.last().at(0).toString(), QStringLiteral("first"));
    QVERIFY(redoTextSpy.last().at(0).toString().isEmpty());

    group.setActiveStack(&secondStack);
    QCOMPARE(group.activeStack(), &secondStack);
    QCOMPARE(lastActive, &secondStack);
    QCOMPARE(activeChanges, 2);

    const int indexCount = indexSpy.count();
    firstStack.undo();
    QCOMPARE(indexSpy.count(), indexCount);

    secondStack.setActive();
    QCOMPARE(activeChanges, 2);

    group.setActiveStack(nullptr);
    QCOMPARE(group.activeStack(), nullptr);
    QCOMPARE(lastActive, nullptr);
    QCOMPARE(activeChanges, 3);
    QCOMPARE(indexSpy.last().at(0).toInt(), 0);
    QVERIFY(cleanSpy.last().at(0).toBool());
    QVERIFY(!canUndoSpy.last().at(0).toBool());
    QVERIFY(!canRedoSpy.last().at(0).toBool());
    QVERIFY(undoTextSpy.last().at(0).toString().isEmpty());
    QVERIFY(redoTextSpy.last().at(0).toString().isEmpty());

    group.setActiveStack(nullptr);
    QCOMPARE(activeChanges, 3);
}

void KUndo2GroupContractTest::delegatesQueriesUndoAndRedo()
{
    CommandState state;
    KUndo2Group group;
    KUndo2QStack stack;
    group.addStack(&stack);
    stack.setActive();

    QVERIFY(group.isClean());
    stack.push(new RecordingCommand(&state, QStringLiteral("raise"), 3));
    QCOMPARE(state.value, 3);
    QVERIFY(group.canUndo());
    QVERIFY(!group.canRedo());
    QCOMPARE(group.undoText(), QStringLiteral("raise"));
    QVERIFY(group.redoText().isEmpty());
    QVERIFY(!group.isClean());

    group.undo();
    QCOMPARE(state.value, 0);
    QVERIFY(!group.canUndo());
    QVERIFY(group.canRedo());
    QVERIFY(group.undoText().isEmpty());
    QCOMPARE(group.redoText(), QStringLiteral("raise"));
    QVERIFY(group.isClean());

    group.redo();
    QCOMPARE(state.value, 3);
    QVERIFY(group.canUndo());
    QVERIFY(!group.canRedo());
    QCOMPARE(state.calls,
             QStringList({QStringLiteral("redo:raise"), QStringLiteral("undo:raise"), QStringLiteral("redo:raise")}));

    group.removeStack(&stack);
    QCOMPARE(group.activeStack(), nullptr);
    QVERIFY(group.isClean());
}

void KUndo2GroupContractTest::keepsActionsSynchronizedWithActiveStack()
{
    CommandState state;
    QObject actionOwner;
    KUndo2Group group;
    KUndo2QStack stack;

    std::unique_ptr<QAction> undoAction(group.createUndoAction(&actionOwner));
    std::unique_ptr<QAction> redoAction(group.createRedoAction(&actionOwner));
    QCOMPARE(undoAction->parent(), &actionOwner);
    QCOMPARE(redoAction->parent(), &actionOwner);
    QVERIFY(!undoAction->isEnabled());
    QVERIFY(!redoAction->isEnabled());
    QVERIFY(!undoAction->text().isEmpty());
    QVERIFY(!redoAction->text().isEmpty());

    group.addStack(&stack);
    group.setActiveStack(&stack);
    stack.push(new RecordingCommand(&state, QStringLiteral("paint"), 5));
    QVERIFY(undoAction->isEnabled());
    QVERIFY(!redoAction->isEnabled());
    QVERIFY(undoAction->text().contains(QStringLiteral("paint")));

    undoAction->trigger();
    QCOMPARE(state.value, 0);
    QVERIFY(!undoAction->isEnabled());
    QVERIFY(redoAction->isEnabled());
    QVERIFY(redoAction->text().contains(QStringLiteral("paint")));

    redoAction->trigger();
    QCOMPARE(state.value, 5);
    QVERIFY(undoAction->isEnabled());
    QVERIFY(!redoAction->isEnabled());

    group.setActiveStack(nullptr);
    QVERIFY(!undoAction->isEnabled());
    QVERIFY(!redoAction->isEnabled());
}

void KUndo2GroupContractTest::acceptsUnregisteredActiveStackAsKnownDefect()
{
    CommandState state;
    KUndo2Group group;
    KUndo2QStack unregisteredStack;

    QVERIFY(group.stacks().isEmpty());
    group.setActiveStack(&unregisteredStack);
    QCOMPARE(group.activeStack(), &unregisteredStack);
    QVERIFY(group.stacks().isEmpty());

    unregisteredStack.push(new RecordingCommand(&state, QStringLiteral("outside"), 4));
    QVERIFY(group.canUndo());
    group.undo();
    QCOMPARE(state.value, 0);
}

QTEST_MAIN(KUndo2GroupContractTest)

#include "KUndo2GroupContractTest.moc"
