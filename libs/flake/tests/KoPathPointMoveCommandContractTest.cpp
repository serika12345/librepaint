/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoPathPoint.h>
#include <KoPathPointMoveCommand.h>
#include <KoShapeBulkActionLock.h>

#include <kis_command_ids.h>

#include <QHash>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace KoPathPointMoveCommandTesting
{
using OffsetMap = QMap<KoPathPointData, QPointF>;
using OffsetBatchApplier = void (*)(const OffsetMap &points, const QSet<KoPathShape *> &paths, qreal factor);

void setOffsetBatchApplierForTesting(OffsetBatchApplier applier);
void resetOffsetBatchApplierForTesting();
} // namespace KoPathPointMoveCommandTesting

namespace
{
using OffsetMap = QMap<KoPathPointData, QPointF>;

QHash<const KUndo2Command *, KUndo2MagicString> commandTexts;
QHash<const KUndo2Command *, KUndo2Command *> commandParents;
QStringList operationTrace;
int baseDestructionCount = 0;

struct OffsetBatchObservation {
    OffsetMap points;
    QSet<KoPathShape *> paths;
    qreal factor{0.0};
};

struct alignas(std::max_align_t) PathShapeToken {
    unsigned char value{0};
};

QList<OffsetBatchObservation> batchObservations;

[[noreturn]] void unexpectedCall(const char *name)
{
    qFatal("Unexpected production path reached: %s", name);
}

KoPathShape *pathShapePointer(PathShapeToken &token)
{
    return reinterpret_cast<KoPathShape *>(&token);
}

void applyOffsets(const OffsetMap &points, const QSet<KoPathShape *> &paths, qreal factor)
{
    batchObservations.append({points, paths, factor});
    operationTrace.append(QStringLiteral("offset-batch"));
}

void verifyOffset(const OffsetBatchObservation &observation, const KoPathPointData &pointData, const QPointF &expected)
{
    QVERIFY(observation.points.contains(pointData));
    QCOMPARE(observation.points.value(pointData), expected);
}
} // namespace

KUndo2MagicString::KUndo2MagicString() = default;

KUndo2MagicString::KUndo2MagicString(const QString &text)
    : m_text(text)
{
}

QString KUndo2MagicString::toString() const
{
    const int separator = m_text.indexOf(QLatin1Char('\n'));
    return separator > 0 ? m_text.left(separator) : m_text;
}

QString KUndo2MagicString::toSecondaryString() const
{
    const int separator = m_text.indexOf(QLatin1Char('\n'));
    return separator > 0 ? m_text.mid(separator + 1) : m_text;
}

bool KUndo2MagicString::isEmpty() const
{
    return m_text.isEmpty();
}

bool KUndo2MagicString::operator==(const KUndo2MagicString &rhs) const
{
    return m_text == rhs.m_text;
}

KUndo2Command::KUndo2Command(KUndo2Command *parent)
{
    commandParents.insert(this, parent);
}

KUndo2Command::KUndo2Command(const KUndo2MagicString &text, KUndo2Command *parent)
{
    commandParents.insert(this, parent);
    setText(text);
}

KUndo2Command::~KUndo2Command()
{
    ++baseDestructionCount;
    commandTexts.remove(this);
    commandParents.remove(this);
}

void KUndo2Command::undo()
{
    operationTrace.append(QStringLiteral("base-undo"));
}

void KUndo2Command::redo()
{
    operationTrace.append(QStringLiteral("base-redo"));
}

QString KUndo2Command::actionText() const
{
    return commandTexts.value(this).toSecondaryString();
}

void KUndo2Command::setText(const KUndo2MagicString &text)
{
    commandTexts.insert(this, text);
}

int KUndo2Command::id() const
{
    return -1;
}

int KUndo2Command::timedId() const
{
    return m_timedID;
}

void KUndo2Command::setTimedID(int timedID)
{
    m_timedID = timedID;
}

bool KUndo2Command::mergeWith(const KUndo2Command *other)
{
    Q_UNUSED(other);
    return false;
}

bool KUndo2Command::timedMergeWith(KUndo2Command *other)
{
    Q_UNUSED(other);
    return false;
}

bool KUndo2Command::canAnnihilateWith(const KUndo2Command *other) const
{
    Q_UNUSED(other);
    return false;
}

