/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapePaintOrderCommand.h>

#include <kis_command_ids.h>

#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace KoShapePaintOrderCommandTesting
{
using PaintOrderReader = QVector<KoShape::PaintOrder> (*)(const KoShape *shape);
using PaintOrderWriter = void (*)(KoShape *shape, KoShape::PaintOrder first, KoShape::PaintOrder second);
using InheritPaintOrderReader = bool (*)(const KoShape *shape);
using InheritPaintOrderWriter = void (*)(KoShape *shape, bool inheritPaintOrder);
using ShapeUpdater = void (*)(const KoShape *shape);

void setShapeAccessForTesting(PaintOrderReader paintOrderReader,
                              PaintOrderWriter paintOrderWriter,
                              InheritPaintOrderReader inheritPaintOrderReader,
                              InheritPaintOrderWriter inheritPaintOrderWriter,
                              ShapeUpdater updater);
void resetShapeAccessForTesting();
} // namespace KoShapePaintOrderCommandTesting

namespace
{
using PaintOrderPair = QPair<KoShape::PaintOrder, KoShape::PaintOrder>;

QHash<const KUndo2Command *, KUndo2MagicString> commandTexts;
QHash<const KUndo2Command *, KUndo2Command *> commandParents;
QStringList operationTrace;
int baseDestructionCount = 0;

struct ShapeObservation {
    QString label;
    QVector<KoShape::PaintOrder> paintOrder;
    bool inheritPaintOrder{false};
    int paintOrderReadCount{0};
    int inheritReadCount{0};
    QList<PaintOrderPair> assignedOrders;
    QList<bool> assignedInheritance;
    int updateCount{0};
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoShape *, ShapeObservation> shapeObservations;

KoShape *shapePointer(ShapeToken &token)
{
    return reinterpret_cast<KoShape *>(&token);
}

ShapeObservation
makeShapeObservation(const QString &label, const QVector<KoShape::PaintOrder> &paintOrder, bool inheritPaintOrder)
{
    ShapeObservation observation;
    observation.label = label;
    observation.paintOrder = paintOrder;
    observation.inheritPaintOrder = inheritPaintOrder;
    return observation;
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

bool KoShape::inheritPaintOrder() const
{
    qFatal("Default KoShape::inheritPaintOrder access reached the contract test");
}

void KoShape::setInheritPaintOrder(bool)
{
    qFatal("Default KoShape::setInheritPaintOrder access reached the contract test");
}

namespace
{
QVector<KoShape::PaintOrder> readPaintOrder(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.paintOrderReadCount;
    operationTrace.append(QStringLiteral("shape-read-order:%1").arg(observation.label));
    return observation.paintOrder;
}

bool readInheritPaintOrder(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.inheritReadCount;
    operationTrace.append(QStringLiteral("shape-read-inherit:%1").arg(observation.label));
    return observation.inheritPaintOrder;
}

void writePaintOrder(KoShape *shape, KoShape::PaintOrder first, KoShape::PaintOrder second)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.assignedOrders.append(qMakePair(first, second));
    operationTrace.append(QStringLiteral("shape-set-order:%1").arg(observation.label));
}

void writeInheritPaintOrder(KoShape *shape, bool value)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.assignedInheritance.append(value);
    operationTrace.append(QStringLiteral("shape-set-inherit:%1").arg(observation.label));
}

void updateShape(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.updateCount;
    operationTrace.append(QStringLiteral("shape-update:%1").arg(observation.label));
}

class ShapeAccessScope
{
public:
    ShapeAccessScope()
    {
        KoShapePaintOrderCommandTesting::setShapeAccessForTesting(readPaintOrder,
                                                                  writePaintOrder,
                                                                  readInheritPaintOrder,
                                                                  writeInheritPaintOrder,
                                                                  updateShape);
    }

