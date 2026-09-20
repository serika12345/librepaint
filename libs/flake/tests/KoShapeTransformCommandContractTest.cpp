/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeBulkActionLock.h>
#include <KoShapeTransformCommand.h>

#include <kis_command_ids.h>

#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>

#include <algorithm>
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

KUndo2MagicString KUndo2Command::text() const
{
    return commandTexts.value(this);
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

void KoShape::setTransformation(const QTransform &)
{
    qFatal("the production transformation writer must not run in the shape transform command contract");
}

KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter(const QList<KoShape *> &)
{
    qFatal("the production bulk-action path must not run in the shape transform command contract");
}

void KoShapeBulkActionLockAdapter::lock()
{
    qFatal("the production bulk-action path must not run in the shape transform command contract");
}

void KoShapeBulkActionLockAdapter::unlock()
{
    qFatal("the production bulk-action path must not run in the shape transform command contract");
}

KoShapeBulkActionLockAdapter::UpdatesList KoShapeBulkActionLockAdapter::takeFinalUpdatesList()
{
    qFatal("the production bulk-action path must not run in the shape transform command contract");
}

KoShapeBulkActionLock::~KoShapeBulkActionLock()
{
    qFatal("the production bulk-action path must not run in the shape transform command contract");
}

void KoShapeBulkActionLock::bulkShapesUpdate(const UpdatesList &)
{
    qFatal("the production bulk-action path must not run in the shape transform command contract");
}

namespace KoShapeTransformCommandTesting
{
using TransformationBatchApplier = void (*)(const QList<KoShape *> &shapes, const QList<QTransform> &transformations);

void setTransformationBatchApplierForTesting(TransformationBatchApplier applier);
void resetTransformationBatchApplierForTesting();
} // namespace KoShapeTransformCommandTesting

namespace
{
struct BatchObservation {
    QList<KoShape *> shapes;
    QList<QTransform> transformations;
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QList<BatchObservation> batchObservations;

KoShape *shapePointer(ShapeToken &token)
{
    return reinterpret_cast<KoShape *>(&token);
}

void applyTransformationBatch(const QList<KoShape *> &shapes, const QList<QTransform> &transformations)
{
    batchObservations.append({shapes, transformations});
    operationTrace.append(QStringLiteral("transformation-batch"));
}

class TransformationBatchScope
{
public:
    TransformationBatchScope()
    {
        batchObservations.clear();
        operationTrace.clear();
        KoShapeTransformCommandTesting::setTransformationBatchApplierForTesting(applyTransformationBatch);
    }

    ~TransformationBatchScope()
    {
        KoShapeTransformCommandTesting::resetTransformationBatchApplierForTesting();
    }
};
} // namespace

class KoShapeTransformCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionCopiesTransformListsAndKeepsShapesBorrowed();
    void redoAndUndoApplyOneOrderedBatchAfterBasePhase();
    void reportsTransformShapeCommandIdentity();
    void mergeRequiresSameShapeOrderAndTextAndAdoptsLatestNewState();
};

void KoShapeTransformCommandContractTest::constructionCopiesTransformListsAndKeepsShapesBorrowed()
{
    TransformationBatchScope batchScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    const QList<KoShape *> expectedShapes{firstShape, secondShape};
    const QList<QTransform> expectedOldStates{QTransform::fromTranslate(1.0, 2.0), QTransform::fromScale(2.0, 3.0)};
    const QList<QTransform> expectedNewStates{QTransform::fromTranslate(10.0, 20.0), QTransform::fromScale(4.0, 5.0)};
    QList<KoShape *> shapes = expectedShapes;
    QList<QTransform> oldStates = expectedOldStates;
    QList<QTransform> newStates = expectedNewStates;
    KUndo2Command parent;
    const int destructionsBefore = baseDestructionCount;

    {
        KoShapeTransformCommand command(shapes, oldStates, newStates, &parent);

        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(operationTrace, QStringList());
        QCOMPARE(batchObservations.size(), 0);

        std::reverse(shapes.begin(), shapes.end());
        oldStates.fill(QTransform::fromTranslate(-1.0, -1.0));
        newStates.fill(QTransform::fromTranslate(-2.0, -2.0));

        command.redo();
        command.undo();
    }

    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.at(0).shapes, expectedShapes);
    QCOMPARE(batchObservations.at(0).transformations, expectedNewStates);
    QCOMPARE(batchObservations.at(1).shapes, expectedShapes);
    QCOMPARE(batchObservations.at(1).transformations, expectedOldStates);
    QCOMPARE(baseDestructionCount, destructionsBefore + 1);
    QCOMPARE(firstStorage.value, static_cast<unsigned char>(0));
    QCOMPARE(secondStorage.value, static_cast<unsigned char>(0));
}

