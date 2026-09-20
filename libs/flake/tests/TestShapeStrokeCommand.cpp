/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestShapeStrokeCommand.h"

#include "KoShapeStrokeCommand.h"

#include <KoShape.h>
#include <KoShapeBulkActionLock.h>
#include <KoShapeStrokeModel.h>

#include <kis_command_ids.h>

#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>
#include <QWeakPointer>

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

KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter(const QList<KoShape *> &)
{
    qFatal("the production bulk-action path must not run in the stroke command contract");
}

void KoShapeBulkActionLockAdapter::lock()
{
    qFatal("the production bulk-action path must not run in the stroke command contract");
}

void KoShapeBulkActionLockAdapter::unlock()
{
    qFatal("the production bulk-action path must not run in the stroke command contract");
}

KoShapeBulkActionLockAdapter::UpdatesList KoShapeBulkActionLockAdapter::takeFinalUpdatesList()
{
    qFatal("the production bulk-action path must not run in the stroke command contract");
}

KoShapeBulkActionLock::~KoShapeBulkActionLock()
{
    qFatal("the production bulk-action path must not run in the stroke command contract");
}

void KoShapeBulkActionLock::bulkShapesUpdate(const UpdatesList &)
{
    qFatal("the production bulk-action path must not run in the stroke command contract");
}

namespace KoShapeStrokeCommandTesting
{
using StrokeReader = KoShapeStrokeModelSP (*)(const KoShape *shape);
using StrokeBatchApplier = void (*)(const QList<KoShape *> &shapes, const QList<KoShapeStrokeModelSP> &strokes);

void setShapeAccessForTesting(StrokeReader reader, StrokeBatchApplier applier);
void resetShapeAccessForTesting();
} // namespace KoShapeStrokeCommandTesting

namespace
{
struct StrokeToken {
    unsigned char value{0};
};

struct StrokeHandle {
    KoShapeStrokeModelSP value;
    QSharedPointer<int> deletionCount;
};

struct ShapeObservation {
    QString label;
    KoShapeStrokeModelSP stroke;
    int readCount{0};
    QList<const KoShapeStrokeModel *> assignedStrokes;
};

struct BatchObservation {
    QList<KoShape *> shapes;
    QList<const KoShapeStrokeModel *> strokes;
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoShape *, ShapeObservation> shapeObservations;
QList<BatchObservation> batchObservations;

StrokeHandle makeStroke()
{
    auto *storage = new StrokeToken;
    auto *stroke = reinterpret_cast<KoShapeStrokeModel *>(storage);
    QSharedPointer<int> deletionCount = QSharedPointer<int>::create(0);
    KoShapeStrokeModelSP value(stroke, [storage, deletionCount](KoShapeStrokeModel *) {
        ++*deletionCount;
        delete storage;
    });
    return {value, deletionCount};
}

KoShapeStrokeModelSP readStroke(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.readCount;
    operationTrace.append(QStringLiteral("shape-read:%1").arg(observation.label));
    return observation.stroke;
}

void applyStrokeBatch(const QList<KoShape *> &shapes, const QList<KoShapeStrokeModelSP> &strokes)
{
    BatchObservation batch;
    batch.shapes = shapes;
    for (const KoShapeStrokeModelSP &stroke : strokes) {
        batch.strokes.append(stroke.data());
    }
    batchObservations.append(batch);
    operationTrace.append(QStringLiteral("stroke-batch"));

    const int count = qMin(shapes.size(), strokes.size());
    for (int i = 0; i < count; ++i) {
        ShapeObservation &observation = shapeObservations[shapes.at(i)];
        observation.stroke = strokes.at(i);
        observation.assignedStrokes.append(strokes.at(i).data());
    }
}

class ShapeAccessScope
{
public:
    ShapeAccessScope()
    {
        KoShapeStrokeCommandTesting::setShapeAccessForTesting(readStroke, applyStrokeBatch);
    }