void KUndo2Command::setTime(const QTime &time)
{
    m_timeOfCreation = time;
}

QTime KUndo2Command::time() const
{
    return m_timeOfCreation;
}

void KUndo2Command::setEndTime(const QTime &time)
{
    m_endOfCommand = time;
}

QTime KUndo2Command::endTime() const
{
    return m_endOfCommand;
}

QVector<KUndo2Command *> KUndo2Command::mergeCommandsVector() const
{
    return m_mergeCommandsVector;
}

bool KUndo2Command::isMerged() const
{
    return !m_mergeCommandsVector.isEmpty();
}

void KUndo2Command::undoMergedCommands()
{
}

void KUndo2Command::redoMergedCommands()
{
}

QPointF KoShape::documentToShape(const QPointF &) const
{
    unexpectedCall("KoShape::documentToShape");
}

KoPathPoint *KoPathShape::pointByIndex(const KoPathPointIndex &) const
{
    unexpectedCall("KoPathShape::pointByIndex");
}

void KoPathPoint::map(const QTransform &)
{
    unexpectedCall("KoPathPoint::map");
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

class KoPathPointMoveCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void constructorsCopyOffsetsAndKeepFirstDuplicate();
    void redoAndUndoDispatchBaseBeforeSignedOffsetBatch();
    void idAndMergeRequireSamePointSetAndAccumulateOffsets();
    void keepsShapesBorrowedAndSupportsEmptyInput();
};

void KoPathPointMoveCommandContractTest::init()
{
    commandTexts.clear();
    commandParents.clear();
    operationTrace.clear();
    baseDestructionCount = 0;
    batchObservations.clear();
    KoPathPointMoveCommandTesting::setOffsetBatchApplierForTesting(applyOffsets);
}

void KoPathPointMoveCommandContractTest::cleanup()
{
    KoPathPointMoveCommandTesting::resetOffsetBatchApplierForTesting();
}

void KoPathPointMoveCommandContractTest::constructorsCopyOffsetsAndKeepFirstDuplicate()
{
    PathShapeToken firstStorage{17};
    PathShapeToken secondStorage{29};
    KoPathShape *const firstShape = pathShapePointer(firstStorage);
    KoPathShape *const secondShape = pathShapePointer(secondStorage);
    const KoPathPointData first(firstShape, KoPathPointIndex(1, 2));
    const KoPathPointData second(secondShape, KoPathPointIndex(3, 4));
    const QList<KoPathPointData> pointData{first, second, first};
    KUndo2Command parent;

    const QPointF commonOffset(2.5, -4.0);
    KoPathPointMoveCommand commonCommand(pointData, commonOffset, &parent);
    QCOMPARE(commandParents.value(&commonCommand), &parent);
    QCOMPARE(commandTexts.value(&commonCommand).toString(), QStringLiteral("Move points"));

    batchObservations.clear();
    commonCommand.redo();
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.first().points.size(), 2);
    verifyOffset(batchObservations.first(), first, commonOffset);
    verifyOffset(batchObservations.first(), second, commonOffset);

    const QPointF firstOffset(1.0, 2.0);
    const QPointF secondOffset(3.0, 4.0);
    const QPointF ignoredDuplicateOffset(50.0, 60.0);
    KoPathPointMoveCommand individualCommand(pointData, {firstOffset, secondOffset, ignoredDuplicateOffset}, &parent);
    QCOMPARE(commandParents.value(&individualCommand), &parent);
    QCOMPARE(commandTexts.value(&individualCommand).toString(), QStringLiteral("Move points"));

    batchObservations.clear();
    individualCommand.redo();
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.first().points.size(), 2);
    verifyOffset(batchObservations.first(), first, firstOffset);
    verifyOffset(batchObservations.first(), second, secondOffset);
    QCOMPARE(firstStorage.value, 17);
    QCOMPARE(secondStorage.value, 29);
}

