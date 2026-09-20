/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeTransparencyCommand.h>

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

namespace KoShapeTransparencyCommandTesting
{
using ShapeUpdater = void (*)(const KoShape *shape);

void setShapeUpdaterForTesting(ShapeUpdater updater);
void resetShapeUpdaterForTesting();
} // namespace KoShapeTransparencyCommandTesting

namespace
{
struct ShapeObservation {
    QString label;
    qreal transparency{0.0};
    int readCount{0};
    int updateCount{0};
    QList<qreal> assignedTransparencies;
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoShape *, ShapeObservation> shapeObservations;

qreal readTransparency(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.readCount;
    operationTrace.append(QStringLiteral("shape-read:%1").arg(observation.label));
    return observation.transparency;
}

void writeTransparency(KoShape *shape, qreal transparency)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.transparency = transparency;
    observation.assignedTransparencies.append(transparency);
    operationTrace.append(QStringLiteral("shape-set:%1").arg(observation.label));
}

void updateShape(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.updateCount;
    operationTrace.append(QStringLiteral("shape-update:%1").arg(observation.label));
}

class ShapeUpdaterScope
{
public:
    ShapeUpdaterScope()
    {
        KoShapeTransparencyCommandTesting::setShapeUpdaterForTesting(updateShape);
    }

    ~ShapeUpdaterScope()
    {
        KoShapeTransparencyCommandTesting::resetShapeUpdaterForTesting();
    }
};

class ShapeTransparencyCommandProbe : public KoShapeTransparencyCommand
{
public:
    ShapeTransparencyCommandProbe(KoShape *shape, qreal transparency, KUndo2Command *parent, int *destructionCount)
        : KoShapeTransparencyCommand(shape, transparency, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~ShapeTransparencyCommandProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};
} // namespace

qreal KoShape::transparency(bool recursive) const
{
    Q_UNUSED(recursive);
    return readTransparency(this);
}

void KoShape::setTransparency(qreal transparency)
{
    writeTransparency(this, transparency);
}

void KoShape::update() const
{
    updateShape(this);
}

class KoShapeTransparencyCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void capturesUniformTransparencyAcrossUndoCycles();
    void supportsSingleAndPerShapeValues();
    void mergesOnlyMatchingShapeSequences();
    void handlesEmptyShapeSequence();
    void keepsBorrowedShapeAndVirtualLifetime();
};

void KoShapeTransparencyCommandContractTest::capturesUniformTransparencyAcrossUndoCycles()
{
    ShapeUpdaterScope updaterScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), 0.125, 0, 0, {}}},
        {secondShape, {QStringLiteral("second"), 0.75, 0, 0, {}}},
    };
    operationTrace.clear();
    QList<KoShape *> shapes{firstShape, secondShape};
    KUndo2Command parent;
    KoShapeTransparencyCommand command(shapes, 0.5, &parent);
    shapes.clear();

    QCOMPARE(operationTrace, QStringList({QStringLiteral("shape-read:first"), QStringLiteral("shape-read:second")}));
    QCOMPARE(shapeObservations[firstShape].readCount, 1);
    QCOMPARE(shapeObservations[secondShape].readCount, 1);
    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Set opacity"));
    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::ChangeShapeTransparencyId));

    const QStringList assignmentTrace = {
        QStringLiteral("shape-set:first"),
        QStringLiteral("shape-update:first"),
        QStringLiteral("shape-set:second"),
        QStringLiteral("shape-update:second"),
    };

    operationTrace.clear();
    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + assignmentTrace);
    QCOMPARE(shapeObservations[firstShape].transparency, 0.5);
    QCOMPARE(shapeObservations[secondShape].transparency, 0.5);

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo")}) + assignmentTrace);
    QCOMPARE(shapeObservations[firstShape].transparency, 0.125);
    QCOMPARE(shapeObservations[secondShape].transparency, 0.75);

    operationTrace.clear();
    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + assignmentTrace);
    QCOMPARE(shapeObservations[firstShape].assignedTransparencies, QList<qreal>({0.5, 0.125, 0.5}));
    QCOMPARE(shapeObservations[secondShape].assignedTransparencies, QList<qreal>({0.5, 0.75, 0.5}));
    QCOMPARE(shapeObservations[firstShape].updateCount, 3);
    QCOMPARE(shapeObservations[secondShape].updateCount, 3);
}

