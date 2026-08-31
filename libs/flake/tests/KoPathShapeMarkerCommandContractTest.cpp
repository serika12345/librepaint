/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoMarker.h>
#include <KoPathShape.h>
#include <KoPathShapeMarkerCommand.h>
#include <KoShapeBulkActionLock.h>

#include <kis_command_ids.h>

#include <QExplicitlySharedDataPointer>
#include <QHash>
#include <QStringList>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace KoPathShapeMarkerCommandTesting
{
using MarkerReader = KoMarker *(*)(const KoPathShape *shape, KoFlake::MarkerPosition position);
using AutoFillMarkerReader = bool (*)(const KoPathShape *shape);
using MarkerBatchApplier = void (*)(const QList<KoPathShape *> &shapes,
                                    const QList<KoMarker *> &markers,
                                    KoFlake::MarkerPosition position,
                                    const QList<bool> &autoFillMarkers);

void setShapeAccessForTesting(MarkerReader markerReader,
                              AutoFillMarkerReader autoFillMarkerReader,
                              MarkerBatchApplier markerBatchApplier);
void resetShapeAccessForTesting();
} // namespace KoPathShapeMarkerCommandTesting

namespace
{
QHash<const KUndo2Command *, KUndo2MagicString> commandTexts;
QHash<const KUndo2Command *, KUndo2Command *> commandParents;
QStringList operationTrace;
int baseDestructionCount = 0;
QHash<const KoMarker *, int> markerDestructionCounts;

struct ShapeObservation {
    QString label;
    KoMarker *marker{nullptr};
    bool autoFillMarkers{false};
    int markerReadCount{0};
    int autoFillReadCount{0};
};

struct BatchObservation {
    QList<KoPathShape *> shapes;
    QList<KoMarker *> markers;
    KoFlake::MarkerPosition position{KoFlake::StartMarker};
    QList<bool> autoFillMarkers;
};

struct alignas(std::max_align_t) PathShapeToken {
    unsigned char value{0};
};

QHash<const KoPathShape *, ShapeObservation> shapeObservations;
QList<BatchObservation> batchObservations;

[[noreturn]] void unexpectedCall(const char *name)
{
    qFatal("Unexpected production path reached: %s", name);
}

KoPathShape *pathShapePointer(PathShapeToken &token)
{
    return reinterpret_cast<KoPathShape *>(&token);
}

QExplicitlySharedDataPointer<KoMarker> makeMarker()
{
    KoMarker *marker = new KoMarker;
    markerDestructionCounts.insert(marker, 0);
    return QExplicitlySharedDataPointer<KoMarker>(marker);
}

KoMarker *readMarker(const KoPathShape *shape, KoFlake::MarkerPosition)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.markerReadCount;
    operationTrace.append(QStringLiteral("marker-read:%1").arg(observation.label));
    return observation.marker;
}

bool readAutoFillMarkers(const KoPathShape *shape)
{
    ShapeObservation &observation = shapeObservations[shape];
    ++observation.autoFillReadCount;
    operationTrace.append(QStringLiteral("auto-fill-read:%1").arg(observation.label));
    return observation.autoFillMarkers;
}

