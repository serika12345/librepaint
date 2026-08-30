/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoKeepShapesSelectedCommand.h>
#include <KoSelectedShapesProxy.h>
#include <KoSelection.h>
#include <KoShape.h>

#include <QHash>
#include <QPointer>
#include <QStringList>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace
{
QHash<const KUndo2Command *, KUndo2Command *> commandParents;
QStringList operationTrace;
QHash<const KoShape *, QString> shapeLabels;
int baseDestructionCount = 0;

struct SelectionObservation {
    int deselectAllCount{0};
    QList<KoShape *> selectedShapes;
};

QHash<const KoSelection *, SelectionObservation> selectionObservations;
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
    commandParents.remove(this);
}

void KUndo2Command::undo()
{
}

void KUndo2Command::redo()
{
}

QString KUndo2Command::actionText() const
{
    return QString();
}

void KUndo2Command::setText(const KUndo2MagicString &text)
{
    Q_UNUSED(text);
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

namespace KisCommandUtils
{
FlipFlopCommand::FlipFlopCommand(bool finalizing, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_currentState(finalizing ? State::FINALIZING : State::INITIALIZING)
{
}

FlipFlopCommand::FlipFlopCommand(State initialState, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_currentState(initialState)
{
}

void FlipFlopCommand::redo()
{
    if (m_currentState == State::INITIALIZING) {
        partA();
    } else {
        partB();
    }
    m_firstRedo = false;
}

void FlipFlopCommand::undo()
{
    if (m_currentState == State::FINALIZING) {
        partA();
    } else {
        partB();
    }
}

void FlipFlopCommand::partA()
{
}

void FlipFlopCommand::partB()
{
}
} // namespace KisCommandUtils

KoSelectedShapesProxy::KoSelectedShapesProxy(QObject *parent)
    : QObject(parent)
{
}

void KoSelection::deselectAll()
{
    SelectionObservation &observation = selectionObservations[this];
    ++observation.deselectAllCount;
    observation.selectedShapes.clear();
    operationTrace.append(QStringLiteral("selection-clear"));
}

void KoSelection::select(KoShape *shape)
{
    selectionObservations[this].selectedShapes.append(shape);
    operationTrace.append(QStringLiteral("selection-select:%1").arg(shapeLabels.value(shape)));
}

namespace
{
struct alignas(std::max_align_t) SelectionToken {
    unsigned char value{0};
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

KoSelection *selectionPointer(SelectionToken &token)
{
    return reinterpret_cast<KoSelection *>(&token);
}

KoShape *shapePointer(ShapeToken &token)
{
    return reinterpret_cast<KoShape *>(&token);
}

class SelectedShapesProxyProbe final : public KoSelectedShapesProxy
{
public:
    explicit SelectedShapesProxyProbe(KoSelection *selection)
        : m_selection(selection)
    {
    }

    KoSelection *selection() override
    {
        ++m_selectionReadCount;
        operationTrace.append(QStringLiteral("proxy-selection"));
        return m_selection;
    }

    int selectionReadCount() const
    {
        return m_selectionReadCount;
    }

private:
    KoSelection *m_selection;
    int m_selectionReadCount{0};
};
} // namespace

class KoKeepShapesSelectedCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void finalizingRedoReplacesSelectionWithAfterListInOrder();
    void initializingUndoReplacesSelectionWithBeforeListInOrder();
    void emptyPhaseListClearsSelectionWithoutSelectingShapes_data();
    void emptyPhaseListClearsSelectionWithoutSelectingShapes();
    void forwardsParentAndBorrowsProxyAndShapes();
};

void KoKeepShapesSelectedCommandContractTest::init()
{
    commandParents.clear();
    operationTrace.clear();
    shapeLabels.clear();
    selectionObservations.clear();
    baseDestructionCount = 0;
}

void KoKeepShapesSelectedCommandContractTest::finalizingRedoReplacesSelectionWithAfterListInOrder()
{
    SelectionToken selectionStorage;
    ShapeToken beforeStorage;
    ShapeToken afterFirstStorage;
    ShapeToken afterSecondStorage;
    KoSelection *const selection = selectionPointer(selectionStorage);
    KoShape *const before = shapePointer(beforeStorage);
    KoShape *const afterFirst = shapePointer(afterFirstStorage);
    KoShape *const afterSecond = shapePointer(afterSecondStorage);
    shapeLabels = {
        {before, QStringLiteral("before")},
        {afterFirst, QStringLiteral("after-first")},
        {afterSecond, QStringLiteral("after-second")},
    };
    SelectedShapesProxyProbe proxy(selection);
    KoKeepShapesSelectedCommand command({before}, {afterFirst, afterSecond}, &proxy, true, nullptr);

    QCOMPARE(proxy.selectionReadCount(), 0);
    command.undo();
    QCOMPARE(proxy.selectionReadCount(), 0);
    QVERIFY(operationTrace.isEmpty());

    command.redo();
    QCOMPARE(proxy.selectionReadCount(), 1);
    QCOMPARE(selectionObservations[selection].deselectAllCount, 1);
    QCOMPARE(selectionObservations[selection].selectedShapes, QList<KoShape *>({afterFirst, afterSecond}));
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("proxy-selection"),
                          QStringLiteral("selection-clear"),
                          QStringLiteral("selection-select:after-first"),
                          QStringLiteral("selection-select:after-second")}));
}

