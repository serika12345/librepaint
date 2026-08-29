/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoPathReverseCommand.h>
#include <KoPathShape.h>

#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>
#include <QVector>

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

namespace KoPathReverseCommandTesting
{
using SubpathCountReader = int (*)(const KoPathShape *path);
using SubpathReverser = bool (*)(KoPathShape *path, int subpathIndex);

void setPathAccessForTesting(SubpathCountReader countReader, SubpathReverser reverser);
void resetPathAccessForTesting();
} // namespace KoPathReverseCommandTesting

namespace
{
struct PathObservation {
    QString label;
    int subpathCount{0};
    QVector<bool> reversed;
    int countReadCount{0};
    int reverseCount{0};
};

struct alignas(std::max_align_t) PathToken {
    unsigned char value{0};
};

QHash<const KoPathShape *, PathObservation> pathObservations;

int readSubpathCount(const KoPathShape *path)
{
    PathObservation &observation = pathObservations[path];
    ++observation.countReadCount;
    operationTrace.append(QStringLiteral("path-count:%1").arg(observation.label));
    return observation.subpathCount;
}

bool reverseSubpath(KoPathShape *path, int subpathIndex)
{
    PathObservation &observation = pathObservations[path];
    if (subpathIndex < 0 || subpathIndex >= observation.reversed.size()) {
        return false;
    }

    observation.reversed[subpathIndex] = !observation.reversed[subpathIndex];
    ++observation.reverseCount;
    operationTrace.append(QStringLiteral("path-reverse:%1:%2").arg(observation.label).arg(subpathIndex));
    return true;
}

class PathAccessScope
{
public:
    PathAccessScope()
    {
        KoPathReverseCommandTesting::setPathAccessForTesting(readSubpathCount, reverseSubpath);
    }

    ~PathAccessScope()
    {
        KoPathReverseCommandTesting::resetPathAccessForTesting();
    }
};

class PathReverseCommandProbe : public KoPathReverseCommand
{
public:
    PathReverseCommandProbe(const QList<KoPathShape *> &paths, KUndo2Command *parent, int *destructionCount)
        : KoPathReverseCommand(paths, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~PathReverseCommandProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};
} // namespace

int KoPathShape::subpathCount() const
{
    return readSubpathCount(this);
}

bool KoPathShape::reverseSubpath(int subpathIndex)
{
    return ::reverseSubpath(this, subpathIndex);
}

class KoPathReverseCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void reversesEverySubpathAcrossUndoCycles();
    void handlesEmptyPathList();
    void keepsBorrowedPathsAndVirtualLifetime();
};

void KoPathReverseCommandContractTest::reversesEverySubpathAcrossUndoCycles()
{
    PathAccessScope accessScope;
    PathToken firstStorage;
    PathToken emptyStorage;
    PathToken secondStorage;
    auto *firstPath = reinterpret_cast<KoPathShape *>(&firstStorage);
    auto *emptyPath = reinterpret_cast<KoPathShape *>(&emptyStorage);
    auto *secondPath = reinterpret_cast<KoPathShape *>(&secondStorage);
    pathObservations = {
        {firstPath, {QStringLiteral("first"), 2, QVector<bool>(2, false), 0, 0}},
        {emptyPath, {QStringLiteral("empty"), 0, {}, 0, 0}},
        {secondPath, {QStringLiteral("second"), 3, QVector<bool>(3, false), 0, 0}},
    };
    KUndo2Command parent;
    KoPathReverseCommand command({firstPath, emptyPath, secondPath}, &parent);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Reverse paths"));
    operationTrace.clear();

    const QStringList reverseTrace = {
        QStringLiteral("path-count:first"),
        QStringLiteral("path-reverse:first:0"),
        QStringLiteral("path-reverse:first:1"),
        QStringLiteral("path-count:empty"),
        QStringLiteral("path-count:second"),
        QStringLiteral("path-reverse:second:0"),
        QStringLiteral("path-reverse:second:1"),
        QStringLiteral("path-reverse:second:2"),
    };

    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + reverseTrace);
    QCOMPARE(pathObservations[firstPath].reversed, QVector<bool>({true, true}));
    QCOMPARE(pathObservations[secondPath].reversed, QVector<bool>({true, true, true}));

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo")}) + reverseTrace);
    QCOMPARE(pathObservations[firstPath].reversed, QVector<bool>({false, false}));
    QCOMPARE(pathObservations[secondPath].reversed, QVector<bool>({false, false, false}));

    operationTrace.clear();
    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo")}) + reverseTrace);
    QCOMPARE(pathObservations[firstPath].reversed, QVector<bool>({true, true}));
    QCOMPARE(pathObservations[secondPath].reversed, QVector<bool>({true, true, true}));
    QCOMPARE(pathObservations[firstPath].countReadCount, 3);
    QCOMPARE(pathObservations[emptyPath].countReadCount, 3);
    QCOMPARE(pathObservations[secondPath].countReadCount, 3);
    QCOMPARE(pathObservations[firstPath].reverseCount, 6);
    QCOMPARE(pathObservations[emptyPath].reverseCount, 0);
    QCOMPARE(pathObservations[secondPath].reverseCount, 9);
}

void KoPathReverseCommandContractTest::handlesEmptyPathList()
{
    PathAccessScope accessScope;
    pathObservations.clear();
    operationTrace.clear();
    KUndo2Command parent;
    KoPathReverseCommand command({}, &parent);

    QCOMPARE(commandParents.value(&command), &parent);
    QCOMPARE(command.actionText(), QStringLiteral("Reverse paths"));

    command.redo();
    command.undo();
    command.redo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"), QStringLiteral("base-undo"), QStringLiteral("base-redo")}));
    QVERIFY(pathObservations.isEmpty());
}

void KoPathReverseCommandContractTest::keepsBorrowedPathsAndVirtualLifetime()
{
    PathAccessScope accessScope;
    PathToken storage;
    auto *path = reinterpret_cast<KoPathShape *>(&storage);
    pathObservations = {{path, {QStringLiteral("borrowed"), 1, QVector<bool>(1, false), 0, 0}}};
    operationTrace.clear();
    KUndo2Command parent;
    int destructionCount = 0;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *command = new PathReverseCommandProbe({path}, &parent, &destructionCount);

    delete command;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QCOMPARE(pathObservations[path].reversed, QVector<bool>({false}));
    QCOMPARE(pathObservations[path].countReadCount, 0);
    QCOMPARE(pathObservations[path].reverseCount, 0);
    QVERIFY(operationTrace.isEmpty());
}

QTEST_GUILESS_MAIN(KoPathReverseCommandContractTest)

#include "KoPathReverseCommandContractTest.moc"
