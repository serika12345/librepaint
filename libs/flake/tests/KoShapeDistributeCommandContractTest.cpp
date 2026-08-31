/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeBulkActionLock.h>
#include <KoShapeDistributeCommand.h>

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
    qFatal("the distribution command contract must not reach a failed product assertion");
}

QPointF KoShape::absolutePosition(KoFlake::AnchorPosition) const
{
    qFatal("the production position reader must not run in the distribution command contract");
    return {};
}

QRectF KoShape::absoluteOutlineRect() const
{
    qFatal("the production outline reader must not run in the distribution command contract");
    return {};
}

void KoShape::setAbsolutePosition(const QPointF &, KoFlake::AnchorPosition)
{
    qFatal("the production position writer must not run in the distribution command contract");
}

KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter(const QList<KoShape *> &)
{
    qFatal("the production bulk-action path must not run in the distribution command contract");
}

void KoShapeBulkActionLockAdapter::lock()
{
    qFatal("the production bulk-action path must not run in the distribution command contract");
}

void KoShapeBulkActionLockAdapter::unlock()
{
    qFatal("the production bulk-action path must not run in the distribution command contract");
}

KoShapeBulkActionLockAdapter::UpdatesList KoShapeBulkActionLockAdapter::takeFinalUpdatesList()
{
    qFatal("the production bulk-action path must not run in the distribution command contract");
}

KoShapeBulkActionLock::~KoShapeBulkActionLock()
{
    qFatal("the production bulk-action path must not run in the distribution command contract");
}

void KoShapeBulkActionLock::bulkShapesUpdate(const UpdatesList &)
{
    qFatal("the production bulk-action path must not run in the distribution command contract");
}

namespace KoShapeDistributeCommandTesting
{
using PositionReader = QPointF (*)(const KoShape *shape);
using OutlineRectReader = QRectF (*)(const KoShape *shape);

void setShapeGeometryForTesting(PositionReader positionReader, OutlineRectReader outlineRectReader);
void resetShapeGeometryForTesting();
} // namespace KoShapeDistributeCommandTesting

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
QStringList positionReadOrder;

KoShape *shapePointer(ShapeToken &token)
{
    return reinterpret_cast<KoShape *>(&token);
}

QPointF readDistributionPosition(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.positionReadCount;
    positionReadOrder.append(observation.label);
    return observation.position;
}

QRectF readDistributionOutlineRect(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.outlineReadCount;
    return observation.outlineRect;
}

QPointF rejectMovePositionRead(const KoShape *, KoFlake::AnchorPosition)
{
    qFatal("the distribution command must use the explicit-position move constructor");
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
        positionReadOrder.clear();
        operationTrace.clear();
        KoShapeDistributeCommandTesting::setShapeGeometryForTesting(readDistributionPosition,
                                                                    readDistributionOutlineRect);
        KoShapeMoveCommandTesting::setShapeAccessForTesting(rejectMovePositionRead, applyPositionBatch);
    }

    ~ShapeAccessScope()
    {
        KoShapeMoveCommandTesting::resetShapeAccessForTesting();
        KoShapeDistributeCommandTesting::resetShapeGeometryForTesting();
    }
};

void resetGeometry(const KoShape *firstShape, const KoShape *secondShape, const KoShape *thirdShape)
{
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), QPointF(10.0, 20.0), QRectF(20.0, 30.0, 10.0, 20.0), 0, 0}},
        {secondShape, {QStringLiteral("second"), QPointF(30.0, 40.0), QRectF(50.0, 60.0, 40.0, 10.0), 0, 0}},
        {thirdShape, {QStringLiteral("third"), QPointF(50.0, 60.0), QRectF(80.0, 90.0, 30.0, 30.0), 0, 0}},
    };
    batchObservations.clear();
    positionReadOrder.clear();
    operationTrace.clear();
}