void KoShapeTransformCommandContractTest::redoAndUndoApplyOneOrderedBatchAfterBasePhase()
{
    TransformationBatchScope batchScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    const QList<KoShape *> shapes{firstShape, secondShape};
    const QList<QTransform> oldStates{QTransform::fromTranslate(-3.0, 7.0), QTransform::fromScale(0.5, 2.0)};
    const QList<QTransform> newStates{QTransform::fromTranslate(13.0, 17.0), QTransform::fromScale(3.0, 4.0)};
    KoShapeTransformCommand command(shapes, oldStates, newStates);

    command.redo();
    command.undo();

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("transformation-batch"),
                          QStringLiteral("base-undo"),
                          QStringLiteral("transformation-batch")}));
    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.at(0).shapes, shapes);
    QCOMPARE(batchObservations.at(0).transformations, newStates);
    QCOMPARE(batchObservations.at(1).shapes, shapes);
    QCOMPARE(batchObservations.at(1).transformations, oldStates);

    batchObservations.clear();
    operationTrace.clear();
    KoShapeTransformCommand emptyCommand({}, {}, {});
    emptyCommand.redo();
    emptyCommand.undo();
    QCOMPARE(batchObservations.size(), 2);
    QVERIFY(batchObservations.at(0).shapes.isEmpty());
    QVERIFY(batchObservations.at(0).transformations.isEmpty());
    QVERIFY(batchObservations.at(1).shapes.isEmpty());
    QVERIFY(batchObservations.at(1).transformations.isEmpty());
}

void KoShapeTransformCommandContractTest::reportsTransformShapeCommandIdentity()
{
    TransformationBatchScope batchScope;
    KoShapeTransformCommand command({}, {}, {});

    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::TransformShapeId));
}

void KoShapeTransformCommandContractTest::mergeRequiresSameShapeOrderAndTextAndAdoptsLatestNewState()
{
    TransformationBatchScope batchScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *firstShape = shapePointer(firstStorage);
    KoShape *secondShape = shapePointer(secondStorage);
    const QList<KoShape *> shapes{firstShape, secondShape};
    const QList<KoShape *> reversedShapes{secondShape, firstShape};
    const QList<QTransform> oldStates{QTransform::fromTranslate(1.0, 2.0), QTransform::fromTranslate(3.0, 4.0)};
    const QList<QTransform> initialNewStates{QTransform::fromScale(2.0, 2.0), QTransform::fromScale(3.0, 3.0)};
    const QList<QTransform> latestNewStates{QTransform::fromTranslate(20.0, 30.0),
                                            QTransform::fromTranslate(40.0, 50.0)};
    const QList<QTransform> reversedOldStates{oldStates.at(1), oldStates.at(0)};
    const QList<QTransform> reversedNewStates{latestNewStates.at(1), latestNewStates.at(0)};
    KoShapeTransformCommand command(shapes, oldStates, initialNewStates);
    KoShapeTransformCommand latest(shapes, oldStates, latestNewStates);
    KoShapeTransformCommand differentOrder(reversedShapes, reversedOldStates, reversedNewStates);
    KoShapeTransformCommand differentText(shapes, oldStates, latestNewStates);
    KUndo2Command differentType;
    const KUndo2MagicString sharedText = kundo2_noi18n(QStringLiteral("transform-shapes"));

    command.setText(sharedText);
    latest.setText(sharedText);
    differentOrder.setText(sharedText);
    differentText.setText(kundo2_noi18n(QStringLiteral("different-operation")));

    QVERIFY(!command.mergeWith(&differentType));
    QVERIFY(!command.mergeWith(&differentOrder));
    QVERIFY(!command.mergeWith(&differentText));
    QVERIFY(command.mergeWith(&latest));

    batchObservations.clear();
    operationTrace.clear();
    command.redo();
    command.undo();

    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.at(0).shapes, shapes);
    QCOMPARE(batchObservations.at(0).transformations, latestNewStates);
    QCOMPARE(batchObservations.at(1).shapes, shapes);
    QCOMPARE(batchObservations.at(1).transformations, oldStates);
}

QTEST_GUILESS_MAIN(KoShapeTransformCommandContractTest)

#include "KoShapeTransformCommandContractTest.moc"
