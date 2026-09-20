/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoPathPoint.h>
#include <KoPathShape.h>
#include <KoSubpathRemoveCommand.h>

#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>
#include <QTransform>

#include <cstddef>
#include <new>
#include <utility>

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

namespace KoSubpathRemoveCommandTesting
{
using PathUpdater = void (*)(KoPathShape *pathShape);
using SubpathRemover = KoSubpath *(*)(KoPathShape * pathShape, int subpathIndex);
using PathNormalizer = QPointF (*)(KoPathShape *pathShape);
using PointMapper = void (*)(KoPathPoint *point, const QTransform &matrix);
using SubpathAdder = bool (*)(KoPathShape *pathShape, KoSubpath *subpath, int subpathIndex);
using SubpathDeleter = void (*)(KoSubpath *subpath);

void setPathAccessForTesting(PathUpdater updater,
                             SubpathRemover remover,
                             PathNormalizer normalizer,
                             PointMapper mapper,
                             SubpathAdder adder,
                             SubpathDeleter deleter);
void resetPathAccessForTesting();
} // namespace KoSubpathRemoveCommandTesting

namespace
{
struct PathObservation {
    QString label;
    KoSubpath *removalResult{nullptr};
    QPointF normalizeOffset;
    bool addResult{true};
    int updateCount{0};
    QList<int> removedIndexes;
    QList<int> addedIndexes;
    QList<KoSubpath *> addedSubpaths;
};

struct alignas(std::max_align_t) PathToken {
    unsigned char value{0};
};

QHash<const KoPathShape *, PathObservation> pathObservations;
QHash<const KoPathPoint *, QString> pointLabels;
QHash<const KoPathPoint *, QList<QTransform>> pointMappings;
QList<const KoPathPoint *> deletedPoints;
QList<const KoSubpath *> deletedSubpaths;

KoPathPoint *allocatePoint(const QString &label)
{
    auto *point = static_cast<KoPathPoint *>(::operator new(sizeof(KoPathPoint)));
    pointLabels.insert(point, label);
    return point;
}

KoSubpath *allocateSubpath(std::initializer_list<KoPathPoint *> points)
{
    return new KoSubpath(points);
}

void observeUpdate(KoPathShape *pathShape)
{
    PathObservation &observation = pathObservations[pathShape];
    ++observation.updateCount;
    operationTrace.append(QStringLiteral("path-update:%1").arg(observation.label));
}

KoSubpath *observeRemoval(KoPathShape *pathShape, int subpathIndex)
{
    PathObservation &observation = pathObservations[pathShape];
    observation.removedIndexes.append(subpathIndex);
    operationTrace.append(QStringLiteral("path-remove:%1:%2").arg(observation.label).arg(subpathIndex));
    return observation.removalResult;
}

QPointF observeNormalization(KoPathShape *pathShape)
{
    const PathObservation &observation = pathObservations[pathShape];
    operationTrace.append(QStringLiteral("path-normalize:%1").arg(observation.label));
    return observation.normalizeOffset;
}

void observeMapping(KoPathPoint *point, const QTransform &matrix)
{
    pointMappings[point].append(matrix);
    operationTrace.append(QStringLiteral("point-map:%1").arg(pointLabels.value(point)));
}

bool observeAddition(KoPathShape *pathShape, KoSubpath *subpath, int subpathIndex)
{
    PathObservation &observation = pathObservations[pathShape];
    observation.addedIndexes.append(subpathIndex);
    observation.addedSubpaths.append(subpath);
    operationTrace.append(QStringLiteral("path-add:%1:%2").arg(observation.label).arg(subpathIndex));
    return observation.addResult;
}

void observeDeletion(KoSubpath *subpath)
{
    deletedSubpaths.append(subpath);
    operationTrace.append(QStringLiteral("subpath-delete"));
    for (KoPathPoint *point : std::as_const(*subpath)) {
        deletedPoints.append(point);
        operationTrace.append(QStringLiteral("point-delete:%1").arg(pointLabels.value(point)));
        pointLabels.remove(point);
        pointMappings.remove(point);
        ::operator delete(point);
    }
    delete subpath;
}

class PathAccessScope
{
public:
    PathAccessScope()
    {
        KoSubpathRemoveCommandTesting::setPathAccessForTesting(observeUpdate,
                                                               observeRemoval,
                                                               observeNormalization,
                                                               observeMapping,
                                                               observeAddition,
                                                               observeDeletion);
    }

