/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>
#include <KoShapeBulkActionLock.h>
#include <KoShapeResizeCommand.h>
#include <kis_command_ids.h>

#include <QHash>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace
{
QHash<const KUndo2Command *, KUndo2Command *> parents;
QStringList trace;
int destructions = 0;
struct ShapeState {
    QSizeF size;
    QTransform transform;
};
QHash<const KoShape *, ShapeState> states;
struct ResizeCall {
    KoShape *shape;
    qreal x;
    qreal y;
    QPointF point;
    bool global;
    bool post;
    QTransform covering;
};
QList<ResizeCall> resizeCalls;
struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};
KoShape *shape(ShapeToken &token)
{
    return reinterpret_cast<KoShape *>(&token);
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
    ++destructions;
    parents.remove(this);
}
void KUndo2Command::undo()
{
}
void KUndo2Command::redo()
{
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

namespace KisCommandUtils
{
SkipFirstRedoBase::SkipFirstRedoBase(bool skip, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_firstRedo(skip)
{
}
SkipFirstRedoBase::SkipFirstRedoBase(bool skip, const KUndo2MagicString &text, KUndo2Command *parent)
    : KUndo2Command(text, parent)
    , m_firstRedo(skip)
{
}
void SkipFirstRedoBase::redo()
{
    if (m_firstRedo)
        m_firstRedo = false;
    else
        redoImpl();
}
void SkipFirstRedoBase::undo()
{
    undoImpl();
}
void SkipFirstRedoBase::setSkipOneRedo(bool value)
{
    m_firstRedo = value;
}
} // namespace KisCommandUtils

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

QTransform KoShape::transformation() const
{
    qFatal("the production shape-state reader must not run in the resize command contract");
    return {};
}

void KoShape::setTransformation(const QTransform &)
{
    qFatal("the production shape-state restorer must not run in the resize command contract");
}

void KoFlake::resizeShapeCommon(KoShape *, qreal, qreal, const QPointF &, bool, bool, const QTransform &)
{
    qFatal("the production resize implementation must not run in the resize command contract");
}

Qt::Orientation KoFlake::significantScaleOrientation(qreal, qreal)
{
    qFatal("the production scale-orientation reader must not run in the resize command contract");
    return Qt::Horizontal;
}

namespace KoShapeResizeCommandTesting
{
using StateReader = void (*)(const KoShape *, QSizeF *, QTransform *);
using Resize = void (*)(KoShape *, qreal, qreal, const QPointF &, bool, bool, const QTransform &);
using Restore = void (*)(KoShape *, const QSizeF &, const QTransform &);
using Orientation = Qt::Orientation (*)(qreal, qreal);
void setShapeAccessForTesting(StateReader, Resize, Restore, Orientation);
void resetShapeAccessForTesting();
} // namespace KoShapeResizeCommandTesting

namespace
{
void readState(const KoShape *shape, QSizeF *size, QTransform *transform)
{
    trace << QStringLiteral("read");
    *size = states[shape].size;
    *transform = states[shape].transform;
}
void resizeShape(KoShape *shape,
                 qreal x,
                 qreal y,
                 const QPointF &point,
                 bool global,
                 bool post,
                 const QTransform &covering)
{
    trace << QStringLiteral("resize");
    resizeCalls << ResizeCall{shape, x, y, point, global, post, covering};
}
void restoreShape(KoShape *shape, const QSizeF &size, const QTransform &transform)
{
    trace << QStringLiteral("restore");
    states[shape] = {size, transform};
}
Qt::Orientation orientation(qreal x, qreal y)
{
    return qAbs(x) >= qAbs(y) ? Qt::Horizontal : Qt::Vertical;
}
class AccessScope
{
public:
    AccessScope()
    {
        trace.clear();
        resizeCalls.clear();
        states.clear();
        KoShapeResizeCommandTesting::setShapeAccessForTesting(readState, resizeShape, restoreShape, orientation);
    }
    ~AccessScope()
    {
        KoShapeResizeCommandTesting::resetShapeAccessForTesting();
    }
};
} // namespace

class KoShapeResizeCommandContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void capturesOriginalStateParentAndBorrowedLifetime();
    void redoAndUndoUseSingleLocksAndOrderedStates();
    void replaceRestoresThenAppliesInsideOneLock();
    void mergeRequiresCompatibleModeAndMultipliesScales();
    void reportsResizeIdentity();
};

void KoShapeResizeCommandContractTest::capturesOriginalStateParentAndBorrowedLifetime()
{
    AccessScope scope;
    ShapeToken a, b;
    KUndo2Command parent;
    states[shape(a)] = {{2, 3}, QTransform::fromTranslate(1, 2)};
    states[shape(b)] = {{4, 5}, QTransform::fromScale(2, 3)};
    const int before = destructions;
    {
        KoShapeResizeCommand command({shape(a), shape(b)}, 2, 3, {4, 5}, true, false, {}, &parent);
        QCOMPARE(parents.value(&command), &parent);
        QCOMPARE(trace, QStringList({"read", "read"}));
    }
    QCOMPARE(destructions, before + 1);
    QCOMPARE(a.value, static_cast<unsigned char>(0));
    QCOMPARE(b.value, static_cast<unsigned char>(0));
}

void KoShapeResizeCommandContractTest::redoAndUndoUseSingleLocksAndOrderedStates()
{
    AccessScope scope;
    ShapeToken a;
    states[shape(a)] = {{2, 3}, QTransform::fromTranslate(1, 2)};
    KoShapeResizeCommand command({shape(a)}, 2, 3, {4, 5}, true, false, QTransform::fromScale(7, 8));
    trace.clear();
    command.redo();
    command.undo();
    QCOMPARE(trace,
             QStringList({"lock-create",
                          "lock",
                          "resize",
                          "unlock",
                          "bulk-update",
                          "lock-destroy",
                          "lock-create",
                          "lock",
                          "restore",
                          "unlock",
                          "bulk-update",
                          "lock-destroy"}));
    QCOMPARE(resizeCalls.size(), 1);
    QCOMPARE(resizeCalls[0].x, 2.0);
    QCOMPARE(resizeCalls[0].y, 3.0);
    QCOMPARE(resizeCalls[0].point, QPointF(4, 5));
    QVERIFY(resizeCalls[0].global);
    QVERIFY(!resizeCalls[0].post);
    QCOMPARE(states[shape(a)].size, QSizeF(2, 3));
}

void KoShapeResizeCommandContractTest::replaceRestoresThenAppliesInsideOneLock()
{
    AccessScope scope;
    ShapeToken a;
    states[shape(a)] = {{9, 10}, QTransform::fromTranslate(3, 4)};
    KoShapeResizeCommand command({shape(a)}, 2, 3, {4, 5}, false, true, QTransform::fromScale(6, 7));
    trace.clear();
    command.replaceResizeAction(8, 9, {10, 11});
    QCOMPARE(trace, QStringList({"lock-create", "lock", "restore", "resize", "unlock", "bulk-update", "lock-destroy"}));
    QCOMPARE(resizeCalls[0].x, 8.0);
    QCOMPARE(resizeCalls[0].y, 9.0);
    QCOMPARE(resizeCalls[0].point, QPointF(10, 11));
    QVERIFY(resizeCalls[0].post);
    QCOMPARE(resizeCalls[0].covering, QTransform::fromScale(6, 7));
}

void KoShapeResizeCommandContractTest::mergeRequiresCompatibleModeAndMultipliesScales()
{
    AccessScope scope;
    ShapeToken a, b;
    states[shape(a)] = {};
    states[shape(b)] = {};
    KoShapeResizeCommand command({shape(a)}, 2, 3, {4, 5}, true, false, {});
    KoShapeResizeCommand compatible({shape(a)}, 4, 5, {4, 5}, true, false, QTransform::fromScale(9, 9));
    KoShapeResizeCommand differentOrder({shape(b)}, 4, 5, {4, 5}, true, false, {});
    KoShapeResizeCommand differentPoint({shape(a)}, 4, 5, {6, 5}, true, false, {});
    KoShapeResizeCommand differentOrientation({shape(a)}, 9, 1, {4, 5}, true, false, {});
    QVERIFY(!command.mergeWith(&differentOrder));
    QVERIFY(!command.mergeWith(&differentPoint));
    QVERIFY(!command.mergeWith(&differentOrientation));
    QVERIFY(command.mergeWith(&compatible));
    resizeCalls.clear();
    command.redo();
    QCOMPARE(resizeCalls[0].x, 8.0);
    QCOMPARE(resizeCalls[0].y, 15.0);
}

void KoShapeResizeCommandContractTest::reportsResizeIdentity()
{
    AccessScope scope;
    KoShapeResizeCommand command({}, 1, 1, {}, false, false, {});
    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::ResizeShapeId));
}

QTEST_GUILESS_MAIN(KoShapeResizeCommandContractTest)
#include "KoShapeResizeCommandContractTest.moc"