void applyMarkerBatch(const QList<KoPathShape *> &shapes,
                      const QList<KoMarker *> &markers,
                      KoFlake::MarkerPosition position,
                      const QList<bool> &autoFillMarkers)
{
    batchObservations.append({shapes, markers, position, autoFillMarkers});
    operationTrace.append(QStringLiteral("marker-batch"));

    const qsizetype count = qMin(shapes.size(), qMin(markers.size(), autoFillMarkers.size()));
    for (qsizetype i = 0; i < count; ++i) {
        ShapeObservation &observation = shapeObservations[shapes.at(i)];
        observation.marker = markers.at(i);
        observation.autoFillMarkers = autoFillMarkers.at(i);
    }
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

KoMarker::KoMarker()
    : d(nullptr)
{
}

KoMarker::~KoMarker()
{
    ++markerDestructionCounts[this];
}

KoMarker *KoPathShape::marker(KoFlake::MarkerPosition) const
{
    unexpectedCall("KoPathShape::marker");
}

bool KoPathShape::autoFillMarkers() const
{
    unexpectedCall("KoPathShape::autoFillMarkers");
}

void KoPathShape::setMarker(KoMarker *, KoFlake::MarkerPosition)
{
    unexpectedCall("KoPathShape::setMarker");
}

void KoPathShape::setAutoFillMarkers(bool)
{
    unexpectedCall("KoPathShape::setAutoFillMarkers");
}

KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter(const QList<KoShape *> &)
{
    unexpectedCall("KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter");
}

void KoShapeBulkActionLockAdapter::lock()
{
    unexpectedCall("KoShapeBulkActionLockAdapter::lock");
}

void KoShapeBulkActionLockAdapter::unlock()
{
    unexpectedCall("KoShapeBulkActionLockAdapter::unlock");
}

KoShapeBulkActionLockAdapter::UpdatesList KoShapeBulkActionLockAdapter::takeFinalUpdatesList()
{
    unexpectedCall("KoShapeBulkActionLockAdapter::takeFinalUpdatesList");
}

KoShapeBulkActionLock::~KoShapeBulkActionLock()
{
    unexpectedCall("KoShapeBulkActionLock::~KoShapeBulkActionLock");
}

void KoShapeBulkActionLock::bulkShapesUpdate(const UpdatesList &)
{
    unexpectedCall("KoShapeBulkActionLock::bulkShapesUpdate");
}

class KoPathShapeMarkerCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void capturesCurrentMarkersAutoFillAndBorrowedShapeOrder();
    void redoAndUndoDispatchBaseBeforeOrderedMarkerBatch_data();
    void redoAndUndoDispatchBaseBeforeOrderedMarkerBatch();
    void idAndMergeRequireIdenticalShapeSequenceAndPosition();
    void emptyShapesAndNullMarkerRemainSafe();
};

void KoPathShapeMarkerCommandContractTest::init()
{
    commandTexts.clear();
    commandParents.clear();
    operationTrace.clear();
    baseDestructionCount = 0;
    markerDestructionCounts.clear();
    shapeObservations.clear();
    batchObservations.clear();
    KoPathShapeMarkerCommandTesting::setShapeAccessForTesting(readMarker, readAutoFillMarkers, applyMarkerBatch);
}

void KoPathShapeMarkerCommandContractTest::cleanup()
{
    KoPathShapeMarkerCommandTesting::resetShapeAccessForTesting();
}

