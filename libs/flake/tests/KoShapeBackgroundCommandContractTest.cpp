/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeBackground.h>
#include <KoShapeBackgroundCommand.h>

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

namespace KoShapeBackgroundCommandTesting
{
using BackgroundReader = QSharedPointer<KoShapeBackground> (*)(const KoShape *shape);
using BackgroundWriter = void (*)(KoShape *shape, QSharedPointer<KoShapeBackground> background);
using ShapeUpdater = void (*)(const KoShape *shape);

void setShapeAccessForTesting(BackgroundReader reader, BackgroundWriter writer, ShapeUpdater updater);
void resetShapeAccessForTesting();
} // namespace KoShapeBackgroundCommandTesting

namespace
{
struct BackgroundToken {
    unsigned char value{0};
};

struct BackgroundHandle {
    QSharedPointer<KoShapeBackground> value;
    QSharedPointer<int> deletionCount;
};

struct ShapeObservation {
    QString label;
    QSharedPointer<KoShapeBackground> background;
    int readCount{0};
    int updateCount{0};
    QList<const KoShapeBackground *> assignedBackgrounds;
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoShape *, ShapeObservation> shapeObservations;

BackgroundHandle makeBackground()
{
    auto *storage = new BackgroundToken;
    auto *background = reinterpret_cast<KoShapeBackground *>(storage);
    QSharedPointer<int> deletionCount = QSharedPointer<int>::create(0);
    QSharedPointer<KoShapeBackground> value(background, [storage, deletionCount](KoShapeBackground *) {
        ++*deletionCount;
        delete storage;
    });
    return {value, deletionCount};
}

QSharedPointer<KoShapeBackground> readBackground(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.readCount;
    operationTrace.append(QStringLiteral("shape-read:%1").arg(observation.label));
    return observation.background;
}

void writeBackground(KoShape *shape, QSharedPointer<KoShapeBackground> background)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.background = background;
    observation.assignedBackgrounds.append(background.data());
    operationTrace.append(QStringLiteral("shape-set:%1").arg(observation.label));
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
        KoShapeBackgroundCommandTesting::setShapeAccessForTesting(readBackground, writeBackground, updateShape);
    }

    ~ShapeAccessScope()
    {
        KoShapeBackgroundCommandTesting::resetShapeAccessForTesting();
    }
};

class ShapeBackgroundCommandProbe : public KoShapeBackgroundCommand
{
public:
    ShapeBackgroundCommandProbe(KoShape *shape,
                                QSharedPointer<KoShapeBackground> background,
                                KUndo2Command *parent,
                                int *destructionCount)
        : KoShapeBackgroundCommand(shape, background, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~ShapeBackgroundCommandProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};
} // namespace

class KoShapeBackgroundCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void retainsUniformBackgroundsAcrossUndoCycles();
    void supportsSingleAndPerShapeBackgrounds();
    void mergesOnlyMatchingShapeSequences();
    void handlesEmptyShapeSequence();
    void keepsBorrowedShapeAndVirtualLifetime();
};

