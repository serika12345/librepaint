/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeBulkActionLock.h>
#include <KoSvgTextReorderShapeInsideCommand.h>
#include <KoSvgTextShape.h>

#include <QHash>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace
{
QHash<const KUndo2Command *, KUndo2Command *> parents;
QStringList trace;
QHash<const KoSvgTextShape *, QList<KoShape *>> insideShapes;
struct MoveCall {
    KoShape *shape;
    int index;
};
QList<MoveCall> moves;
int destructionCount = 0;
struct alignas(std::max_align_t) Token {
    unsigned char value{0};
};
KoShape *shape(Token &token)
{
    return reinterpret_cast<KoShape *>(&token);
}
KoSvgTextShape *textShape(Token &token)
{
    return reinterpret_cast<KoSvgTextShape *>(&token);
}
} // namespace

KUndo2MagicString::KUndo2MagicString() = default;
KUndo2MagicString::KUndo2MagicString(const QString &text)
    : m_text(text)
{
}
QString KUndo2MagicString::toString() const
{
    return m_text.section(QLatin1Char('\n'), 0, 0);
}
QString KUndo2MagicString::toSecondaryString() const
{
    return m_text.section(QLatin1Char('\n'), 1);
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
    parents[this] = parent;
}
KUndo2Command::KUndo2Command(const KUndo2MagicString &, KUndo2Command *parent)
{
    parents[this] = parent;
}
KUndo2Command::~KUndo2Command()
{
    ++destructionCount;
    parents.remove(this);
}
void KUndo2Command::undo()
{
    trace << QStringLiteral("base-undo");
}
void KUndo2Command::redo()
{
    trace << QStringLiteral("base-redo");
}
QString KUndo2Command::actionText() const
{
    return {};
}
void KUndo2Command::setText(const KUndo2MagicString &)
{
}
int KUndo2Command::id() const
{
    return -1;
}
int KUndo2Command::timedId() const
{
    return m_timedID;
}
void KUndo2Command::setTimedID(int value)
{
    m_timedID = value;
}
bool KUndo2Command::mergeWith(const KUndo2Command *)
{
    return false;
}
bool KUndo2Command::timedMergeWith(KUndo2Command *)
{
    return false;
}
bool KUndo2Command::canAnnihilateWith(const KUndo2Command *) const
{
    return false;
}
void KUndo2Command::setTime(const QTime &value)
{
    m_timeOfCreation = value;
}
QTime KUndo2Command::time() const
{
    return m_timeOfCreation;
}
void KUndo2Command::setEndTime(const QTime &value)
{
    m_endOfCommand = value;
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

KoShapeBulkActionLockAdapter::KoShapeBulkActionLockAdapter(const QList<KoShape *> &)
{
    trace << QStringLiteral("lock-create");
}
void KoShapeBulkActionLockAdapter::lock()
{
    trace << QStringLiteral("lock");
}
void KoShapeBulkActionLockAdapter::unlock()
{
    trace << QStringLiteral("unlock");
}
KoShapeBulkActionLockAdapter::UpdatesList KoShapeBulkActionLockAdapter::takeFinalUpdatesList()
{
    return {};
}
KoShapeBulkActionLock::~KoShapeBulkActionLock()
{
    trace << QStringLiteral("lock-destroy");
}
void KoShapeBulkActionLock::bulkShapesUpdate(const UpdatesList &)
{
    trace << QStringLiteral("bulk-update");
}

KoSvgTextShapeMementoSP KoSvgTextShape::getMemento()
{
    qFatal("production memento reader reached");
    return {};
}
QList<KoShape *> KoSvgTextShape::shapesInside() const
{
    qFatal("production inside reader reached");
    return {};
}
void KoSvgTextShape::moveShapeInsideToIndex(KoShape *, const int)
{
    qFatal("production move writer reached");
}
void KoSvgTextShape::setMemento(const KoSvgTextShapeMementoSP)
{
    qFatal("production memento restorer reached");
}

namespace KoSvgTextReorderShapeInsideCommandTesting
{
using MementoReader = KoSvgTextShapeMementoSP (*)(KoSvgTextShape *);
using InsideShapesReader = QList<KoShape *> (*)(const KoSvgTextShape *);
using MoveInsideWriter = void (*)(KoSvgTextShape *, KoShape *, int);
using MementoRestorer = void (*)(KoSvgTextShape *, const KoSvgTextShapeMementoSP &);
void setShapeAccessForTesting(MementoReader, InsideShapesReader, MoveInsideWriter, MementoRestorer);
void resetShapeAccessForTesting();
} // namespace KoSvgTextReorderShapeInsideCommandTesting

namespace
{
KoSvgTextShapeMementoSP readMemento(KoSvgTextShape *)
{
    trace << QStringLiteral("memento-read");
    return {};
}
QList<KoShape *> readInside(const KoSvgTextShape *shape)
{
    return insideShapes.value(shape);
}
void moveInside(KoSvgTextShape *text, KoShape *item, int index)
{
    trace << QStringLiteral("move");
    moves << MoveCall{item, index};
    QList<KoShape *> &items = insideShapes[text];
    items.removeOne(item);
    items.insert(qBound(0, index, items.size()), item);
}
void restoreMemento(KoSvgTextShape *, const KoSvgTextShapeMementoSP &)
{
    trace << QStringLiteral("memento-restore");
}
class AccessScope
{
public:
    AccessScope()
    {
        trace.clear();
        moves.clear();
        insideShapes.clear();
        KoSvgTextReorderShapeInsideCommandTesting::setShapeAccessForTesting(readMemento,
                                                                            readInside,
                                                                            moveInside,
                                                                            restoreMemento);
    }
    ~AccessScope()
    {
        KoSvgTextReorderShapeInsideCommandTesting::resetShapeAccessForTesting();
    }
};
} // namespace

class KoSvgTextReorderShapeInsideCommandContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void exposesMoveTypesAndCapturesBorrowedShapes();
    void bringToFrontMovesTargetsInInsideOrder();
    void moveEarlierUsesFirstTargetBoundary();
    void laterAndBackDispatchInReverseOrder();
    void undoRestoresIndicesAndMemento();
};

