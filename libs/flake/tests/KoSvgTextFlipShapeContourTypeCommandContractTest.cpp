/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeBulkActionLock.h>
#include <KoSvgTextFlipShapeContourTypeCommand.h>
#include <KoSvgTextShape.h>

#include <QHash>
#include <QList>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace KoSvgTextFlipShapeContourTypeCommandTesting
{
using ContourMembershipReader = bool (*)(const KoSvgTextShape *textShape, KoShape *shape);
using ContourMembershipBatchApplier = void (*)(KoSvgTextShape *textShape, KoShape *shape, bool inside);

void setContourAccessForTesting(ContourMembershipReader reader, ContourMembershipBatchApplier applier);
void resetContourAccessForTesting();
} // namespace KoSvgTextFlipShapeContourTypeCommandTesting

namespace
{
QHash<const KUndo2Command *, KUndo2Command *> commandParents;
int baseDestructionCount = 0;

bool currentMembership = false;
int membershipReadCount = 0;
const KoSvgTextShape *lastReadTextShape = nullptr;
KoShape *lastReadShape = nullptr;

struct AppliedMembership {
    KoSvgTextShape *textShape;
    KoShape *shape;
    bool inside;
};

QList<AppliedMembership> appliedMemberships;

[[noreturn]] void unexpectedCall(const char *name)
{
    qFatal("Unexpected production path reached: %s", name);
}

bool readMembership(const KoSvgTextShape *textShape, KoShape *shape)
{
    ++membershipReadCount;
    lastReadTextShape = textShape;
    lastReadShape = shape;
    return currentMembership;
}

void applyMembership(KoSvgTextShape *textShape, KoShape *shape, bool inside)
{
    appliedMemberships.append({textShape, shape, inside});
}

struct alignas(std::max_align_t) TextShapeToken {
    unsigned char value{0};
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

KoSvgTextShape *textShapePointer(TextShapeToken &token)
{
    return reinterpret_cast<KoSvgTextShape *>(&token);
}

KoShape *shapePointer(ShapeToken &token)
{
    return reinterpret_cast<KoShape *>(&token);
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

QList<KoShape *> KoSvgTextShape::shapesInside() const
{
    unexpectedCall("KoSvgTextShape::shapesInside");
}

void KoSvgTextShape::addShapeContours(QList<KoShape *>, const bool)
{
    unexpectedCall("KoSvgTextShape::addShapeContours");
}

KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter(const QList<KoShape *> &)
{
    unexpectedCall("KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter");
}

void KoShapeBulkActionLockAdapter::lock()
{
    unexpectedCall("KoShapeBulkActionLockAdapter::lock");
}

void KoShapeBulkActionLockAdapter::unlock()
{
    unexpectedCall("KoShapeBulkActionLockAdapter::unlock");
}

KoShapeBulkActionLockAdapter::UpdatesList KoShapeBulkActionLockAdapter::takeFinalUpdatesList()
{
    unexpectedCall("KoShapeBulkActionLockAdapter::takeFinalUpdatesList");
}

KoShapeBulkActionLock::~KoShapeBulkActionLock()
{
    unexpectedCall("KoShapeBulkActionLock::~KoShapeBulkActionLock");
}

void KoShapeBulkActionLock::bulkShapesUpdate(const UpdatesList &)
{
    unexpectedCall("KoShapeBulkActionLock::bulkShapesUpdate");
}

class KoSvgTextFlipShapeContourTypeCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void capturesInitialMembershipAndKeepsInputsBorrowed();
    void redoFlipsAndUndoRestoresCapturedContourMembership_data();
    void redoFlipsAndUndoRestoresCapturedContourMembership();
};

void KoSvgTextFlipShapeContourTypeCommandContractTest::init()
{
    commandParents.clear();
    baseDestructionCount = 0;
    currentMembership = false;
    membershipReadCount = 0;
    lastReadTextShape = nullptr;
    lastReadShape = nullptr;
    appliedMemberships.clear();
    KoSvgTextFlipShapeContourTypeCommandTesting::setContourAccessForTesting(readMembership, applyMembership);
}

void KoSvgTextFlipShapeContourTypeCommandContractTest::cleanup()
{
    KoSvgTextFlipShapeContourTypeCommandTesting::resetContourAccessForTesting();
}

void KoSvgTextFlipShapeContourTypeCommandContractTest::capturesInitialMembershipAndKeepsInputsBorrowed()
{
    TextShapeToken textShapeStorage;
    textShapeStorage.value = 19;
    ShapeToken shapeStorage;
    shapeStorage.value = 23;
    KoSvgTextShape *const textShape = textShapePointer(textShapeStorage);
    KoShape *const shape = shapePointer(shapeStorage);
    currentMembership = true;
    KUndo2Command parent;
    const int destructionCountBeforeCommand = baseDestructionCount;

    {
        KoSvgTextFlipShapeContourTypeCommand command(textShape, shape, &parent);

        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(membershipReadCount, 1);
        QCOMPARE(lastReadTextShape, textShape);
        QCOMPARE(lastReadShape, shape);
        QVERIFY(appliedMemberships.isEmpty());
    }

    QCOMPARE(baseDestructionCount, destructionCountBeforeCommand + 1);
    QCOMPARE(textShapeStorage.value, 19);
    QCOMPARE(shapeStorage.value, 23);
}

void KoSvgTextFlipShapeContourTypeCommandContractTest::redoFlipsAndUndoRestoresCapturedContourMembership_data()
{
    QTest::addColumn<bool>("initialInside");

    QTest::newRow("initially-inside") << true;
    QTest::newRow("initially-outside") << false;
}

void KoSvgTextFlipShapeContourTypeCommandContractTest::redoFlipsAndUndoRestoresCapturedContourMembership()
{
    QFETCH(bool, initialInside);

    TextShapeToken textShapeStorage;
    ShapeToken shapeStorage;
    KoSvgTextShape *const textShape = textShapePointer(textShapeStorage);
    KoShape *const shape = shapePointer(shapeStorage);
    currentMembership = initialInside;

    KoSvgTextFlipShapeContourTypeCommand command(textShape, shape);
    QCOMPARE(membershipReadCount, 1);
    currentMembership = !initialInside;

    command.redo();
    command.undo();

    QCOMPARE(membershipReadCount, 1);
    QCOMPARE(appliedMemberships.size(), 2);
    QCOMPARE(appliedMemberships.at(0).textShape, textShape);
    QCOMPARE(appliedMemberships.at(0).shape, shape);
    QCOMPARE(appliedMemberships.at(0).inside, !initialInside);
    QCOMPARE(appliedMemberships.at(1).textShape, textShape);
    QCOMPARE(appliedMemberships.at(1).shape, shape);
    QCOMPARE(appliedMemberships.at(1).inside, initialInside);
}

QTEST_GUILESS_MAIN(KoSvgTextFlipShapeContourTypeCommandContractTest)

#include "KoSvgTextFlipShapeContourTypeCommandContractTest.moc"