void KoShapeBackgroundCommandContractTest::retainsUniformBackgroundsAcrossUndoCycles()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    BackgroundHandle oldFirst = makeBackground();
    BackgroundHandle oldSecond = makeBackground();
    BackgroundHandle replacement = makeBackground();
    const KoShapeBackground *oldFirstPointer = oldFirst.value.data();
    const KoShapeBackground *oldSecondPointer = oldSecond.value.data();
    const KoShapeBackground *replacementPointer = replacement.value.data();
    QWeakPointer<KoShapeBackground> oldFirstWeak(oldFirst.value);
    QWeakPointer<KoShapeBackground> oldSecondWeak(oldSecond.value);
    QWeakPointer<KoShapeBackground> replacementWeak(replacement.value);
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirst.value, 0, 0, {}}},
        {secondShape, {QStringLiteral("second"), oldSecond.value, 0, 0, {}}},
    };
    QList<KoShape *> shapes{firstShape, secondShape};
    KUndo2Command parent;

    {
        operationTrace.clear();
        KoShapeBackgroundCommand command(shapes, replacement.value, &parent);
        shapes.clear();
        oldFirst.value.clear();
        oldSecond.value.clear();
        replacement.value.clear();

        QCOMPARE(operationTrace,
                 QStringList({QStringLiteral("shape-read:first"), QStringLiteral("shape-read:second")}));
        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(command.actionText(), QStringLiteral("Set background"));
        QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::ChangeShapeBackgroundId));

        const QStringList assignmentTrace = {
            QStringLiteral("shape-set:first"),
            QStringLiteral("shape-update:first"),
            QStringLiteral("shape-set:second"),
            QStringLiteral("shape-update:second"),
        };

        operationTrace.clear();
        command.redo();
        QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + assignmentTrace);
        QCOMPARE(shapeObservations[firstShape].background.data(), replacementPointer);
        QCOMPARE(shapeObservations[secondShape].background.data(), replacementPointer);
        QVERIFY(!oldFirstWeak.isNull());
        QVERIFY(!oldSecondWeak.isNull());

        operationTrace.clear();
        command.undo();
        QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo")}) + assignmentTrace);
        QCOMPARE(shapeObservations[firstShape].background.data(), oldFirstPointer);
        QCOMPARE(shapeObservations[secondShape].background.data(), oldSecondPointer);
        QVERIFY(!replacementWeak.isNull());

        operationTrace.clear();
        command.redo();
        QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + assignmentTrace);
        QCOMPARE(shapeObservations[firstShape].assignedBackgrounds,
                 QList<const KoShapeBackground *>({replacementPointer, oldFirstPointer, replacementPointer}));
        QCOMPARE(shapeObservations[secondShape].assignedBackgrounds,
                 QList<const KoShapeBackground *>({replacementPointer, oldSecondPointer, replacementPointer}));
        QCOMPARE(shapeObservations[firstShape].updateCount, 3);
        QCOMPARE(shapeObservations[secondShape].updateCount, 3);
    }

    QVERIFY(oldFirstWeak.isNull());
    QVERIFY(oldSecondWeak.isNull());
    QVERIFY(!replacementWeak.isNull());
    QCOMPARE(*oldFirst.deletionCount, 1);
    QCOMPARE(*oldSecond.deletionCount, 1);

    shapeObservations.clear();
    QVERIFY(replacementWeak.isNull());
    QCOMPARE(*replacement.deletionCount, 1);
}

void KoShapeBackgroundCommandContractTest::supportsSingleAndPerShapeBackgrounds()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    BackgroundHandle oldFirst = makeBackground();
    BackgroundHandle oldSecond = makeBackground();
    BackgroundHandle singleReplacement = makeBackground();
    BackgroundHandle firstReplacement = makeBackground();
    BackgroundHandle secondReplacement = makeBackground();
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirst.value, 0, 0, {}}},
        {secondShape, {QStringLiteral("second"), oldSecond.value, 0, 0, {}}},
    };
    KUndo2Command parent;

    KoShapeBackgroundCommand singleShapeCommand(firstShape, singleReplacement.value, &parent);
    operationTrace.clear();
    singleShapeCommand.redo();
    QCOMPARE(shapeObservations[firstShape].background, singleReplacement.value);
    singleShapeCommand.undo();
    QCOMPARE(shapeObservations[firstShape].background, oldFirst.value);

    QList<KoShape *> shapes{firstShape, secondShape};
    QList<QSharedPointer<KoShapeBackground>> backgrounds{firstReplacement.value, secondReplacement.value};
    KoShapeBackgroundCommand perShapeCommand(shapes, backgrounds, &parent);
    shapes.clear();
    backgrounds.clear();

    operationTrace.clear();
    perShapeCommand.redo();
    QCOMPARE(shapeObservations[firstShape].background, firstReplacement.value);
    QCOMPARE(shapeObservations[secondShape].background, secondReplacement.value);
    perShapeCommand.undo();
    QCOMPARE(shapeObservations[firstShape].background, oldFirst.value);
    QCOMPARE(shapeObservations[secondShape].background, oldSecond.value);

    shapeObservations.clear();
}