void verifyDistribution(KoShapeDistributeCommand::Distribute mode,
                        const QList<KoShape *> &inputShapes,
                        const QList<KoShape *> &sortedShapes,
                        const QList<QPointF> &expectedPositions)
{
    KoShapeDistributeCommand command(inputShapes, mode, QRectF(100.0, 200.0, 120.0, 90.0));

    QCOMPARE(positionReadOrder,
             QStringList({QStringLiteral("first"), QStringLiteral("second"), QStringLiteral("third")}));
    for (KoShape *shape : sortedShapes) {
        QCOMPARE(shapeObservations[shape].positionReadCount, 1);
        QVERIFY(shapeObservations[shape].outlineReadCount >= 2);
    }

    operationTrace.clear();
    batchObservations.clear();
    command.redo();

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"), QStringLiteral("base-redo"), QStringLiteral("position-batch")}));
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.constFirst().shapes, sortedShapes);
    QCOMPARE(batchObservations.constFirst().positions, expectedPositions);
    QCOMPARE(static_cast<int>(batchObservations.constFirst().anchor), static_cast<int>(KoFlake::Center));
}
} // namespace

class KoShapeDistributeCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void distributionValuesRemainStable();
    void horizontalCenterLeftAndRightDistributionsUseSortedOutlineEdges();
    void horizontalGapDistributionAccountsForEveryWidth();
    void verticalCenterTopAndBottomDistributionsUseSortedOutlineEdges();
    void verticalGapDistributionAccountsForEveryHeight();
    void redoUndoDelegateThroughMoveCommandAndKeepBorrowedLifetime();
};

void KoShapeDistributeCommandContractTest::distributionValuesRemainStable()
{
    QVERIFY((std::is_base_of_v<KUndo2Command, KoShapeDistributeCommand>));
    QVERIFY(std::has_virtual_destructor_v<KoShapeDistributeCommand>);
    QCOMPARE(static_cast<int>(KoShapeDistributeCommand::HorizontalCenterDistribution), 0);
    QCOMPARE(static_cast<int>(KoShapeDistributeCommand::HorizontalGapsDistribution), 1);
    QCOMPARE(static_cast<int>(KoShapeDistributeCommand::HorizontalLeftDistribution), 2);
    QCOMPARE(static_cast<int>(KoShapeDistributeCommand::HorizontalRightDistribution), 3);
    QCOMPARE(static_cast<int>(KoShapeDistributeCommand::VerticalCenterDistribution), 4);
    QCOMPARE(static_cast<int>(KoShapeDistributeCommand::VerticalGapsDistribution), 5);
    QCOMPARE(static_cast<int>(KoShapeDistributeCommand::VerticalBottomDistribution), 6);
    QCOMPARE(static_cast<int>(KoShapeDistributeCommand::VerticalTopDistribution), 7);
}

void KoShapeDistributeCommandContractTest::horizontalCenterLeftAndRightDistributionsUseSortedOutlineEdges()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    ShapeToken thirdStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    KoShape *thirdShape = shapePointer(thirdStorage);
    const QList<KoShape *> inputShapes{thirdShape, firstShape, secondShape};
    const QList<KoShape *> sortedShapes{firstShape, secondShape, thirdShape};

    const QList<KoShapeDistributeCommand::Distribute> modes{
        KoShapeDistributeCommand::HorizontalCenterDistribution,
        KoShapeDistributeCommand::HorizontalLeftDistribution,
        KoShapeDistributeCommand::HorizontalRightDistribution,
    };
    const QList<QList<QPointF>> expectedPositions{
        {QPointF(90.0, 20.0), QPointF(115.0, 40.0), QPointF(160.0, 60.0)},
        {QPointF(90.0, 20.0), QPointF(125.0, 40.0), QPointF(160.0, 60.0)},
        {QPointF(90.0, 20.0), QPointF(105.0, 40.0), QPointF(160.0, 60.0)},
    };

    for (int i = 0; i < modes.size(); ++i) {
        resetGeometry(firstShape, secondShape, thirdShape);
        verifyDistribution(modes.at(i), inputShapes, sortedShapes, expectedPositions.at(i));
    }
}

void KoShapeDistributeCommandContractTest::horizontalGapDistributionAccountsForEveryWidth()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    ShapeToken thirdStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    KoShape *thirdShape = shapePointer(thirdStorage);

    resetGeometry(firstShape, secondShape, thirdShape);
    verifyDistribution(KoShapeDistributeCommand::HorizontalGapsDistribution,
                       {thirdShape, firstShape, secondShape},
                       {firstShape, secondShape, thirdShape},
                       {QPointF(90.0, 20.0), QPointF(110.0, 40.0), QPointF(160.0, 60.0)});
}

