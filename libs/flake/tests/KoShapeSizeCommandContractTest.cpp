/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeSizeCommand.h>

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

namespace KoShapeSizeCommandTesting
{
using ShapeUpdater = void (*)(const KoShape *shape);
using ShapeSizeSetter = void (*)(KoShape *shape, const QSizeF &size);

void setShapeAccessForTesting(ShapeUpdater updater, ShapeSizeSetter sizeSetter);
void resetShapeAccessForTesting();
} // namespace KoShapeSizeCommandTesting

namespace
{
struct ShapeObservation {
    QString label;
    QSizeF size;
    int updateCount{0};
    QList<QSizeF> assignedSizes;
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

void setShapeSize(KoShape *shape, const QSizeF &size)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.size = size;
    observation.assignedSizes.append(size);
    operationTrace.append(QStringLiteral("shape-set:%1").arg(observation.label));
}

class ShapeAccessScope
{
public:
    ShapeAccessScope()
    {
        KoShapeSizeCommandTesting::setShapeAccessForTesting(updateShape, setShapeSize);
    }

    ~ShapeAccessScope()
    {
        KoShapeSizeCommandTesting::resetShapeAccessForTesting();
    }
};

class ShapeSizeCommandProbe : public KoShapeSizeCommand
{
public:
    ShapeSizeCommandProbe(const QList<KoShape *> &shapes,
                          const QList<QSizeF> &previousSizes,
                          const QList<QSizeF> &newSizes,
                          KUndo2Command *parent,
                          int *destructionCount)
        : KoShapeSizeCommand(shapes, previousSizes, newSizes, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~ShapeSizeCommandProbe() override
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

void KoShape::setSize(const QSizeF &size)
{
    setShapeSize(this, size);
}

class KoShapeSizeCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesSizesAcrossUndoCycles();
    void handlesEmptyShapeList();
    void keepsBorrowedShapesAndVirtualLifetime();
};

void KoShapeSizeCommandContractTest::preservesSizesAcrossUndoCycles()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    ShapeToken unchangedStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    auto *unchangedShape = reinterpret_cast<KoShape *>(&unchangedStorage);
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), QSizeF(101.0, 102.0), 0, {}}},
        {secondShape, {QStringLiteral("second"), QSizeF(201.0, 202.0), 0, {}}},
        {unchangedShape, {QStringLiteral("unchanged"), QSizeF(9.0, 9.0), 0, {}}},
    };
    QList<KoShape *> shapes{firstShape, secondShape, unchangedShape};
    QList<QSizeF> previousSizes{QSizeF(10.5, 20.25), QSizeF(0.0, 0.0), QSizeF(9.0, 9.0)};
    QList<QSizeF> newSizes{QSizeF(30.0, 40.0), QSizeF(5.5, 7.25), QSizeF(9.0, 9.0)};
    const QList<QSizeF> expectedPreviousSizes = previousSizes;
    const QList<QSizeF> expectedNewSizes = newSizes;
    KUndo2Command parent;
    KoShapeSizeCommand command(shapes, previousSizes, newSizes, &parent);

    shapes.clear();
    previousSizes.fill(QSizeF(301.0, 302.0));
    newSizes.fill(QSizeF(401.0, 402.0));

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Resize shapes"));
    QVERIFY(operationTrace.isEmpty());

    const QStringList sizeTrace = {
        QStringLiteral("shape-update:first"),
        QStringLiteral("shape-set:first"),
        QStringLiteral("shape-update:first"),
        QStringLiteral("shape-update:second"),
        QStringLiteral("shape-set:second"),
        QStringLiteral("shape-update:second"),
        QStringLiteral("shape-update:unchanged"),
        QStringLiteral("shape-set:unchanged"),
        QStringLiteral("shape-update:unchanged"),
    };

    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + sizeTrace);
    QCOMPARE(shapeObservations[firstShape].size, expectedNewSizes[0]);
    QCOMPARE(shapeObservations[secondShape].size, expectedNewSizes[1]);
    QCOMPARE(shapeObservations[unchangedShape].size, expectedNewSizes[2]);

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo")}) + sizeTrace);
    QCOMPARE(shapeObservations[firstShape].size, expectedPreviousSizes[0]);
    QCOMPARE(shapeObservations[secondShape].size, expectedPreviousSizes[1]);
    QCOMPARE(shapeObservations[unchangedShape].size, expectedPreviousSizes[2]);

    operationTrace.clear();
    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + sizeTrace);
    QCOMPARE(shapeObservations[firstShape].size, expectedNewSizes[0]);
    QCOMPARE(shapeObservations[secondShape].size, expectedNewSizes[1]);
    QCOMPARE(shapeObservations[unchangedShape].size, expectedNewSizes[2]);
    QCOMPARE(shapeObservations[firstShape].updateCount, 6);
    QCOMPARE(shapeObservations[secondShape].updateCount, 6);
    QCOMPARE(shapeObservations[unchangedShape].updateCount, 6);
    QCOMPARE(shapeObservations[firstShape].assignedSizes,
             QList<QSizeF>({expectedNewSizes[0], expectedPreviousSizes[0], expectedNewSizes[0]}));
    QCOMPARE(shapeObservations[secondShape].assignedSizes,
             QList<QSizeF>({expectedNewSizes[1], expectedPreviousSizes[1], expectedNewSizes[1]}));
    QCOMPARE(shapeObservations[unchangedShape].assignedSizes,
             QList<QSizeF>({expectedNewSizes[2], expectedPreviousSizes[2], expectedNewSizes[2]}));
}

void KoShapeSizeCommandContractTest::handlesEmptyShapeList()
{
    ShapeAccessScope accessScope;
    shapeObservations.clear();
    operationTrace.clear();
    KUndo2Command parent;
    KoShapeSizeCommand command({}, {}, {}, &parent);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Resize shapes"));

    command.redo();
    command.undo();
    command.redo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"), QStringLiteral("base-undo"), QStringLiteral("base-redo")}));
    QVERIFY(shapeObservations.isEmpty());
}

void KoShapeSizeCommandContractTest::keepsBorrowedShapesAndVirtualLifetime()
{
    ShapeAccessScope accessScope;
    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    const QSizeF originalSize(12.0, 13.0);
    shapeObservations = {{shape, {QStringLiteral("borrowed"), originalSize, 0, {}}}};
    operationTrace.clear();
    KUndo2Command parent;
    int destructionCount = 0;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *command =
        new ShapeSizeCommandProbe({shape}, {originalSize}, {QSizeF(14.0, 15.0)}, &parent, &destructionCount);

    delete command;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QCOMPARE(shapeObservations[shape].size, originalSize);
    QCOMPARE(shapeObservations[shape].updateCount, 0);
    QVERIFY(shapeObservations[shape].assignedSizes.isEmpty());
    QVERIFY(operationTrace.isEmpty());
}

QTEST_GUILESS_MAIN(KoShapeSizeCommandContractTest)

#include "KoShapeSizeCommandContractTest.moc"