    ~ShapeAccessScope()
    {
        KoShapeStrokeCommandTesting::resetShapeAccessForTesting();
    }
};
} // namespace

void TestShapeStrokeCommand::uniformAndSingleConstructorsCapturePreviousStrokes()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    StrokeHandle oldFirst = makeStroke();
    StrokeHandle oldSecond = makeStroke();
    StrokeHandle replacement = makeStroke();
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirst.value, 0, {}}},
        {secondShape, {QStringLiteral("second"), oldSecond.value, 0, {}}},
    };
    KUndo2Command parent;

    operationTrace.clear();
    KoShapeStrokeCommand uniform({firstShape, secondShape}, replacement.value, &parent);
    QCOMPARE(operationTrace, QStringList({QStringLiteral("shape-read:first"), QStringLiteral("shape-read:second")}));
    QCOMPARE(commandParents.value(&uniform), &parent);
    QCOMPARE(shapeObservations[firstShape].readCount, 1);
    QCOMPARE(shapeObservations[secondShape].readCount, 1);

    operationTrace.clear();
    batchObservations.clear();
    uniform.redo();
    QCOMPARE(shapeObservations[firstShape].stroke, replacement.value);
    QCOMPARE(shapeObservations[secondShape].stroke, replacement.value);
    uniform.undo();
    QCOMPARE(shapeObservations[firstShape].stroke, oldFirst.value);
    QCOMPARE(shapeObservations[secondShape].stroke, oldSecond.value);

    operationTrace.clear();
    KoShapeStrokeCommand single(firstShape, replacement.value, &parent);
    QCOMPARE(operationTrace, QStringList({QStringLiteral("shape-read:first")}));
    QCOMPARE(commandParents.value(&single), &parent);
    QCOMPARE(shapeObservations[firstShape].readCount, 2);

    shapeObservations.clear();
}

void TestShapeStrokeCommand::perShapeConstructorPreservesStrokePairing()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    StrokeHandle oldFirst = makeStroke();
    StrokeHandle oldSecond = makeStroke();
    StrokeHandle replacementFirst = makeStroke();
    StrokeHandle replacementSecond = makeStroke();
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirst.value, 0, {}}},
        {secondShape, {QStringLiteral("second"), oldSecond.value, 0, {}}},
    };
    const QList<KoShape *> shapes{firstShape, secondShape};
    const QList<KoShapeStrokeModelSP> strokes{replacementFirst.value, replacementSecond.value};
    KoShapeStrokeCommand command(shapes, strokes);

    batchObservations.clear();
    command.redo();
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.first().shapes, shapes);
    QCOMPARE(batchObservations.first().strokes,
             QList<const KoShapeStrokeModel *>({replacementFirst.value.data(), replacementSecond.value.data()}));
    QCOMPARE(shapeObservations[firstShape].stroke, replacementFirst.value);
    QCOMPARE(shapeObservations[secondShape].stroke, replacementSecond.value);

    command.undo();
    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.last().strokes,
             QList<const KoShapeStrokeModel *>({oldFirst.value.data(), oldSecond.value.data()}));
    QCOMPARE(shapeObservations[firstShape].stroke, oldFirst.value);
    QCOMPARE(shapeObservations[secondShape].stroke, oldSecond.value);

    shapeObservations.clear();
}

void TestShapeStrokeCommand::redoAndUndoDispatchBaseBeforeStrokeBatch()
{
    ShapeAccessScope accessScope;
    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    StrokeHandle oldStroke = makeStroke();
    StrokeHandle newStroke = makeStroke();
    shapeObservations = {{shape, {QStringLiteral("shape"), oldStroke.value, 0, {}}}};
    KoShapeStrokeCommand command(shape, newStroke.value);

    batchObservations.clear();
    operationTrace.clear();
    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo"), QStringLiteral("stroke-batch")}));
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.first().shapes, QList<KoShape *>({shape}));
    QCOMPARE(batchObservations.first().strokes, QList<const KoShapeStrokeModel *>({newStroke.value.data()}));

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo"), QStringLiteral("stroke-batch")}));
    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.last().strokes, QList<const KoShapeStrokeModel *>({oldStroke.value.data()}));

    shapeObservations.clear();
}

