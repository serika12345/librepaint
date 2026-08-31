/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoAddRemoveShapeCommands.h>
#include <KoShapeContainer.h>

#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace
{
QHash<const KUndo2Command *, KUndo2Command *> commandParents;
QHash<const KisCommandUtils::FlipFlopCommand *, KisCommandUtils::FlipFlopCommand::State> commandStates;
QHash<const KoShape *, QString> shapeLabels;
QHash<const KoShapeContainer *, QString> containerLabels;
QStringList operationTrace;
int baseDestructionCount = 0;

[[noreturn]] void unexpectedCall(const char *name)
{
    qFatal("Unexpected production path reached: %s", name);
}
} // namespace

KUndo2Command::KUndo2Command(KUndo2Command *parent)
{
    commandParents.insert(this, parent);
}

KUndo2Command::~KUndo2Command()
{
    ++baseDestructionCount;
    commandParents.remove(this);
}

void KUndo2Command::undo()
{
    unexpectedCall("KUndo2Command::undo");
}

void KUndo2Command::redo()
{
    unexpectedCall("KUndo2Command::redo");
}

int KUndo2Command::id() const
{
    unexpectedCall("KUndo2Command::id");
}

int KUndo2Command::timedId() const
{
    unexpectedCall("KUndo2Command::timedId");
}

void KUndo2Command::setTimedID(int)
{
    unexpectedCall("KUndo2Command::setTimedID");
}

bool KUndo2Command::mergeWith(const KUndo2Command *)
{
    unexpectedCall("KUndo2Command::mergeWith");
}

bool KUndo2Command::timedMergeWith(KUndo2Command *)
{
    unexpectedCall("KUndo2Command::timedMergeWith");
}

bool KUndo2Command::canAnnihilateWith(const KUndo2Command *) const
{
    unexpectedCall("KUndo2Command::canAnnihilateWith");
}

void KUndo2Command::setTime(const QTime &)
{
    unexpectedCall("KUndo2Command::setTime");
}

QTime KUndo2Command::time() const
{
    unexpectedCall("KUndo2Command::time");
}

void KUndo2Command::setEndTime(const QTime &)
{
    unexpectedCall("KUndo2Command::setEndTime");
}

QTime KUndo2Command::endTime() const
{
    unexpectedCall("KUndo2Command::endTime");
}

QVector<KUndo2Command *> KUndo2Command::mergeCommandsVector() const
{
    unexpectedCall("KUndo2Command::mergeCommandsVector");
}

bool KUndo2Command::isMerged() const
{
    unexpectedCall("KUndo2Command::isMerged");
}

void KUndo2Command::undoMergedCommands()
{
    unexpectedCall("KUndo2Command::undoMergedCommands");
}

void KUndo2Command::redoMergedCommands()
{
    unexpectedCall("KUndo2Command::redoMergedCommands");
}

namespace KisCommandUtils
{
FlipFlopCommand::FlipFlopCommand(bool finalizing, KUndo2Command *parent)
    : FlipFlopCommand(finalizing ? State::FINALIZING : State::INITIALIZING, parent)
{
}

FlipFlopCommand::FlipFlopCommand(State initialState, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_currentState(initialState)
{
    commandStates.insert(this, initialState);
}

void FlipFlopCommand::redo()
{
    unexpectedCall("KisCommandUtils::FlipFlopCommand::redo");
}

void FlipFlopCommand::undo()
{
    unexpectedCall("KisCommandUtils::FlipFlopCommand::undo");
}

void FlipFlopCommand::partA()
{
    unexpectedCall("KisCommandUtils::FlipFlopCommand::partA");
}

void FlipFlopCommand::partB()
{
    unexpectedCall("KisCommandUtils::FlipFlopCommand::partB");
}
} // namespace KisCommandUtils

void KoShapeContainer::addShape(KoShape *shape)
{
    operationTrace.append(QStringLiteral("%1:add:%2").arg(containerLabels.value(this), shapeLabels.value(shape)));
}

void KoShapeContainer::removeShape(KoShape *shape)
{
    operationTrace.append(QStringLiteral("%1:remove:%2").arg(containerLabels.value(this), shapeLabels.value(shape)));
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    unexpectedCall("kis_safe_assert_recoverable");
}

namespace
{
struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

struct alignas(std::max_align_t) ContainerToken {
    unsigned char value{0};
};

KoShape *shapePointer(ShapeToken &token)
{
    return reinterpret_cast<KoShape *>(&token);
}

KoShapeContainer *containerPointer(ContainerToken &token)
{
    return reinterpret_cast<KoShapeContainer *>(&token);
}
} // namespace

class KoAddRemoveShapeCommandsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void basePhasesAddRemoveAndReturnToBorrowedState();
    void addCommandSelectsInitializingState();
    void removeCommandSelectsFinalizingState();
};