    ~ShapeAccessScope()
    {
        KoShapePaintOrderCommandTesting::resetShapeAccessForTesting();
    }
};

class ShapePaintOrderCommandProbe final : public KoShapePaintOrderCommand
{
public:
    ShapePaintOrderCommandProbe(const QList<KoShape *> &shapes,
                                KoShape::PaintOrder first,
                                KoShape::PaintOrder second,
                                KUndo2Command *parent,
                                int *destructionCount)
        : KoShapePaintOrderCommand(shapes, first, second, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~ShapePaintOrderCommandProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};
} // namespace

class KoShapePaintOrderCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void capturesOriginalOrderInheritanceParentAndBorrowedLifetime();
    void redoAndUndoApplyOrdersInShapeSequence();
    void mergeAdoptsOnlyMatchingShapeSequence();
    void emptySequenceIsStableAndReportsCommandId();
};

void KoShapePaintOrderCommandContractTest::init()
{
    commandTexts.clear();
    commandParents.clear();
    shapeObservations.clear();
    operationTrace.clear();
    baseDestructionCount = 0;
}

void KoShapePaintOrderCommandContractTest::capturesOriginalOrderInheritanceParentAndBorrowedLifetime()
{
    ShapeAccessScope shapeAccessScope;
    ShapeToken firstStorage;
    firstStorage.value = 17;
    ShapeToken secondStorage;
    secondStorage.value = 23;
    KoShape *const firstShape = shapePointer(firstStorage);
    KoShape *const secondShape = shapePointer(secondStorage);
    shapeObservations = {
        {firstShape,
         makeShapeObservation(QStringLiteral("first"), {KoShape::Fill, KoShape::Stroke, KoShape::Markers}, true)},
        {secondShape,
         makeShapeObservation(QStringLiteral("second"), {KoShape::Markers, KoShape::Fill, KoShape::Stroke}, false)},
    };
    KUndo2Command parent;
    int derivedDestructionCount = 0;
    const int destructionCountBeforeCommand = baseDestructionCount;
    KUndo2Command *command = new ShapePaintOrderCommandProbe({firstShape, secondShape},
                                                             KoShape::Stroke,
                                                             KoShape::Markers,
                                                             &parent,
                                                             &derivedDestructionCount);

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("shape-read-order:first"),
                          QStringLiteral("shape-read-order:first"),
                          QStringLiteral("shape-read-inherit:first"),
                          QStringLiteral("shape-read-order:second"),
                          QStringLiteral("shape-read-order:second"),
                          QStringLiteral("shape-read-inherit:second")}));
    QCOMPARE(commandParents.value(command), &parent);
    QCOMPARE(shapeObservations[firstShape].paintOrderReadCount, 2);
    QCOMPARE(shapeObservations[firstShape].inheritReadCount, 1);
    QCOMPARE(shapeObservations[secondShape].paintOrderReadCount, 2);
    QCOMPARE(shapeObservations[secondShape].inheritReadCount, 1);
    QVERIFY(!commandTexts.value(command).isEmpty());

    operationTrace.clear();
    delete command;

    QCOMPARE(derivedDestructionCount, 1);
    QCOMPARE(baseDestructionCount, destructionCountBeforeCommand + 1);
    QCOMPARE(firstStorage.value, 17);
    QCOMPARE(secondStorage.value, 23);
    QVERIFY(operationTrace.isEmpty());
}

