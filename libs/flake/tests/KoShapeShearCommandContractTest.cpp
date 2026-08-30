/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeShearCommand.h>

#include <QHash>
#include <QPointF>
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

namespace KoShapeShearCommandTesting
{
using ShapeUpdater = void (*)(const KoShape *shape);
using ShapeShearer = void (*)(KoShape *shape, qreal shearX, qreal shearY);

void setShapeAccessForTesting(ShapeUpdater updater, ShapeShearer shearer);
void resetShapeAccessForTesting();
} // namespace KoShapeShearCommandTesting

namespace
{
struct ShapeObservation {
    QString label;
    int updateCount{0};
    QList<QPointF> assignedShears;
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoShape *, ShapeObservation> shapeObservations;

void updateShape(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.updateCount;
    operationTrace.append(QStringLiteral("shape-update:%1").arg(observation.label));
}

void shearShape(KoShape *shape, qreal shearX, qreal shearY)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.assignedShears.append(QPointF(shearX, shearY));
    operationTrace.append(QStringLiteral("shape-shear:%1").arg(observation.label));
}

class ShapeAccessScope
{
public:
    ShapeAccessScope()
    {
        KoShapeShearCommandTesting::setShapeAccessForTesting(updateShape, shearShape);
    }

    ~ShapeAccessScope()
    {
        KoShapeShearCommandTesting::resetShapeAccessForTesting();
    }
};

class ShapeShearCommandProbe : public KoShapeShearCommand
{
public:
    ShapeShearCommandProbe(const QList<KoShape *> &shapes,
                           const QList<qreal> &previousShearXs,
                           const QList<qreal> &previousShearYs,
                           const QList<qreal> &newShearXs,
                           const QList<qreal> &newShearYs,
                           KUndo2Command *parent,
                           int *destructionCount)
        : KoShapeShearCommand(shapes, previousShearXs, previousShearYs, newShearXs, newShearYs, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~ShapeShearCommandProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};
} // namespace

void KoShape::update() const
{
    updateShape(this);
}

void KoShape::shear(qreal shearX, qreal shearY)
{
    shearShape(this, shearX, shearY);
}

class KoShapeShearCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesShearArgumentsAcrossUndoCycles();
    void handlesEmptyShapeList();
    void keepsBorrowedShapesAndVirtualLifetime();
};

void KoShapeShearCommandContractTest::preservesShearArgumentsAcrossUndoCycles()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken zeroStorage;
    ShapeToken unchangedStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *zeroShape = reinterpret_cast<KoShape *>(&zeroStorage);
    auto *unchangedShape = reinterpret_cast<KoShape *>(&unchangedStorage);
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), 0, {}}},
        {zeroShape, {QStringLiteral("zero"), 0, {}}},
        {unchangedShape, {QStringLiteral("unchanged"), 0, {}}},
    };
    QList<KoShape *> shapes{firstShape, zeroShape, unchangedShape};
    QList<qreal> previousShearXs{-0.5, 0.0, 0.125};
    QList<qreal> previousShearYs{0.25, 0.0, -0.5};
    QList<qreal> newShearXs{0.75, 0.0, 0.125};
    QList<qreal> newShearYs{-1.5, 0.0, -0.5};
    const QList<QPointF> expectedPreviousShears{
        QPointF(previousShearXs[0], previousShearYs[0]),
        QPointF(previousShearXs[1], previousShearYs[1]),
        QPointF(previousShearXs[2], previousShearYs[2]),
    };
    const QList<QPointF> expectedNewShears{
        QPointF(newShearXs[0], newShearYs[0]),
        QPointF(newShearXs[1], newShearYs[1]),
        QPointF(newShearXs[2], newShearYs[2]),
    };
    KUndo2Command parent;
    KoShapeShearCommand command(shapes, previousShearXs, previousShearYs, newShearXs, newShearYs, &parent);

    shapes.clear();
    previousShearXs.fill(2.0);
    previousShearYs.fill(3.0);
    newShearXs.fill(4.0);
    newShearYs.fill(5.0);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Shear shapes"));
    QVERIFY(operationTrace.isEmpty());

    const QStringList shearTrace = {
        QStringLiteral("shape-update:first"),
        QStringLiteral("shape-shear:first"),
        QStringLiteral("shape-update:first"),
        QStringLiteral("shape-update:zero"),
        QStringLiteral("shape-shear:zero"),
        QStringLiteral("shape-update:zero"),
        QStringLiteral("shape-update:unchanged"),
        QStringLiteral("shape-shear:unchanged"),
        QStringLiteral("shape-update:unchanged"),
    };

    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + shearTrace);
    QCOMPARE(shapeObservations[firstShape].assignedShears.last(), expectedNewShears[0]);
    QCOMPARE(shapeObservations[zeroShape].assignedShears.last(), expectedNewShears[1]);
    QCOMPARE(shapeObservations[unchangedShape].assignedShears.last(), expectedNewShears[2]);

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo")}) + shearTrace);
    QCOMPARE(shapeObservations[firstShape].assignedShears.last(), expectedPreviousShears[0]);
    QCOMPARE(shapeObservations[zeroShape].assignedShears.last(), expectedPreviousShears[1]);
    QCOMPARE(shapeObservations[unchangedShape].assignedShears.last(), expectedPreviousShears[2]);

    operationTrace.clear();
    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + shearTrace);
    QCOMPARE(shapeObservations[firstShape].assignedShears,
             QList<QPointF>({expectedNewShears[0], expectedPreviousShears[0], expectedNewShears[0]}));
    QCOMPARE(shapeObservations[zeroShape].assignedShears,
             QList<QPointF>({expectedNewShears[1], expectedPreviousShears[1], expectedNewShears[1]}));
    QCOMPARE(shapeObservations[unchangedShape].assignedShears,
             QList<QPointF>({expectedNewShears[2], expectedPreviousShears[2], expectedNewShears[2]}));
    QCOMPARE(shapeObservations[firstShape].updateCount, 6);
    QCOMPARE(shapeObservations[zeroShape].updateCount, 6);
    QCOMPARE(shapeObservations[unchangedShape].updateCount, 6);
}

void KoShapeShearCommandContractTest::handlesEmptyShapeList()
{
    ShapeAccessScope accessScope;
    shapeObservations.clear();
    operationTrace.clear();
    KUndo2Command parent;
    KoShapeShearCommand command({}, {}, {}, {}, {}, &parent);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Shear shapes"));

    command.redo();
    command.undo();
    command.redo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"), QStringLiteral("base-undo"), QStringLiteral("base-redo")}));
    QVERIFY(shapeObservations.isEmpty());
}

void KoShapeShearCommandContractTest::keepsBorrowedShapesAndVirtualLifetime()
{
    ShapeAccessScope accessScope;
    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    shapeObservations = {{shape, {QStringLiteral("borrowed"), 0, {}}}};
    operationTrace.clear();
    KUndo2Command parent;
    int destructionCount = 0;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *command =
        new ShapeShearCommandProbe({shape}, {0.0}, {0.0}, {0.25}, {-0.25}, &parent, &destructionCount);

    delete command;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QCOMPARE(shapeObservations[shape].updateCount, 0);
    QVERIFY(shapeObservations[shape].assignedShears.isEmpty());
    QVERIFY(operationTrace.isEmpty());
}

QTEST_GUILESS_MAIN(KoShapeShearCommandContractTest)

#include "KoShapeShearCommandContractTest.moc"