void KoSvgTextReorderShapeInsideCommandContractTest::exposesMoveTypesAndCapturesBorrowedShapes()
{
    QCOMPARE(int(KoSvgTextReorderShapeInsideCommand::MoveEarlier), 0);
    QCOMPARE(int(KoSvgTextReorderShapeInsideCommand::MoveLater), 1);
    QCOMPARE(int(KoSvgTextReorderShapeInsideCommand::BringToFront), 2);
    QCOMPARE(int(KoSvgTextReorderShapeInsideCommand::SendToBack), 3);
    AccessScope scope;
    Token text, a, b;
    insideShapes[textShape(text)] = {shape(a), shape(b)};
    KUndo2Command parent;
    const int before = destructionCount;
    {
        KoSvgTextReorderShapeInsideCommand command(textShape(text),
                                                   {shape(b), shape(a)},
                                                   KoSvgTextReorderShapeInsideCommand::BringToFront,
                                                   &parent);
        QCOMPARE(parents.value(&command), &parent);
        QCOMPARE(trace, QStringList({"memento-read"}));
    }
    QCOMPARE(destructionCount, before + 1);
    QCOMPARE(a.value, uchar(0));
    QCOMPARE(b.value, uchar(0));
}

void KoSvgTextReorderShapeInsideCommandContractTest::bringToFrontMovesTargetsInInsideOrder()
{
    AccessScope scope;
    Token text, a, b, c, d;
    insideShapes[textShape(text)] = {shape(a), shape(b), shape(c), shape(d)};
    KoSvgTextReorderShapeInsideCommand command(textShape(text),
                                               {shape(d), shape(c)},
                                               KoSvgTextReorderShapeInsideCommand::BringToFront);
    trace.clear();
    command.redo();
    QCOMPARE(insideShapes[textShape(text)], QList<KoShape *>({shape(c), shape(d), shape(a), shape(b)}));
    QCOMPARE(moves.size(), 2);
    QCOMPARE(moves[0].shape, shape(c));
    QCOMPARE(moves[0].index, 0);
    QCOMPARE(moves[1].shape, shape(d));
    QCOMPARE(moves[1].index, 1);
}

void KoSvgTextReorderShapeInsideCommandContractTest::moveEarlierUsesFirstTargetBoundary()
{
    AccessScope scope;
    Token text, a, b, c;
    insideShapes[textShape(text)] = {shape(a), shape(b), shape(c)};
    KoSvgTextReorderShapeInsideCommand command(textShape(text),
                                               {shape(a), shape(b)},
                                               KoSvgTextReorderShapeInsideCommand::MoveEarlier);
    moves.clear();
    command.redo();
    QCOMPARE(moves.size(), 1);
    QCOMPARE(moves.constFirst().shape, shape(b));
    QCOMPARE(moves.constFirst().index, 0);
    QCOMPARE(insideShapes[textShape(text)], QList<KoShape *>({shape(b), shape(a), shape(c)}));
}

void KoSvgTextReorderShapeInsideCommandContractTest::laterAndBackDispatchInReverseOrder()
{
    AccessScope scope;
    Token text, a, b, c, d;
    insideShapes[textShape(text)] = {shape(a), shape(b), shape(c), shape(d)};
    KoSvgTextReorderShapeInsideCommand later(textShape(text),
                                             {shape(a), shape(b)},
                                             KoSvgTextReorderShapeInsideCommand::MoveLater);
    moves.clear();
    later.redo();
    QCOMPARE(moves.size(), 2);
    QCOMPARE(moves[0].shape, shape(b));
    QCOMPARE(moves[1].shape, shape(a));
    insideShapes[textShape(text)] = {shape(a), shape(b), shape(c), shape(d)};
    moves.clear();
    KoSvgTextReorderShapeInsideCommand back(textShape(text),
                                            {shape(a), shape(b)},
                                            KoSvgTextReorderShapeInsideCommand::SendToBack);
    moves.clear();
    back.redo();
    QCOMPARE(moves[0].shape, shape(b));
    QCOMPARE(moves[0].index, 3);
    QCOMPARE(moves[1].shape, shape(a));
    QCOMPARE(moves[1].index, 2);
}

void KoSvgTextReorderShapeInsideCommandContractTest::undoRestoresIndicesAndMemento()
{
    AccessScope scope;
    Token text, a, b, c, d;
    insideShapes[textShape(text)] = {shape(a), shape(b), shape(c), shape(d)};
    KoSvgTextReorderShapeInsideCommand command(textShape(text),
                                               {shape(c), shape(d)},
                                               KoSvgTextReorderShapeInsideCommand::BringToFront);
    command.redo();
    moves.clear();
    trace.clear();
    command.undo();
    QCOMPARE(moves.size(), 2);
    QCOMPARE(moves[0].shape, shape(d));
    QCOMPARE(moves[0].index, 3);
    QCOMPARE(moves[1].shape, shape(c));
    QCOMPARE(moves[1].index, 2);
    QVERIFY(trace.indexOf("memento-restore") > trace.lastIndexOf("move"));
}

QTEST_GUILESS_MAIN(KoSvgTextReorderShapeInsideCommandContractTest)
#include "KoSvgTextReorderShapeInsideCommandContractTest.moc"