void KoShapePaintOrderCommandContractTest::redoAndUndoApplyOrdersInShapeSequence()
{
    ShapeAccessScope shapeAccessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *const firstShape = shapePointer(firstStorage);
    KoShape *const secondShape = shapePointer(secondStorage);
    shapeObservations = {
        {firstShape,
         makeShapeObservation(QStringLiteral("first"), {KoShape::Fill, KoShape::Stroke, KoShape::Markers}, true)},
        {secondShape,
         makeShapeObservation(QStringLiteral("second"), {KoShape::Markers, KoShape::Fill, KoShape::Stroke}, false)},
    };
    KoShapePaintOrderCommand command({firstShape, secondShape}, KoShape::Stroke, KoShape::Markers);

    operationTrace.clear();
    command.redo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("shape-set-order:first"),
                          QStringLiteral("shape-update:first"),
                          QStringLiteral("shape-set-order:second"),
                          QStringLiteral("shape-update:second")}));
    QCOMPARE(shapeObservations[firstShape].assignedOrders,
             QList<PaintOrderPair>({qMakePair(KoShape::Stroke, KoShape::Markers)}));
    QCOMPARE(shapeObservations[secondShape].assignedOrders,
             QList<PaintOrderPair>({qMakePair(KoShape::Stroke, KoShape::Markers)}));

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-undo"),
                          QStringLiteral("shape-set-order:first"),
                          QStringLiteral("shape-set-inherit:first"),
                          QStringLiteral("shape-update:first"),
                          QStringLiteral("shape-set-order:second"),
                          QStringLiteral("shape-set-inherit:second"),
                          QStringLiteral("shape-update:second")}));
    QCOMPARE(shapeObservations[firstShape].assignedOrders,
             QList<PaintOrderPair>(
                 {qMakePair(KoShape::Stroke, KoShape::Markers), qMakePair(KoShape::Fill, KoShape::Stroke)}));
    QCOMPARE(shapeObservations[secondShape].assignedOrders,
             QList<PaintOrderPair>(
                 {qMakePair(KoShape::Stroke, KoShape::Markers), qMakePair(KoShape::Markers, KoShape::Fill)}));
    QCOMPARE(shapeObservations[firstShape].assignedInheritance, QList<bool>({true}));
    QCOMPARE(shapeObservations[secondShape].assignedInheritance, QList<bool>({false}));
    QCOMPARE(shapeObservations[firstShape].updateCount, 2);
    QCOMPARE(shapeObservations[secondShape].updateCount, 2);
}

void KoShapePaintOrderCommandContractTest::mergeAdoptsOnlyMatchingShapeSequence()
{
    ShapeAccessScope shapeAccessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    KoShape *const firstShape = shapePointer(firstStorage);
    KoShape *const secondShape = shapePointer(secondStorage);
    shapeObservations = {
        {firstShape,
         makeShapeObservation(QStringLiteral("first"), {KoShape::Fill, KoShape::Stroke, KoShape::Markers}, false)},
        {secondShape,
         makeShapeObservation(QStringLiteral("second"), {KoShape::Stroke, KoShape::Markers, KoShape::Fill}, true)},
    };
    KoShapePaintOrderCommand command({firstShape, secondShape}, KoShape::Stroke, KoShape::Markers);
    KoShapePaintOrderCommand replacement({firstShape, secondShape}, KoShape::Markers, KoShape::Fill);
    KoShapePaintOrderCommand reordered({secondShape, firstShape}, KoShape::Markers, KoShape::Stroke);
    KUndo2Command unrelated;

    QVERIFY(command.mergeWith(&replacement));
    QVERIFY(!command.mergeWith(&reordered));
    QVERIFY(!command.mergeWith(&unrelated));

    operationTrace.clear();
    command.redo();
    QCOMPARE(shapeObservations[firstShape].assignedOrders,
             QList<PaintOrderPair>({qMakePair(KoShape::Markers, KoShape::Fill)}));
    QCOMPARE(shapeObservations[secondShape].assignedOrders,
             QList<PaintOrderPair>({qMakePair(KoShape::Markers, KoShape::Fill)}));

    command.undo();
    QCOMPARE(
        shapeObservations[firstShape].assignedOrders,
        QList<PaintOrderPair>({qMakePair(KoShape::Markers, KoShape::Fill), qMakePair(KoShape::Fill, KoShape::Stroke)}));
    QCOMPARE(shapeObservations[secondShape].assignedOrders,
             QList<PaintOrderPair>(
                 {qMakePair(KoShape::Markers, KoShape::Fill), qMakePair(KoShape::Stroke, KoShape::Markers)}));
}

void KoShapePaintOrderCommandContractTest::emptySequenceIsStableAndReportsCommandId()
{
    ShapeAccessScope shapeAccessScope;
    KUndo2Command parent;
    KoShapePaintOrderCommand command({}, KoShape::Markers, KoShape::Stroke, &parent);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::ChangePaintOrderCommand));
    QVERIFY(shapeObservations.isEmpty());

    operationTrace.clear();
    command.redo();
    command.undo();
    command.redo();

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"), QStringLiteral("base-undo"), QStringLiteral("base-redo")}));
    QVERIFY(shapeObservations.isEmpty());
}

QTEST_GUILESS_MAIN(KoShapePaintOrderCommandContractTest)

#include "KoShapePaintOrderCommandContractTest.moc"
