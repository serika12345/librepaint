/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeAlignCommand.h>
#include <KoShapeBulkActionLock.h>

#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>

#include <cstddef>
#include <type_traits>

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
    qFatal("the alignment command contract must not reach a failed product assertion");
}

QPointF KoShape::absolutePosition(KoFlake::AnchorPosition) const
{
    qFatal("the production position reader must not run in the alignment command contract");
    return {};
}

QRectF KoShape::absoluteOutlineRect() const
{
    qFatal("the production outline reader must not run in the alignment command contract");
    return {};
}

void KoShape::setAbsolutePosition(const QPointF &, KoFlake::AnchorPosition)
{
    qFatal("the production position writer must not run in the alignment command contract");
}

KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter(const QList<KoShape *> &)
{
    qFatal("the production bulk-action path must not run in the alignment command contract");
}

void KoShapeBulkActionLockAdapter::lock()
{
    qFatal("the production bulk-action path must not run in the alignment command contract");
}

void KoShapeBulkActionLockAdapter::unlock()
{
    qFatal("the production bulk-action path must not run in the alignment command contract");
}

KoShapeBulkActionLockAdapter::UpdatesList KoShapeBulkActionLockAdapter::takeFinalUpdatesList()
{
    qFatal("the production bulk-action path must not run in the alignment command contract");
}

KoShapeBulkActionLock::~KoShapeBulkActionLock()
{
    qFatal("the production bulk-action path must not run in the alignment command contract");
}

void KoShapeBulkActionLock::bulkShapesUpdate(const UpdatesList &)
{
    qFatal("the production bulk-action path must not run in the alignment command contract");
}

namespace KoShapeAlignCommandTesting
{
using PositionReader = QPointF (*)(const KoShape *shape);
using OutlineRectReader = QRectF (*)(const KoShape *shape);

void setShapeGeometryForTesting(PositionReader positionReader, OutlineRectReader outlineRectReader);
void resetShapeGeometryForTesting();
} // namespace KoShapeAlignCommandTesting

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
    QRectF outlineRect;
    int positionReadCount{0};
    int outlineReadCount{0};
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

QPointF readAlignmentPosition(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.positionReadCount;
    operationTrace.append(QStringLiteral("position-read:%1").arg(observation.label));
    return observation.position;
}

QRectF readAlignmentOutlineRect(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.outlineReadCount;
    operationTrace.append(QStringLiteral("outline-read:%1").arg(observation.label));
    return observation.outlineRect;
}

QPointF rejectMovePositionRead(const KoShape *, KoFlake::AnchorPosition)
{
    qFatal("the alignment command must use the explicit-position move constructor");
    return {};
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
        KoShapeAlignCommandTesting::setShapeGeometryForTesting(readAlignmentPosition, readAlignmentOutlineRect);
        KoShapeMoveCommandTesting::setShapeAccessForTesting(rejectMovePositionRead, applyPositionBatch);
    }

    ~ShapeAccessScope()
    {
        KoShapeMoveCommandTesting::resetShapeAccessForTesting();
        KoShapeAlignCommandTesting::resetShapeGeometryForTesting();
    }
};

const QStringList expectedConstructionTrace{
    QStringLiteral("position-read:first"),
    QStringLiteral("outline-read:first"),
    QStringLiteral("position-read:second"),
    QStringLiteral("outline-read:second"),
};

void resetGeometry(const KoShape *firstShape, const KoShape *secondShape)
{
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), QPointF(25.0, 50.0), QRectF(10.0, 20.0, 30.0, 40.0), 0, 0}},
        {secondShape, {QStringLiteral("second"), QPointF(0.0, 0.0), QRectF(30.0, 40.0, 10.0, 20.0), 0, 0}},
    };
    batchObservations.clear();
    operationTrace.clear();
}

void verifyConstruction(const KoShape *firstShape, const KoShape *secondShape)
{
    QCOMPARE(operationTrace, expectedConstructionTrace);
    QCOMPARE(shapeObservations[firstShape].positionReadCount, 1);
    QCOMPARE(shapeObservations[firstShape].outlineReadCount, 1);
    QCOMPARE(shapeObservations[secondShape].positionReadCount, 1);
    QCOMPARE(shapeObservations[secondShape].outlineReadCount, 1);
}

} // namespace

class KoShapeAlignCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void alignmentValuesRemainStable();
    void horizontalAlignmentsUseOutlineEdgesAndPreserveVerticalPosition();
    void verticalAlignmentsUseOutlineEdgesAndPreserveHorizontalPosition();
    void redoUndoDelegateThroughMoveCommandAndKeepBorrowedLifetime();
};

void KoShapeAlignCommandContractTest::alignmentValuesRemainStable()
{
    QVERIFY((std::is_base_of_v<KUndo2Command, KoShapeAlignCommand>));
    QVERIFY(std::has_virtual_destructor_v<KoShapeAlignCommand>);
    QCOMPARE(static_cast<int>(KoShapeAlignCommand::HorizontalLeftAlignment), 0);
    QCOMPARE(static_cast<int>(KoShapeAlignCommand::HorizontalCenterAlignment), 1);
    QCOMPARE(static_cast<int>(KoShapeAlignCommand::HorizontalRightAlignment), 2);
    QCOMPARE(static_cast<int>(KoShapeAlignCommand::VerticalBottomAlignment), 3);
    QCOMPARE(static_cast<int>(KoShapeAlignCommand::VerticalCenterAlignment), 4);
    QCOMPARE(static_cast<int>(KoShapeAlignCommand::VerticalTopAlignment), 5);
}