void KoKeepShapesSelectedCommandContractTest::initializingUndoReplacesSelectionWithBeforeListInOrder()
{
    SelectionToken selectionStorage;
    ShapeToken beforeFirstStorage;
    ShapeToken beforeSecondStorage;
    ShapeToken afterStorage;
    KoSelection *const selection = selectionPointer(selectionStorage);
    KoShape *const beforeFirst = shapePointer(beforeFirstStorage);
    KoShape *const beforeSecond = shapePointer(beforeSecondStorage);
    KoShape *const after = shapePointer(afterStorage);
    shapeLabels = {
        {beforeFirst, QStringLiteral("before-first")},
        {beforeSecond, QStringLiteral("before-second")},
        {after, QStringLiteral("after")},
    };
    SelectedShapesProxyProbe proxy(selection);
    KoKeepShapesSelectedCommand command({beforeFirst, beforeSecond}, {after}, &proxy, false, nullptr);

    QCOMPARE(proxy.selectionReadCount(), 0);
    command.redo();
    QCOMPARE(proxy.selectionReadCount(), 0);
    QVERIFY(operationTrace.isEmpty());

    command.undo();
    QCOMPARE(proxy.selectionReadCount(), 1);
    QCOMPARE(selectionObservations[selection].deselectAllCount, 1);
    QCOMPARE(selectionObservations[selection].selectedShapes, QList<KoShape *>({beforeFirst, beforeSecond}));
    QCOMPARE(operationTrace,
             QStringList({QStringLiteral("proxy-selection"),
                          QStringLiteral("selection-clear"),
                          QStringLiteral("selection-select:before-first"),
                          QStringLiteral("selection-select:before-second")}));
}

void KoKeepShapesSelectedCommandContractTest::emptyPhaseListClearsSelectionWithoutSelectingShapes_data()
{
    QTest::addColumn<bool>("finalizing");

    QTest::newRow("finalizing-redo") << true;
    QTest::newRow("initializing-undo") << false;
}

void KoKeepShapesSelectedCommandContractTest::emptyPhaseListClearsSelectionWithoutSelectingShapes()
{
    QFETCH(bool, finalizing);

    SelectionToken selectionStorage;
    ShapeToken nonEmptyStorage;
    KoSelection *const selection = selectionPointer(selectionStorage);
    KoShape *const nonEmptyShape = shapePointer(nonEmptyStorage);
    SelectedShapesProxyProbe proxy(selection);
    const QList<KoShape *> selectedBefore = finalizing ? QList<KoShape *>({nonEmptyShape}) : QList<KoShape *>();
    const QList<KoShape *> selectedAfter = finalizing ? QList<KoShape *>() : QList<KoShape *>({nonEmptyShape});
    KoKeepShapesSelectedCommand command(selectedBefore, selectedAfter, &proxy, finalizing, nullptr);

    if (finalizing) {
        command.redo();
    } else {
        command.undo();
    }

    QCOMPARE(proxy.selectionReadCount(), 1);
    QCOMPARE(selectionObservations[selection].deselectAllCount, 1);
    QVERIFY(selectionObservations[selection].selectedShapes.isEmpty());
    QCOMPARE(operationTrace, QStringList({QStringLiteral("proxy-selection"), QStringLiteral("selection-clear")}));
}

void KoKeepShapesSelectedCommandContractTest::forwardsParentAndBorrowsProxyAndShapes()
{
    SelectionToken selectionStorage;
    selectionStorage.value = 17;
    ShapeToken beforeStorage;
    beforeStorage.value = 23;
    ShapeToken afterStorage;
    afterStorage.value = 29;
    KoSelection *const selection = selectionPointer(selectionStorage);
    KoShape *const before = shapePointer(beforeStorage);
    KoShape *const after = shapePointer(afterStorage);
    SelectedShapesProxyProbe proxy(selection);
    QPointer<KoSelectedShapesProxy> proxyGuard(&proxy);
    KUndo2Command parent;
    const int destructionCountBeforeCommand = baseDestructionCount;

    {
        KoKeepShapesSelectedCommand command({before}, {after}, &proxy, true, &parent);
        QCOMPARE(commandParents.value(&command), &parent);
    }

    QCOMPARE(baseDestructionCount, destructionCountBeforeCommand + 1);
    QVERIFY(!proxyGuard.isNull());
    QCOMPARE(proxyGuard.data(), &proxy);
    QCOMPARE(selectionStorage.value, 17);
    QCOMPARE(beforeStorage.value, 23);
    QCOMPARE(afterStorage.value, 29);
    QVERIFY(operationTrace.isEmpty());
}

QTEST_GUILESS_MAIN(KoKeepShapesSelectedCommandContractTest)

#include "KoKeepShapesSelectedCommandContractTest.moc"