void KoShapeDistributeCommandContractTest::verticalCenterTopAndBottomDistributionsUseSortedOutlineEdges()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    ShapeToken thirdStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    KoShape *thirdShape = shapePointer(thirdStorage);
    const QList<KoShape *> inputShapes{thirdShape, firstShape, secondShape};
    const QList<KoShape *> sortedShapes{firstShape, secondShape, thirdShape};

    const QList<KoShapeDistributeCommand::Distribute> modes{
        KoShapeDistributeCommand::VerticalCenterDistribution,
        KoShapeDistributeCommand::VerticalTopDistribution,
        KoShapeDistributeCommand::VerticalBottomDistribution,
    };
    const QList<QList<QPointF>> expectedPositions{
        {QPointF(10.0, 190.0), QPointF(30.0, 217.5), QPointF(50.0, 230.0)},
        {QPointF(10.0, 190.0), QPointF(30.0, 210.0), QPointF(50.0, 230.0)},
        {QPointF(10.0, 190.0), QPointF(30.0, 225.0), QPointF(50.0, 230.0)},
    };

    for (int i = 0; i < modes.size(); ++i) {
        resetGeometry(firstShape, secondShape, thirdShape);
        verifyDistribution(modes.at(i), inputShapes, sortedShapes, expectedPositions.at(i));
    }
}

void KoShapeDistributeCommandContractTest::verticalGapDistributionAccountsForEveryHeight()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    ShapeToken thirdStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    KoShape *thirdShape = shapePointer(thirdStorage);

    resetGeometry(firstShape, secondShape, thirdShape);
    verifyDistribution(KoShapeDistributeCommand::VerticalGapsDistribution,
                       {thirdShape, firstShape, secondShape},
                       {firstShape, secondShape, thirdShape},
                       {QPointF(10.0, 190.0), QPointF(30.0, 215.0), QPointF(50.0, 230.0)});
}

void KoShapeDistributeCommandContractTest::redoUndoDelegateThroughMoveCommandAndKeepBorrowedLifetime()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    ShapeToken thirdStorage;
    firstStorage.value = 17;
    secondStorage.value = 29;
    thirdStorage.value = 43;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    KoShape *thirdShape = shapePointer(thirdStorage);
    const QList<KoShape *> inputShapes{thirdShape, firstShape, secondShape};
    const QList<KoShape *> sortedShapes{firstShape, secondShape, thirdShape};
    const QList<QPointF> originalPositions{QPointF(10.0, 20.0), QPointF(30.0, 40.0), QPointF(50.0, 60.0)};
    const QList<QPointF> distributedPositions{QPointF(90.0, 20.0), QPointF(110.0, 40.0), QPointF(160.0, 60.0)};
    KUndo2Command parent;
    const int destructionsBefore = baseDestructionCount;

    resetGeometry(firstShape, secondShape, thirdShape);
    {
        KoShapeDistributeCommand command(inputShapes,
                                         KoShapeDistributeCommand::HorizontalGapsDistribution,
                                         QRectF(100.0, 200.0, 120.0, 90.0),
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
        QCOMPARE(batchObservations.at(0).shapes, sortedShapes);
        QCOMPARE(batchObservations.at(0).positions, distributedPositions);
        QCOMPARE(static_cast<int>(batchObservations.at(0).anchor), static_cast<int>(KoFlake::Center));
        QCOMPARE(batchObservations.at(1).shapes, sortedShapes);
        QCOMPARE(batchObservations.at(1).positions, originalPositions);
        QCOMPARE(static_cast<int>(batchObservations.at(1).anchor), static_cast<int>(KoFlake::Center));
    }

    QCOMPARE(baseDestructionCount, destructionsBefore + 2);
    QCOMPARE(firstStorage.value, static_cast<unsigned char>(17));
    QCOMPARE(secondStorage.value, static_cast<unsigned char>(29));
    QCOMPARE(thirdStorage.value, static_cast<unsigned char>(43));
}

QTEST_GUILESS_MAIN(KoShapeDistributeCommandContractTest)

#include "KoShapeDistributeCommandContractTest.moc"