void KoPathPointMoveCommandContractTest::redoAndUndoDispatchBaseBeforeSignedOffsetBatch()
{
    PathShapeToken firstStorage;
    PathShapeToken secondStorage;
    KoPathShape *const firstShape = pathShapePointer(firstStorage);
    KoPathShape *const secondShape = pathShapePointer(secondStorage);
    const KoPathPointData first(firstShape, KoPathPointIndex(0, 1));
    const KoPathPointData second(secondShape, KoPathPointIndex(2, 3));
    const QPointF firstOffset(5.5, -7.25);
    const QPointF secondOffset(-8.0, 9.0);
    KoPathPointMoveCommand command({first, second}, {firstOffset, secondOffset});

    operationTrace.clear();
    batchObservations.clear();
    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo"), QStringLiteral("offset-batch")}));
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.first().factor, 1.0);
    QVERIFY(batchObservations.first().paths == QSet<KoPathShape *>({firstShape, secondShape}));
    verifyOffset(batchObservations.first(), first, firstOffset);
    verifyOffset(batchObservations.first(), second, secondOffset);

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo"), QStringLiteral("offset-batch")}));
    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.last().factor, -1.0);
    QVERIFY(batchObservations.last().paths == QSet<KoPathShape *>({firstShape, secondShape}));
    verifyOffset(batchObservations.last(), first, firstOffset);
    verifyOffset(batchObservations.last(), second, secondOffset);
}

void KoPathPointMoveCommandContractTest::idAndMergeRequireSamePointSetAndAccumulateOffsets()
{
    PathShapeToken firstStorage;
    PathShapeToken secondStorage;
    KoPathShape *const firstShape = pathShapePointer(firstStorage);
    KoPathShape *const secondShape = pathShapePointer(secondStorage);
    const KoPathPointData first(firstShape, KoPathPointIndex(0, 1));
    const KoPathPointData second(secondShape, KoPathPointIndex(2, 3));
    const KoPathPointData changedIndex(firstShape, KoPathPointIndex(4, 5));
    KoPathPointMoveCommand command({first, second}, {QPointF(1.0, 2.0), QPointF(3.0, 4.0)});
    KoPathPointMoveCommand reordered({second, first}, {QPointF(30.0, 40.0), QPointF(10.0, 20.0)});
    KoPathPointMoveCommand differentPathSet({first}, QPointF(5.0, 6.0));
    KoPathPointMoveCommand differentPointSet({changedIndex, second}, QPointF(7.0, 8.0));
    KUndo2Command unrelated;

    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::ChangePathShapePointId));
    QVERIFY(command.mergeWith(&reordered));
    QVERIFY(!command.mergeWith(&differentPathSet));
    QVERIFY(!command.mergeWith(&differentPointSet));
    QVERIFY(!command.mergeWith(&unrelated));

    batchObservations.clear();
    command.redo();
    QCOMPARE(batchObservations.size(), 1);
    verifyOffset(batchObservations.first(), first, QPointF(11.0, 22.0));
    verifyOffset(batchObservations.first(), second, QPointF(33.0, 44.0));
    command.undo();
    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.last().factor, -1.0);
    verifyOffset(batchObservations.last(), first, QPointF(11.0, 22.0));
    verifyOffset(batchObservations.last(), second, QPointF(33.0, 44.0));
}

void KoPathPointMoveCommandContractTest::keepsShapesBorrowedAndSupportsEmptyInput()
{
    PathShapeToken storage{91};
    KoPathShape *const shape = pathShapePointer(storage);
    const KoPathPointData pointData(shape, KoPathPointIndex(6, 7));
    const int destructionCountBeforeCommand = baseDestructionCount;

    KUndo2Command *command = new KoPathPointMoveCommand({pointData}, QPointF(9.0, 10.0));
    delete command;
    QCOMPARE(baseDestructionCount, destructionCountBeforeCommand + 1);
    QCOMPARE(storage.value, 91);

    KoPathPointMoveCommand empty({}, QPointF(12.0, 13.0));
    operationTrace.clear();
    batchObservations.clear();
    empty.redo();
    empty.undo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("offset-batch"),
                          QStringLiteral("base-undo"),
                          QStringLiteral("offset-batch")}));
    QCOMPARE(batchObservations.size(), 2);
    QVERIFY(batchObservations.first().points.isEmpty());
    QVERIFY(batchObservations.first().paths.isEmpty());
    QCOMPARE(batchObservations.first().factor, 1.0);
    QVERIFY(batchObservations.last().points.isEmpty());
    QVERIFY(batchObservations.last().paths.isEmpty());
    QCOMPARE(batchObservations.last().factor, -1.0);
}

QTEST_GUILESS_MAIN(KoPathPointMoveCommandContractTest)

#include "KoPathPointMoveCommandContractTest.moc"
