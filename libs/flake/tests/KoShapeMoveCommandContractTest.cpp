/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeBulkActionLock.h>
#include <KoShapeMoveCommand.h>

#include <kis_command_ids.h>

#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace
{
QHash<const KUndo2Command *, KUndo2MagicString> commandTexts;
QHash<const KUndo2Command *, KUndo2Command *> commandParents;
QStringList operationTrace;
int baseDestructionCount = 0;
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

void kis_assert_exception(const char *, const char *, int)
{
    qFatal("the move command contract must not reach a failed product assertion");
}

QPointF KoShape::absolutePosition(KoFlake::AnchorPosition) const
{
    qFatal("the production position reader must not run in the move command contract");
    return {};
}

void KoShape::setAbsolutePosition(const QPointF &, KoFlake::AnchorPosition)
{
    qFatal("the production position writer must not run in the move command contract");
}

KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter(const QList<KoShape *> &)
{
    qFatal("the production bulk-action path must not run in the move command contract");
}

void KoShapeBulkActionLockAdapter::lock()
{
    qFatal("the production bulk-action path must not run in the move command contract");
}

void KoShapeBulkActionLockAdapter::unlock()
{
    qFatal("the production bulk-action path must not run in the move command contract");
}

KoShapeBulkActionLockAdapter::UpdatesList KoShapeBulkActionLockAdapter::takeFinalUpdatesList()
{
    qFatal("the production bulk-action path must not run in the move command contract");
}

KoShapeBulkActionLock::~KoShapeBulkActionLock()
{
    qFatal("the production bulk-action path must not run in the move command contract");
}

void KoShapeBulkActionLock::bulkShapesUpdate(const UpdatesList &)
{
    qFatal("the production bulk-action path must not run in the move command contract");
}

namespace KoShapeMoveCommandTesting
{
using PositionReader = QPointF (*)(const KoShape *shape, KoFlake::AnchorPosition anchor);
using PositionBatchApplier = void (*)(const QList<KoShape *> &shapes,
                                      const QList<QPointF> &positions,
                                      KoFlake::AnchorPosition anchor);

void setShapeAccessForTesting(PositionReader reader, PositionBatchApplier applier);
void resetShapeAccessForTesting();
} // namespace KoShapeMoveCommandTesting

namespace
{
struct ShapeObservation {
    QString label;
    QPointF position;
    int readCount{0};
    QList<KoFlake::AnchorPosition> readAnchors;
};

struct BatchObservation {
    QList<KoShape *> shapes;
    QList<QPointF> positions;
    KoFlake::AnchorPosition anchor;
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoShape *, ShapeObservation> shapeObservations;
QList<BatchObservation> batchObservations;

KoShape *shapePointer(ShapeToken &token)
{
    return reinterpret_cast<KoShape *>(&token);
}

QPointF readPosition(const KoShape *shape, KoFlake::AnchorPosition anchor)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.readCount;
    observation.readAnchors.append(anchor);
    operationTrace.append(QStringLiteral("position-read:%1").arg(observation.label));
    return observation.position;
}

void applyPositionBatch(const QList<KoShape *> &shapes, const QList<QPointF> &positions, KoFlake::AnchorPosition anchor)
{
    batchObservations.append({shapes, positions, anchor});
    operationTrace.append(QStringLiteral("position-batch"));

    const int count = qMin(shapes.size(), positions.size());
    for (int i = 0; i < count; ++i) {
        shapeObservations[shapes.at(i)].position = positions.at(i);
    }
}

class ShapeAccessScope
{
public:
    ShapeAccessScope()
    {
        shapeObservations.clear();
        batchObservations.clear();
        operationTrace.clear();
        KoShapeMoveCommandTesting::setShapeAccessForTesting(readPosition, applyPositionBatch);
    }

    ~ShapeAccessScope()
    {
        KoShapeMoveCommandTesting::resetShapeAccessForTesting();
    }
};
} // namespace

class KoShapeMoveCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void explicitPositionsPreservePairingAnchorParentAndBorrowing();
    void offsetConstructorReadsCentersAndBuildsTranslatedEndpoints();
    void redoAndUndoApplyOneOrderedBatchAfterBasePhase();
    void mergeRequiresMatchingShapeOrderAndAnchorAndAdoptsLatestEndpoint();
};

void KoShapeMoveCommandContractTest::explicitPositionsPreservePairingAnchorParentAndBorrowing()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    const QList<KoShape *> shapes{firstShape, secondShape};
    QList<QPointF> previousPositions{QPointF(1.0, 2.0), QPointF(-3.0, 4.0)};
    QList<QPointF> newPositions{QPointF(11.0, 12.0), QPointF(7.0, 24.0)};
    KUndo2Command parent;
    const int destructionsBefore = baseDestructionCount;

    operationTrace.clear();
    {
        KoShapeMoveCommand command(shapes, previousPositions, newPositions, KoFlake::TopLeft, &parent);

        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(operationTrace, QStringList());
        QCOMPARE(batchObservations.size(), 0);
    }

    QCOMPARE(baseDestructionCount, destructionsBefore + 1);
    QCOMPARE(firstStorage.value, static_cast<unsigned char>(0));
    QCOMPARE(secondStorage.value, static_cast<unsigned char>(0));
}