    ~PathAccessScope()
    {
        KoSubpathRemoveCommandTesting::resetPathAccessForTesting();
    }
};

class SubpathRemoveCommandProbe : public KoSubpathRemoveCommand
{
public:
    SubpathRemoveCommandProbe(KoPathShape *pathShape, int subpathIndex, KUndo2Command *parent, int *destructionCount)
        : KoSubpathRemoveCommand(pathShape, subpathIndex, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~SubpathRemoveCommandProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

void resetObservations()
{
    pathObservations.clear();
    pointLabels.clear();
    pointMappings.clear();
    deletedPoints.clear();
    deletedSubpaths.clear();
    operationTrace.clear();
}
} // namespace

void KoShape::update() const
{
    observeUpdate(reinterpret_cast<KoPathShape *>(const_cast<KoShape *>(this)));
}

KoSubpath *KoPathShape::removeSubpath(int subpathIndex)
{
    return observeRemoval(this, subpathIndex);
}

QPointF KoPathShape::normalize()
{
    return observeNormalization(this);
}

bool KoPathShape::addSubpath(KoSubpath *subpath, int subpathIndex)
{
    return observeAddition(this, subpath, subpathIndex);
}

void KoPathPoint::map(const QTransform &matrix)
{
    observeMapping(this, matrix);
}

KoPathPoint::~KoPathPoint() = default;

class KoSubpathRemoveCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesSubpathAcrossUndoCycles();
    void handlesMissingSubpath();
    void transfersSubpathOwnershipAcrossLifetime();
    void losesOwnershipWhenReinsertionFails();
};

void KoSubpathRemoveCommandContractTest::preservesSubpathAcrossUndoCycles()
{
    PathAccessScope accessScope;
    resetObservations();
    PathToken pathStorage;
    auto *path = reinterpret_cast<KoPathShape *>(&pathStorage);
    KoPathPoint *firstPoint = allocatePoint(QStringLiteral("first"));
    KoPathPoint *secondPoint = allocatePoint(QStringLiteral("second"));
    KoSubpath *subpath = allocateSubpath({firstPoint, secondPoint});
    pathObservations.insert(path, {QStringLiteral("path"), subpath, QPointF(3.5, -2.25), true, 0, {}, {}, {}});
    KUndo2Command parent;

    {
        KoSubpathRemoveCommand command(path, 2, &parent);
        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(command.actionText(), QStringLiteral("Remove subpath"));

        const QStringList redoTrace = {
            QStringLiteral("base-redo"),
            QStringLiteral("path-update:path"),
            QStringLiteral("path-remove:path:2"),
            QStringLiteral("path-normalize:path"),
            QStringLiteral("point-map:first"),
            QStringLiteral("point-map:second"),
            QStringLiteral("path-update:path"),
        };
        command.redo();
        QCOMPARE(operationTrace, redoTrace);
        QCOMPARE(pointMappings[firstPoint].size(), 1);
        QCOMPARE(pointMappings[firstPoint].constFirst().dx(), -3.5);
        QCOMPARE(pointMappings[firstPoint].constFirst().dy(), 2.25);
        QCOMPARE(pointMappings[secondPoint], pointMappings[firstPoint]);

        operationTrace.clear();
        command.undo();
        QCOMPARE(operationTrace,
                 QStringList({QStringLiteral("base-undo"),
                              QStringLiteral("path-add:path:2"),
                              QStringLiteral("path-normalize:path"),
                              QStringLiteral("path-update:path")}));
        QCOMPARE(pathObservations[path].addedSubpaths, QList<KoSubpath *>({subpath}));

        operationTrace.clear();
        command.redo();
        QCOMPARE(operationTrace, redoTrace);
        QCOMPARE(pathObservations[path].removedIndexes, QList<int>({2, 2}));
        QCOMPARE(pathObservations[path].addedIndexes, QList<int>({2}));
        QCOMPARE(pathObservations[path].updateCount, 5);
        QCOMPARE(pointMappings[firstPoint].size(), 2);
        QCOMPARE(pointMappings[secondPoint].size(), 2);
        operationTrace.clear();
    }

    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("subpath-delete"),
                          QStringLiteral("point-delete:first"),
                          QStringLiteral("point-delete:second")}));
    QCOMPARE(deletedSubpaths, QList<const KoSubpath *>({subpath}));
    QCOMPARE(deletedPoints, QList<const KoPathPoint *>({firstPoint, secondPoint}));
}