void TestShapeStrokeCommand::idAndMergeAcceptOnlyIdenticalShapeSequences()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    StrokeHandle oldFirst = makeStroke();
    StrokeHandle oldSecond = makeStroke();
    StrokeHandle initialFirst = makeStroke();
    StrokeHandle initialSecond = makeStroke();
    StrokeHandle replacementFirst = makeStroke();
    StrokeHandle replacementSecond = makeStroke();
    StrokeHandle reorderedFirst = makeStroke();
    StrokeHandle reorderedSecond = makeStroke();
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirst.value, 0, {}}},
        {secondShape, {QStringLiteral("second"), oldSecond.value, 0, {}}},
    };
    const QList<KoShape *> shapes{firstShape, secondShape};
    KoShapeStrokeCommand command(shapes, {initialFirst.value, initialSecond.value});
    KoShapeStrokeCommand replacement(shapes, {replacementFirst.value, replacementSecond.value});
    KoShapeStrokeCommand reordered({secondShape, firstShape}, {reorderedFirst.value, reorderedSecond.value});
    KUndo2Command unrelated;

    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::ChangeShapeStrokeId));
    QVERIFY(command.mergeWith(&replacement));
    QVERIFY(!command.mergeWith(&reordered));
    QVERIFY(!command.mergeWith(&unrelated));

    batchObservations.clear();
    command.redo();
    QCOMPARE(batchObservations.first().strokes,
             QList<const KoShapeStrokeModel *>({replacementFirst.value.data(), replacementSecond.value.data()}));
    command.undo();
    QCOMPARE(batchObservations.last().strokes,
             QList<const KoShapeStrokeModel *>({oldFirst.value.data(), oldSecond.value.data()}));

    shapeObservations.clear();
}

void TestShapeStrokeCommand::emptyInputsAndSharedOwnershipRemainSafe()
{
    ShapeAccessScope accessScope;
    StrokeHandle unusedReplacement = makeStroke();
    operationTrace.clear();
    batchObservations.clear();
    KoShapeStrokeCommand empty(QList<KoShape *>{}, unusedReplacement.value);

    empty.redo();
    empty.undo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("stroke-batch"),
                          QStringLiteral("base-undo"),
                          QStringLiteral("stroke-batch")}));
    QCOMPARE(batchObservations.size(), 2);
    QVERIFY(batchObservations.first().shapes.isEmpty());
    QVERIFY(batchObservations.first().strokes.isEmpty());

    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    StrokeHandle oldStroke = makeStroke();
    StrokeHandle firstReplacement = makeStroke();
    StrokeHandle secondReplacement = makeStroke();
    const KoShapeStrokeModel *oldStrokePointer = oldStroke.value.data();
    QWeakPointer<KoShapeStrokeModel> oldWeak(oldStroke.value);
    QWeakPointer<KoShapeStrokeModel> firstReplacementWeak(firstReplacement.value);
    QWeakPointer<KoShapeStrokeModel> secondReplacementWeak(secondReplacement.value);
    shapeObservations = {{shape, {QStringLiteral("borrowed"), oldStroke.value, 0, {}}}};
    KUndo2Command parent;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *firstCommand = new KoShapeStrokeCommand(shape, firstReplacement.value, &parent);
    QCOMPARE(commandParents.value(firstCommand), &parent);
    firstCommand->redo();
    firstCommand->undo();

    KUndo2Command *secondCommand = new KoShapeStrokeCommand(shape, secondReplacement.value, &parent);
    QCOMPARE(commandParents.value(secondCommand), &parent);
    secondCommand->redo();

    oldStroke.value.clear();
    firstReplacement.value.clear();
    secondReplacement.value.clear();
    QVERIFY(!oldWeak.isNull());
    QVERIFY(!firstReplacementWeak.isNull());
    QVERIFY(!secondReplacementWeak.isNull());

    delete firstCommand;
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QVERIFY(!oldWeak.isNull());
    QVERIFY(firstReplacementWeak.isNull());
    QVERIFY(!secondReplacementWeak.isNull());

    secondCommand->undo();
    QCOMPARE(shapeObservations[shape].stroke.data(), oldStrokePointer);
    shapeObservations.clear();

    delete secondCommand;
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 2);
    QVERIFY(oldWeak.isNull());
    QVERIFY(secondReplacementWeak.isNull());
    QCOMPARE(*oldStroke.deletionCount, 1);
    QCOMPARE(*firstReplacement.deletionCount, 1);
    QCOMPARE(*secondReplacement.deletionCount, 1);
}

QTEST_GUILESS_MAIN(TestShapeStrokeCommand)
