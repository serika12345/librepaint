/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeKeepAspectRatioCommand.h>

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

namespace KoShapeKeepAspectRatioCommandTesting
{
using KeepAspectRatioReader = bool (*)(const KoShape *shape);
using KeepAspectRatioWriter = void (*)(KoShape *shape, bool keepAspectRatio);

void setShapeAccessForTesting(KeepAspectRatioReader reader, KeepAspectRatioWriter writer);
void resetShapeAccessForTesting();
} // namespace KoShapeKeepAspectRatioCommandTesting

namespace
{
struct ShapeObservation {
    QString label;
    bool keepAspectRatio{false};
    int readCount{0};
    int writeCount{0};
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoShape *, ShapeObservation> shapeObservations;
QStringList readTrace;

bool readKeepAspectRatio(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.readCount;
    readTrace.append(observation.label);
    return observation.keepAspectRatio;
}

void writeKeepAspectRatio(KoShape *shape, bool keepAspectRatio)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.keepAspectRatio = keepAspectRatio;
    ++observation.writeCount;
    operationTrace.append(QStringLiteral("shape-write:%1:%2").arg(observation.label).arg(keepAspectRatio));
}

class ShapeAccessScope
{
public:
    ShapeAccessScope()
    {
        KoShapeKeepAspectRatioCommandTesting::setShapeAccessForTesting(readKeepAspectRatio, writeKeepAspectRatio);
    }

    ~ShapeAccessScope()
    {
        KoShapeKeepAspectRatioCommandTesting::resetShapeAccessForTesting();
    }
};

class KeepAspectRatioCommandProbe : public KoShapeKeepAspectRatioCommand
{
public:
    KeepAspectRatioCommandProbe(const QList<KoShape *> &shapes,
                                bool newKeepAspectRatio,
                                KUndo2Command *parent,
                                int *destructionCount)
        : KoShapeKeepAspectRatioCommand(shapes, newKeepAspectRatio, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~KeepAspectRatioCommandProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};
} // namespace

bool KoShape::keepAspectRatio() const
{
    return readKeepAspectRatio(this);
}

void KoShape::setKeepAspectRatio(bool keepAspectRatio)
{
    writeKeepAspectRatio(this, keepAspectRatio);
}

class KoShapeKeepAspectRatioCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesPerShapeValues_data();
    void preservesPerShapeValues();
    void handlesEmptyShapeList();
    void keepsBorrowedShapesAndVirtualLifetime();
};

void KoShapeKeepAspectRatioCommandContractTest::preservesPerShapeValues_data()
{
    QTest::addColumn<bool>("firstOriginal");
    QTest::addColumn<bool>("secondOriginal");
    QTest::addColumn<bool>("newValue");

    QTest::newRow("set-true") << false << true << true;
    QTest::newRow("set-false") << true << false << false;
}

void KoShapeKeepAspectRatioCommandContractTest::preservesPerShapeValues()
{
    QFETCH(bool, firstOriginal);
    QFETCH(bool, secondOriginal);
    QFETCH(bool, newValue);

    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoShape *>(&secondStorage);
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), firstOriginal, 0, 0}},
        {secondShape, {QStringLiteral("second"), secondOriginal, 0, 0}},
    };
    readTrace.clear();
    KUndo2Command parent;
    KoShapeKeepAspectRatioCommand command({firstShape, secondShape, firstShape}, newValue, &parent);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Keep Aspect Ratio"));
    QCOMPARE(readTrace, QStringList({QStringLiteral("first"), QStringLiteral("second"), QStringLiteral("first")}));
    QCOMPARE(shapeObservations[firstShape].readCount, 2);
    QCOMPARE(shapeObservations[secondShape].readCount, 1);

    shapeObservations[firstShape].keepAspectRatio = !firstOriginal;
    shapeObservations[secondShape].keepAspectRatio = !secondOriginal;
    operationTrace.clear();

    const QString valueText = newValue ? QStringLiteral("1") : QStringLiteral("0");
    command.redo();
    QCOMPARE(shapeObservations[firstShape].keepAspectRatio, newValue);
    QCOMPARE(shapeObservations[secondShape].keepAspectRatio, newValue);
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("shape-write:first:%1").arg(valueText),
                          QStringLiteral("shape-write:second:%1").arg(valueText),
                          QStringLiteral("shape-write:first:%1").arg(valueText)}));

    operationTrace.clear();
    command.undo();
    QCOMPARE(shapeObservations[firstShape].keepAspectRatio, firstOriginal);
    QCOMPARE(shapeObservations[secondShape].keepAspectRatio, secondOriginal);
    QCOMPARE(operationTrace.first(), QStringLiteral("base-undo"));

    operationTrace.clear();
    command.redo();
    QCOMPARE(shapeObservations[firstShape].keepAspectRatio, newValue);
    QCOMPARE(shapeObservations[secondShape].keepAspectRatio, newValue);
    QCOMPARE(operationTrace.first(), QStringLiteral("base-redo"));
    QCOMPARE(shapeObservations[firstShape].writeCount, 6);
    QCOMPARE(shapeObservations[secondShape].writeCount, 3);
}

void KoShapeKeepAspectRatioCommandContractTest::handlesEmptyShapeList()
{
    ShapeAccessScope accessScope;
    shapeObservations.clear();
    readTrace.clear();
    operationTrace.clear();
    KUndo2Command parent;
    KoShapeKeepAspectRatioCommand command({}, true, &parent);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Keep Aspect Ratio"));
    QVERIFY(readTrace.isEmpty());

    command.redo();
    command.undo();
    command.redo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"), QStringLiteral("base-undo"), QStringLiteral("base-redo")}));
    QVERIFY(shapeObservations.isEmpty());
}

void KoShapeKeepAspectRatioCommandContractTest::keepsBorrowedShapesAndVirtualLifetime()
{
    ShapeAccessScope accessScope;
    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    shapeObservations = {{shape, {QStringLiteral("borrowed"), false, 0, 0}}};
    readTrace.clear();
    KUndo2Command parent;
    int destructionCount = 0;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *command = new KeepAspectRatioCommandProbe({shape}, true, &parent, &destructionCount);

    delete command;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QVERIFY(!shapeObservations[shape].keepAspectRatio);
    QCOMPARE(shapeObservations[shape].readCount, 1);
    QCOMPARE(shapeObservations[shape].writeCount, 0);
}

QTEST_GUILESS_MAIN(KoShapeKeepAspectRatioCommandContractTest)

#include "KoShapeKeepAspectRatioCommandContractTest.moc"