void KoSubpathRemoveCommandContractTest::handlesMissingSubpath()
{
    PathAccessScope accessScope;
    resetObservations();
    PathToken pathStorage;
    auto *path = reinterpret_cast<KoPathShape *>(&pathStorage);
    pathObservations.insert(path, {QStringLiteral("missing"), nullptr, QPointF(1.0, 2.0), true, 0, {}, {}, {}});
    KoSubpathRemoveCommand command(path, -1);

    command.redo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("path-update:missing"),
                          QStringLiteral("path-remove:missing:-1")}));

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo")}));

    operationTrace.clear();
    command.redo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("path-update:missing"),
                          QStringLiteral("path-remove:missing:-1")}));
    QCOMPARE(pathObservations[path].updateCount, 2);
    QVERIFY(deletedSubpaths.isEmpty());
}

void KoSubpathRemoveCommandContractTest::transfersSubpathOwnershipAcrossLifetime()
{
    PathAccessScope accessScope;
    resetObservations();
    PathToken pathStorage;
    auto *path = reinterpret_cast<KoPathShape *>(&pathStorage);
    KoPathPoint *point = allocatePoint(QStringLiteral("returned"));
    KoSubpath *subpath = allocateSubpath({point});
    pathObservations.insert(path, {QStringLiteral("owned"), subpath, QPointF(0.0, 0.0), true, 0, {}, {}, {}});
    KUndo2Command parent;
    int destructionCount = 0;
    const int previousBaseDestructionCount = baseDestructionCount;
    KUndo2Command *command = new SubpathRemoveCommandProbe(path, 4, &parent, &destructionCount);

    command->redo();
    command->undo();
    operationTrace.clear();
    delete command;

    QCOMPARE(destructionCount, 1);
    QCOMPARE(baseDestructionCount, previousBaseDestructionCount + 1);
    QVERIFY(operationTrace.isEmpty());
    QVERIFY(deletedSubpaths.isEmpty());
    QCOMPARE(pathObservations[path].addedSubpaths, QList<KoSubpath *>({subpath}));

    observeDeletion(subpath);
}

void KoSubpathRemoveCommandContractTest::losesOwnershipWhenReinsertionFails()
{
    PathAccessScope accessScope;
    resetObservations();
    PathToken pathStorage;
    auto *path = reinterpret_cast<KoPathShape *>(&pathStorage);
    KoPathPoint *point = allocatePoint(QStringLiteral("leaked"));
    KoSubpath *subpath = allocateSubpath({point});
    pathObservations.insert(path, {QStringLiteral("failed"), subpath, QPointF(1.0, 1.0), false, 0, {}, {}, {}});
    auto *command = new KoSubpathRemoveCommand(path, 3);

    command->redo();
    operationTrace.clear();
    command->undo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-undo"),
                          QStringLiteral("path-add:failed:3"),
                          QStringLiteral("path-normalize:failed"),
                          QStringLiteral("path-update:failed")}));

    operationTrace.clear();
    command->undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo")}));
    operationTrace.clear();
    delete command;
    QVERIFY(operationTrace.isEmpty());
    QVERIFY(deletedSubpaths.isEmpty());

    observeDeletion(subpath);
}

QTEST_GUILESS_MAIN(KoSubpathRemoveCommandContractTest)

#include "KoSubpathRemoveCommandContractTest.moc"