void KoShapeTransparencyCommandContractTest::supportsSingleAndPerShapeValues()
{
    ShapeUpdaterScope updaterScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), 0.2, 0, 0, {}}},
        {secondShape, {QStringLiteral("second"), 0.8, 0, 0, {}}},
    };
    KUndo2Command parent;

    KoShapeTransparencyCommand singleShapeCommand(firstShape, 0.6, &parent);
    operationTrace.clear();
    singleShapeCommand.redo();
    QCOMPARE(shapeObservations[firstShape].transparency, 0.6);
    singleShapeCommand.undo();
    QCOMPARE(shapeObservations[firstShape].transparency, 0.2);

    QList<KoShape *> shapes{firstShape, secondShape};
    QList<qreal> transparencies{0.1, 0.9};
    KoShapeTransparencyCommand perShapeCommand(shapes, transparencies, &parent);
    shapes.clear();
    transparencies.fill(0.4);

    operationTrace.clear();
    perShapeCommand.redo();
    QCOMPARE(shapeObservations[firstShape].transparency, 0.1);
    QCOMPARE(shapeObservations[secondShape].transparency, 0.9);
    perShapeCommand.undo();
    QCOMPARE(shapeObservations[firstShape].transparency, 0.2);
    QCOMPARE(shapeObservations[secondShape].transparency, 0.8);
}

void KoShapeTransparencyCommandContractTest::mergesOnlyMatchingShapeSequences()
{
    ShapeUpdaterScope updaterScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), 0.25, 0, 0, {}}},
        {secondShape, {QStringLiteral("second"), 0.75, 0, 0, {}}},
    };
    const QList<KoShape *> shapes{firstShape, secondShape};
    KoShapeTransparencyCommand command(shapes, QList<qreal>({0.3, 0.7}));
    KoShapeTransparencyCommand replacement(shapes, QList<qreal>({0.4, 0.6}));
    KoShapeTransparencyCommand reordered({secondShape, firstShape}, QList<qreal>({0.8, 0.2}));
    KUndo2Command unrelated;

    QVERIFY(command.mergeWith(&replacement));
    QVERIFY(!command.mergeWith(&reordered));
    QVERIFY(!command.mergeWith(&unrelated));

    operationTrace.clear();
    command.redo();
    QCOMPARE(shapeObservations[firstShape].transparency, 0.4);
    QCOMPARE(shapeObservations[secondShape].transparency, 0.6);
    command.undo();
    QCOMPARE(shapeObservations[firstShape].transparency, 0.25);
    QCOMPARE(shapeObservations[secondShape].transparency, 0.75);
}

void KoShapeTransparencyCommandContractTest::handlesEmptyShapeSequence()
{
    ShapeUpdaterScope updaterScope;
    shapeObservations.clear();
    operationTrace.clear();
    KUndo2Command parent;
    KoShapeTransparencyCommand command(QList<KoShape *>{}, 0.5, &parent);

    command.redo();
    command.undo();
    command.redo();

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"), QStringLiteral("base-undo"), QStringLiteral("base-redo")}));
    QVERIFY(shapeObservations.isEmpty());
}

void KoShapeTransparencyCommandContractTest::keepsBorrowedShapeAndVirtualLifetime()
{
    ShapeUpdaterScope updaterScope;
    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    shapeObservations = {{shape, {QStringLiteral("borrowed"), 0.35, 0, 0, {}}}};
    KUndo2Command parent;
    int destructionCount = 0;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *command = new ShapeTransparencyCommandProbe(shape, 0.65, &parent, &destructionCount);
    operationTrace.clear();

    delete command;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QCOMPARE(shapeObservations[shape].transparency, 0.35);
    QCOMPARE(shapeObservations[shape].updateCount, 0);
    QVERIFY(shapeObservations[shape].assignedTransparencies.isEmpty());
    QVERIFY(operationTrace.isEmpty());
}

QTEST_GUILESS_MAIN(KoShapeTransparencyCommandContractTest)

#include "KoShapeTransparencyCommandContractTest.moc"