void KoShapeMoveCommandContractTest::offsetConstructorReadsCentersAndBuildsTranslatedEndpoints()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), QPointF(1.5, -2.0), 0, {}}},
        {secondShape, {QStringLiteral("second"), QPointF(-4.0, 8.5), 0, {}}},
    };
    batchObservations.clear();
    operationTrace.clear();

    KoShapeMoveCommand command({firstShape, secondShape}, QPointF(3.0, -5.0));

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("position-read:first"), QStringLiteral("position-read:second")}));
    QCOMPARE(shapeObservations[firstShape].readCount, 1);
    QCOMPARE(shapeObservations[secondShape].readCount, 1);
    QCOMPARE(static_cast<int>(shapeObservations[firstShape].readAnchors.constFirst()),
             static_cast<int>(KoFlake::Center));
    QCOMPARE(static_cast<int>(shapeObservations[secondShape].readAnchors.constFirst()),
             static_cast<int>(KoFlake::Center));

    operationTrace.clear();
    command.redo();

    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo"), QStringLiteral("position-batch")}));
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.constFirst().shapes, QList<KoShape *>({firstShape, secondShape}));
    QCOMPARE(batchObservations.constFirst().positions, QList<QPointF>({QPointF(4.5, -7.0), QPointF(-1.0, 3.5)}));
    QCOMPARE(static_cast<int>(batchObservations.constFirst().anchor), static_cast<int>(KoFlake::Center));
}

void KoShapeMoveCommandContractTest::redoAndUndoApplyOneOrderedBatchAfterBasePhase()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    const QList<KoShape *> shapes{firstShape, secondShape};
    QList<QPointF> previousPositions{QPointF(2.0, 4.0), QPointF(6.0, 8.0)};
    QList<QPointF> newPositions{QPointF(20.0, 40.0), QPointF(60.0, 80.0)};
    KoShapeMoveCommand command(shapes, previousPositions, newPositions, KoFlake::BottomRight);

    batchObservations.clear();
    operationTrace.clear();
    command.redo();
    command.undo();

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("position-batch"),
                          QStringLiteral("base-undo"),
                          QStringLiteral("position-batch")}));
    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.at(0).shapes, shapes);
    QCOMPARE(batchObservations.at(0).positions, newPositions);
    QCOMPARE(static_cast<int>(batchObservations.at(0).anchor), static_cast<int>(KoFlake::BottomRight));
    QCOMPARE(batchObservations.at(1).shapes, shapes);
    QCOMPARE(batchObservations.at(1).positions, previousPositions);
    QCOMPARE(static_cast<int>(batchObservations.at(1).anchor), static_cast<int>(KoFlake::BottomRight));
}

void KoShapeMoveCommandContractTest::mergeRequiresMatchingShapeOrderAndAnchorAndAdoptsLatestEndpoint()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    QList<KoShape *> shapes{firstShape, secondShape};
    QList<KoShape *> reversedShapes{secondShape, firstShape};
    QList<QPointF> previousPositions{QPointF(0.0, 1.0), QPointF(2.0, 3.0)};
    QList<QPointF> initialPositions{QPointF(10.0, 11.0), QPointF(12.0, 13.0)};
    QList<QPointF> latestPositions{QPointF(20.0, 21.0), QPointF(22.0, 23.0)};
    QList<QPointF> reversedPrevious{previousPositions.at(1), previousPositions.at(0)};
    QList<QPointF> reversedLatest{latestPositions.at(1), latestPositions.at(0)};
    KoShapeMoveCommand command(shapes, previousPositions, initialPositions, KoFlake::Right);
    KoShapeMoveCommand latest(shapes, previousPositions, latestPositions, KoFlake::Right);
    KoShapeMoveCommand differentOrder(reversedShapes, reversedPrevious, reversedLatest, KoFlake::Right);
    KoShapeMoveCommand differentAnchor(shapes, previousPositions, latestPositions, KoFlake::Left);

    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::MoveShapeId));
    QCOMPARE(command.id(), 9999);
    QVERIFY(command.mergeWith(&latest));
    QVERIFY(!command.mergeWith(&differentOrder));
    QVERIFY(!command.mergeWith(&differentAnchor));

    batchObservations.clear();
    operationTrace.clear();
    command.redo();

    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo"), QStringLiteral("position-batch")}));
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.constFirst().shapes, shapes);
    QCOMPARE(batchObservations.constFirst().positions, latestPositions);
    QCOMPARE(static_cast<int>(batchObservations.constFirst().anchor), static_cast<int>(KoFlake::Right));
}

QTEST_GUILESS_MAIN(KoShapeMoveCommandContractTest)

#include "KoShapeMoveCommandContractTest.moc"