void KoAddRemoveShapeCommandsContractTest::init()
{
    commandParents.clear();
    commandStates.clear();
    shapeLabels.clear();
    containerLabels.clear();
    operationTrace.clear();
    baseDestructionCount = 0;
}

void KoAddRemoveShapeCommandsContractTest::basePhasesAddRemoveAndReturnToBorrowedState()
{
    ShapeToken shapeStorage;
    shapeStorage.value = 17;
    ContainerToken containerStorage;
    containerStorage.value = 23;
    KoShape *const shape = shapePointer(shapeStorage);
    KoShapeContainer *const container = containerPointer(containerStorage);
    shapeLabels.insert(shape, QStringLiteral("shape"));
    containerLabels.insert(container, QStringLiteral("container"));
    KUndo2Command parent;
    const int destructionCountBeforeCommand = baseDestructionCount;

    {
        KoAddRemoveShapeCommandImpl command(shape, container, KisCommandUtils::FlipFlopCommand::INITIALIZING, &parent);
        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(commandStates.value(&command), KisCommandUtils::FlipFlopCommand::INITIALIZING);

        command.partA();
        command.partB();
        command.partA();

        QCOMPARE(operationTrace,
                 QStringList({QStringLiteral("container:add:shape"),
                              QStringLiteral("container:remove:shape"),
                              QStringLiteral("container:add:shape")}));
    }

    QCOMPARE(baseDestructionCount, destructionCountBeforeCommand + 1);
    QCOMPARE(shapeStorage.value, 17);
    QCOMPARE(containerStorage.value, 23);
}

void KoAddRemoveShapeCommandsContractTest::addCommandSelectsInitializingState()
{
    ShapeToken shapeStorage;
    ContainerToken containerStorage;
    KoShape *const shape = shapePointer(shapeStorage);
    KoShapeContainer *const container = containerPointer(containerStorage);
    shapeLabels.insert(shape, QStringLiteral("added"));
    containerLabels.insert(container, QStringLiteral("parent"));
    KUndo2Command parent;

    {
        KoAddShapeCommand command(shape, container, &parent);
        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(commandStates.value(&command), KisCommandUtils::FlipFlopCommand::INITIALIZING);

        command.partA();
        command.partB();
        command.partA();
    }

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("parent:add:added"),
                          QStringLiteral("parent:remove:added"),
                          QStringLiteral("parent:add:added")}));
}

void KoAddRemoveShapeCommandsContractTest::removeCommandSelectsFinalizingState()
{
    ShapeToken shapeStorage;
    ContainerToken containerStorage;
    KoShape *const shape = shapePointer(shapeStorage);
    KoShapeContainer *const container = containerPointer(containerStorage);
    shapeLabels.insert(shape, QStringLiteral("removed"));
    containerLabels.insert(container, QStringLiteral("parent"));
    KUndo2Command parent;

    {
        KoRemoveShapeCommand command(shape, container, &parent);
        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(commandStates.value(&command), KisCommandUtils::FlipFlopCommand::FINALIZING);

        command.partB();
        command.partA();
    }

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("parent:remove:removed"), QStringLiteral("parent:add:removed")}));
}

QTEST_GUILESS_MAIN(KoAddRemoveShapeCommandsContractTest)

#include "KoAddRemoveShapeCommandsContractTest.moc"