void KoShapeBackgroundCommandContractTest::mergesOnlyMatchingShapeSequences()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    BackgroundHandle oldFirst = makeBackground();
    BackgroundHandle oldSecond = makeBackground();
    BackgroundHandle initialFirst = makeBackground();
    BackgroundHandle initialSecond = makeBackground();
    BackgroundHandle replacementFirst = makeBackground();
    BackgroundHandle replacementSecond = makeBackground();
    BackgroundHandle reorderedFirst = makeBackground();
    BackgroundHandle reorderedSecond = makeBackground();
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirst.value, 0, 0, {}}},
        {secondShape, {QStringLiteral("second"), oldSecond.value, 0, 0, {}}},
    };
    const QList<KoShape *> shapes{firstShape, secondShape};
    QList<QSharedPointer<KoShapeBackground>> initialValues{initialFirst.value, initialSecond.value};
    KoShapeBackgroundCommand command(shapes, initialValues);
    initialValues.clear();
    initialFirst.value.clear();
    initialSecond.value.clear();
    KoShapeBackgroundCommand replacement(
        shapes,
        QList<QSharedPointer<KoShapeBackground>>({replacementFirst.value, replacementSecond.value}));
    KoShapeBackgroundCommand reordered(
        {secondShape, firstShape},
        QList<QSharedPointer<KoShapeBackground>>({reorderedFirst.value, reorderedSecond.value}));
    KUndo2Command unrelated;

    QVERIFY(command.mergeWith(&replacement));
    QCOMPARE(*initialFirst.deletionCount, 1);
    QCOMPARE(*initialSecond.deletionCount, 1);
    QVERIFY(!command.mergeWith(&reordered));
    QVERIFY(!command.mergeWith(&unrelated));

    operationTrace.clear();
    command.redo();
    QCOMPARE(shapeObservations[firstShape].background, replacementFirst.value);
    QCOMPARE(shapeObservations[secondShape].background, replacementSecond.value);
    command.undo();
    QCOMPARE(shapeObservations[firstShape].background, oldFirst.value);
    QCOMPARE(shapeObservations[secondShape].background, oldSecond.value);

    shapeObservations.clear();
}

void KoShapeBackgroundCommandContractTest::handlesEmptyShapeSequence()
{
    ShapeAccessScope accessScope;
    shapeObservations.clear();
    operationTrace.clear();
    BackgroundHandle background = makeBackground();
    KUndo2Command parent;
    KoShapeBackgroundCommand command(QList<KoShape *>{}, background.value, &parent);

    command.redo();
    command.undo();
    command.redo();

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"), QStringLiteral("base-undo"), QStringLiteral("base-redo")}));
    QVERIFY(shapeObservations.isEmpty());
}

void KoShapeBackgroundCommandContractTest::keepsBorrowedShapeAndVirtualLifetime()
{
    ShapeAccessScope accessScope;
    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    BackgroundHandle oldBackground = makeBackground();
    BackgroundHandle replacement = makeBackground();
    shapeObservations = {{shape, {QStringLiteral("borrowed"), oldBackground.value, 0, 0, {}}}};
    KUndo2Command parent;
    int destructionCount = 0;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *command = new ShapeBackgroundCommandProbe(shape, replacement.value, &parent, &destructionCount);
    operationTrace.clear();

    delete command;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QCOMPARE(shapeObservations[shape].background, oldBackground.value);
    QCOMPARE(shapeObservations[shape].updateCount, 0);
    QVERIFY(shapeObservations[shape].assignedBackgrounds.isEmpty());
    QVERIFY(operationTrace.isEmpty());

    shapeObservations.clear();
}

QTEST_GUILESS_MAIN(KoShapeBackgroundCommandContractTest)

#include "KoShapeBackgroundCommandContractTest.moc"