void KoPathShapeMarkerCommandContractTest::capturesCurrentMarkersAutoFillAndBorrowedShapeOrder()
{
    PathShapeToken firstStorage;
    firstStorage.value = 17;
    PathShapeToken secondStorage;
    secondStorage.value = 29;
    KoPathShape *const firstShape = pathShapePointer(firstStorage);
    KoPathShape *const secondShape = pathShapePointer(secondStorage);
    QExplicitlySharedDataPointer<KoMarker> oldFirst = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> oldSecond = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> replacement = makeMarker();
    KoMarker *const oldFirstPointer = oldFirst.data();
    KoMarker *const oldSecondPointer = oldSecond.data();
    KoMarker *const replacementPointer = replacement.data();
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirstPointer, false, 0, 0}},
        {secondShape, {QStringLiteral("second"), oldSecondPointer, true, 0, 0}},
    };
    KUndo2Command parent;
    const int destructionCountBeforeCommand = baseDestructionCount;

    {
        KoPathShapeMarkerCommand command({firstShape, secondShape}, replacementPointer, KoFlake::MidMarker, &parent);

        QCOMPARE(commandParents.value(&command), &parent);
        QCOMPARE(commandTexts.value(&command).toString(), QStringLiteral("Set marker"));
        QCOMPARE(operationTrace,
                 QStringList({QStringLiteral("marker-read:first"),
                              QStringLiteral("auto-fill-read:first"),
                              QStringLiteral("marker-read:second"),
                              QStringLiteral("auto-fill-read:second")}));
        QCOMPARE(shapeObservations[firstShape].markerReadCount, 1);
        QCOMPARE(shapeObservations[firstShape].autoFillReadCount, 1);
        QCOMPARE(shapeObservations[secondShape].markerReadCount, 1);
        QCOMPARE(shapeObservations[secondShape].autoFillReadCount, 1);

        oldFirst.reset();
        oldSecond.reset();
        replacement.reset();
        QCOMPARE(markerDestructionCounts.value(oldFirstPointer), 0);
        QCOMPARE(markerDestructionCounts.value(oldSecondPointer), 0);
        QCOMPARE(markerDestructionCounts.value(replacementPointer), 0);
    }

    QCOMPARE(baseDestructionCount, destructionCountBeforeCommand + 1);
    QCOMPARE(markerDestructionCounts.value(oldFirstPointer), 1);
    QCOMPARE(markerDestructionCounts.value(oldSecondPointer), 1);
    QCOMPARE(markerDestructionCounts.value(replacementPointer), 1);
    QCOMPARE(firstStorage.value, 17);
    QCOMPARE(secondStorage.value, 29);
}

void KoPathShapeMarkerCommandContractTest::redoAndUndoDispatchBaseBeforeOrderedMarkerBatch_data()
{
    QTest::addColumn<int>("position");

    QTest::newRow("start") << int(KoFlake::StartMarker);
    QTest::newRow("middle") << int(KoFlake::MidMarker);
    QTest::newRow("end") << int(KoFlake::EndMarker);
}

void KoPathShapeMarkerCommandContractTest::redoAndUndoDispatchBaseBeforeOrderedMarkerBatch()
{
    QFETCH(int, position);
    const auto markerPosition = static_cast<KoFlake::MarkerPosition>(position);

    PathShapeToken firstStorage;
    PathShapeToken secondStorage;
    KoPathShape *const firstShape = pathShapePointer(firstStorage);
    KoPathShape *const secondShape = pathShapePointer(secondStorage);
    QExplicitlySharedDataPointer<KoMarker> oldFirst = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> oldSecond = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> replacement = makeMarker();
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirst.data(), false, 0, 0}},
        {secondShape, {QStringLiteral("second"), oldSecond.data(), true, 0, 0}},
    };
    KoPathShapeMarkerCommand command({firstShape, secondShape}, replacement.data(), markerPosition);

    operationTrace.clear();
    batchObservations.clear();
    command.redo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-redo"), QStringLiteral("marker-batch")}));
    QCOMPARE(batchObservations.size(), 1);
    QCOMPARE(batchObservations.first().shapes, QList<KoPathShape *>({firstShape, secondShape}));
    QCOMPARE(batchObservations.first().markers, QList<KoMarker *>({replacement.data(), replacement.data()}));
    QCOMPARE(batchObservations.first().position, markerPosition);
    QCOMPARE(batchObservations.first().autoFillMarkers, QList<bool>({true, true}));

    operationTrace.clear();
    command.undo();
    QCOMPARE(operationTrace, QStringList({QStringLiteral("base-undo"), QStringLiteral("marker-batch")}));
    QCOMPARE(batchObservations.size(), 2);
    QCOMPARE(batchObservations.last().shapes, QList<KoPathShape *>({firstShape, secondShape}));
    QCOMPARE(batchObservations.last().markers, QList<KoMarker *>({oldFirst.data(), oldSecond.data()}));
    QCOMPARE(batchObservations.last().position, markerPosition);
    QCOMPARE(batchObservations.last().autoFillMarkers, QList<bool>({false, true}));
}