void KoShapeAlignCommandContractTest::horizontalAlignmentsUseOutlineEdgesAndPreserveVerticalPosition()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    const QList<KoShape *> shapes{firstShape, secondShape};
    const QRectF targetRect(100.0, 200.0, 80.0, 60.0);

    const QList<KoShapeAlignCommand::Align> modes{
        KoShapeAlignCommand::HorizontalLeftAlignment,
        KoShapeAlignCommand::HorizontalCenterAlignment,
        KoShapeAlignCommand::HorizontalRightAlignment,
    };
    const QList<QList<QPointF>> expectedPositions{
        {QPointF(115.0, 50.0), QPointF(70.0, 0.0)},
        {QPointF(140.0, 50.0), QPointF(105.0, 0.0)},
        {QPointF(165.0, 50.0), QPointF(140.0, 0.0)},
    };

    for (int i = 0; i < modes.size(); ++i) {
        resetGeometry(firstShape, secondShape);
        KoShapeAlignCommand command(shapes, modes.at(i), targetRect);
        verifyConstruction(firstShape, secondShape);

        operationTrace.clear();
        command.redo();

        QCOMPARE(
            operationTrace,
            QStringList({QStringLiteral("base-redo"), QStringLiteral("base-redo"), QStringLiteral("position-batch")}));
        QCOMPARE(batchObservations.size(), 1);
        QCOMPARE(batchObservations.constFirst().shapes, shapes);
        QCOMPARE(batchObservations.constFirst().positions, expectedPositions.at(i));
        QCOMPARE(static_cast<int>(batchObservations.constFirst().anchor), static_cast<int>(KoFlake::Center));
    }
}

void KoShapeAlignCommandContractTest::verticalAlignmentsUseOutlineEdgesAndPreserveHorizontalPosition()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    const QList<KoShape *> shapes{firstShape, secondShape};
    const QRectF targetRect(100.0, 200.0, 80.0, 60.0);

    const QList<KoShapeAlignCommand::Align> modes{
        KoShapeAlignCommand::VerticalTopAlignment,
        KoShapeAlignCommand::VerticalCenterAlignment,
        KoShapeAlignCommand::VerticalBottomAlignment,
    };
    const QList<QList<QPointF>> expectedPositions{
        {QPointF(25.0, 230.0), QPointF(0.0, 160.0)},
        {QPointF(25.0, 240.0), QPointF(0.0, 180.0)},
        {QPointF(25.0, 250.0), QPointF(0.0, 200.0)},
    };

    for (int i = 0; i < modes.size(); ++i) {
        resetGeometry(firstShape, secondShape);
        KoShapeAlignCommand command(shapes, modes.at(i), targetRect);
        verifyConstruction(firstShape, secondShape);

        operationTrace.clear();
        command.redo();

        QCOMPARE(
            operationTrace,
            QStringList({QStringLiteral("base-redo"), QStringLiteral("base-redo"), QStringLiteral("position-batch")}));
        QCOMPARE(batchObservations.size(), 1);
        QCOMPARE(batchObservations.constFirst().shapes, shapes);
        QCOMPARE(batchObservations.constFirst().positions, expectedPositions.at(i));
        QCOMPARE(static_cast<int>(batchObservations.constFirst().anchor), static_cast<int>(KoFlake::Center));
    }
}

void KoShapeAlignCommandContractTest::redoUndoDelegateThroughMoveCommandAndKeepBorrowedLifetime()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    firstStorage.value = 17;
    secondStorage.value = 29;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    const QList<KoShape *> shapes{firstShape, secondShape};
    const QList<QPointF> originalPositions{QPointF(25.0, 50.0), QPointF(0.0, 0.0)};
    const QList<QPointF> alignedPositions{QPointF(115.0, 50.0), QPointF(70.0, 0.0)};
    KUndo2Command parent;
    const int destructionsBefore = baseDestructionCount;

    resetGeometry(firstShape, secondShape);
    {
        KoShapeAlignCommand command(shapes,
                                    KoShapeAlignCommand::HorizontalLeftAlignment,
                                    QRectF(100.0, 200.0, 80.0, 60.0),
                                    &parent);
        QCOMPARE(commandParents.value(&command), &parent);

        operationTrace.clear();
        batchObservations.clear();
        command.redo();
        command.undo();

        QCOMPARE(operationTrace,
                 QStringList({QStringLiteral("base-redo"),
                              QStringLiteral("base-redo"),
                              QStringLiteral("position-batch"),
                              QStringLiteral("base-undo"),
                              QStringLiteral("base-undo"),
                              QStringLiteral("position-batch")}));
        QCOMPARE(batchObservations.size(), 2);
        QCOMPARE(batchObservations.at(0).shapes, shapes);
        QCOMPARE(batchObservations.at(0).positions, alignedPositions);
        QCOMPARE(static_cast<int>(batchObservations.at(0).anchor), static_cast<int>(KoFlake::Center));
        QCOMPARE(batchObservations.at(1).shapes, shapes);
        QCOMPARE(batchObservations.at(1).positions, originalPositions);
        QCOMPARE(static_cast<int>(batchObservations.at(1).anchor), static_cast<int>(KoFlake::Center));
    }

    QCOMPARE(baseDestructionCount, destructionsBefore + 2);
    QCOMPARE(firstStorage.value, static_cast<unsigned char>(17));
    QCOMPARE(secondStorage.value, static_cast<unsigned char>(29));
}

QTEST_GUILESS_MAIN(KoShapeAlignCommandContractTest)

#include "KoShapeAlignCommandContractTest.moc"
