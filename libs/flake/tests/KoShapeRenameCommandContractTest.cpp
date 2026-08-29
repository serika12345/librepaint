/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeRenameCommand.h>

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

namespace KoShapeRenameCommandTesting
{
using ShapeNameReader = QString (*)(const KoShape *shape);
using ShapeNameWriter = void (*)(KoShape *shape, const QString &name);

void setShapeAccessForTesting(ShapeNameReader reader, ShapeNameWriter writer);
void resetShapeAccessForTesting();
} // namespace KoShapeRenameCommandTesting

namespace
{
struct ShapeObservation {
    QString name;
    int readCount{0};
    int writeCount{0};
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoShape *, ShapeObservation> shapeObservations;

QString readShapeName(const KoShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.readCount;
    return observation.name;
}

void writeShapeName(KoShape *shape, const QString &name)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.name = name;
    ++observation.writeCount;
    operationTrace.append(QStringLiteral("shape-write"));
}

class ShapeAccessScope
{
public:
    ShapeAccessScope()
    {
        KoShapeRenameCommandTesting::setShapeAccessForTesting(readShapeName, writeShapeName);
    }

    ~ShapeAccessScope()
    {
        KoShapeRenameCommandTesting::resetShapeAccessForTesting();
    }
};

class RenameCommandProbe : public KoShapeRenameCommand
{
public:
    RenameCommandProbe(KoShape *shape, const QString &newName, KUndo2Command *parent, int *destructionCount)
        : KoShapeRenameCommand(shape, newName, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~RenameCommandProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};
} // namespace

QString KoShape::name() const
{
    return readShapeName(this);
}

void KoShape::setName(const QString &name)
{
    writeShapeName(this, name);
}

class KoShapeRenameCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void renamesAcrossUndoRedo_data();
    void renamesAcrossUndoRedo();
    void keepsBorrowedShapeAndVirtualLifetime();
};

void KoShapeRenameCommandContractTest::renamesAcrossUndoRedo_data()
{
    QTest::addColumn<QString>("originalName");
    QTest::addColumn<QString>("newName");
    QTest::addColumn<QString>("intermediateName");

    QTest::newRow("unicode") << QString::fromUtf8("元の図形") << QString::fromUtf8("新しい図形")
                             << QString::fromUtf8("途中の図形");
    QTest::newRow("empty") << QStringLiteral("visible") << QString() << QStringLiteral("intermediate");
    QTest::newRow("same") << QStringLiteral("same") << QStringLiteral("same") << QStringLiteral("external");
}

void KoShapeRenameCommandContractTest::renamesAcrossUndoRedo()
{
    QFETCH(QString, originalName);
    QFETCH(QString, newName);
    QFETCH(QString, intermediateName);

    ShapeAccessScope accessScope;
    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    shapeObservations = {{shape, {originalName, 0, 0}}};
    KUndo2Command parent;
    KoShapeRenameCommand command(shape, newName, &parent);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Rename Shape"));
    QCOMPARE(shapeObservations[shape].readCount, 1);

    shapeObservations[shape].name = intermediateName;
    operationTrace.clear();

    command.redo();
    QCOMPARE(shapeObservations[shape].name, newName);
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo"), QStringLiteral("shape-write")}));

    operationTrace.clear();
    command.undo();
    QCOMPARE(shapeObservations[shape].name, originalName);
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo"), QStringLiteral("shape-write")}));

    operationTrace.clear();
    command.redo();
    QCOMPARE(shapeObservations[shape].name, newName);
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo"), QStringLiteral("shape-write")}));
    QCOMPARE(shapeObservations[shape].readCount, 1);
    QCOMPARE(shapeObservations[shape].writeCount, 3);
}

void KoShapeRenameCommandContractTest::keepsBorrowedShapeAndVirtualLifetime()
{
    ShapeAccessScope accessScope;
    ShapeToken storage;
    auto *shape = reinterpret_cast<KoShape *>(&storage);
    shapeObservations = {{shape, {QStringLiteral("borrowed"), 0, 0}}};
    KUndo2Command parent;
    int destructionCount = 0;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *command = new RenameCommandProbe(shape, QStringLiteral("renamed"), &parent, &destructionCount);

    delete command;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QCOMPARE(shapeObservations[shape].name, QStringLiteral("borrowed"));
    QCOMPARE(shapeObservations[shape].readCount, 1);
    QCOMPARE(shapeObservations[shape].writeCount, 0);
}

QTEST_GUILESS_MAIN(KoShapeRenameCommandContractTest)

#include "KoShapeRenameCommandContractTest.moc"