void KoPathShapeMarkerCommandContractTest::idAndMergeRequireIdenticalShapeSequenceAndPosition()
{
    PathShapeToken firstStorage;
    PathShapeToken secondStorage;
    KoPathShape *const firstShape = pathShapePointer(firstStorage);
    KoPathShape *const secondShape = pathShapePointer(secondStorage);
    QExplicitlySharedDataPointer<KoMarker> oldFirst = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> oldSecond = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> initial = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> replacement = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> reorderedReplacement = makeMarker();
    QExplicitlySharedDataPointer<KoMarker> otherPositionReplacement = makeMarker();
    shapeObservations = {
        {firstShape, {QStringLiteral("first"), oldFirst.data(), false, 0, 0}},
        {secondShape, {QStringLiteral("second"), oldSecond.data(), true, 0, 0}},
    };
    const QList<KoPathShape *> shapes{firstShape, secondShape};
    KoPathShapeMarkerCommand command(shapes, initial.data(), KoFlake::StartMarker);
    KoPathShapeMarkerCommand replacementCommand(shapes, replacement.data(), KoFlake::StartMarker);
    KoPathShapeMarkerCommand reorderedCommand({secondShape, firstShape},
                                              reorderedReplacement.data(),
                                              KoFlake::StartMarker);
    KoPathShapeMarkerCommand otherPositionCommand(shapes, otherPositionReplacement.data(), KoFlake::EndMarker);
    KUndo2Command unrelated;

    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::ChangeShapeMarkersId));
    QVERIFY(command.mergeWith(&replacementCommand));
    QVERIFY(!command.mergeWith(&reorderedCommand));
    QVERIFY(!command.mergeWith(&otherPositionCommand));
    QVERIFY(!command.mergeWith(&unrelated));

    batchObservations.clear();
    command.redo();
    QCOMPARE(batchObservations.first().markers, QList<KoMarker *>({replacement.data(), replacement.data()}));
    command.undo();
    QCOMPARE(batchObservations.last().markers, QList<KoMarker *>({oldFirst.data(), oldSecond.data()}));
    QCOMPARE(batchObservations.last().autoFillMarkers, QList<bool>({false, true}));
}

void KoPathShapeMarkerCommandContractTest::emptyShapesAndNullMarkerRemainSafe()
{
    KoPathShapeMarkerCommand empty({}, nullptr, KoFlake::StartMarker);

    operationTrace.clear();
    batchObservations.clear();
    empty.redo();
    empty.undo();
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("base-redo"),
                          QStringLiteral("marker-batch"),
                          QStringLiteral("base-undo"),
                          QStringLiteral("marker-batch")}));
    QCOMPARE(batchObservations.size(), 2);
    QVERIFY(batchObservations.first().shapes.isEmpty());
    QVERIFY(batchObservations.first().markers.isEmpty());
    QVERIFY(batchObservations.first().autoFillMarkers.isEmpty());

    PathShapeToken storage;
    KoPathShape *const shape = pathShapePointer(storage);
    QExplicitlySharedDataPointer<KoMarker> oldMarker = makeMarker();
    shapeObservations = {{shape, {QStringLiteral("shape"), oldMarker.data(), false, 0, 0}}};
    KoPathShapeMarkerCommand removeMarker({shape}, nullptr, KoFlake::EndMarker);

    batchObservations.clear();
    removeMarker.redo();
    QCOMPARE(batchObservations.first().markers, QList<KoMarker *>({nullptr}));
    QCOMPARE(batchObservations.first().autoFillMarkers, QList<bool>({true}));
    removeMarker.undo();
    QCOMPARE(batchObservations.last().markers, QList<KoMarker *>({oldMarker.data()}));
    QCOMPARE(batchObservations.last().autoFillMarkers, QList<bool>({false}));
}

QTEST_GUILESS_MAIN(KoPathShapeMarkerCommandContractTest)

#include "KoPathShapeMarkerCommandContractTest.moc"
