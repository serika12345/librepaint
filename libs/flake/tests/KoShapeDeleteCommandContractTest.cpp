/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include <KoShape.h>
#include <KoShapeContainer.h>
#include <KoShapeDeleteCommand.h>
#include <QHash>
#include <QTest>
#include <QTime>
#include <cstddef>

namespace
{
QHash<const KUndo2Command *, KUndo2Command *> parents;
QStringList trace;
int baseDeaths = 0;
QHash<const KoShape *, KoShapeContainer *> shapeParents;
QList<KoShape *> deleted;
struct alignas(std::max_align_t) Token {
    unsigned char value{0};
};
KoShape *shape(Token &t)
{
    return reinterpret_cast<KoShape *>(&t);
}
KoShapeContainer *container(Token &t)
{
    return reinterpret_cast<KoShapeContainer *>(&t);
}
} // namespace
KUndo2MagicString::KUndo2MagicString() = default;
KUndo2MagicString::KUndo2MagicString(const QString &t)
    : m_text(t)
{
}
QString KUndo2MagicString::toString() const
{
    return m_text.section('\n', 0, 0);
}
QString KUndo2MagicString::toSecondaryString() const
{
    return m_text.section('\n', 1);
}
bool KUndo2MagicString::isEmpty() const
{
    return m_text.isEmpty();
}
bool KUndo2MagicString::operator==(const KUndo2MagicString &r) const
{
    return m_text == r.m_text;
}
KUndo2Command::KUndo2Command(KUndo2Command *p)
{
    parents[this] = p;
}
KUndo2Command::KUndo2Command(const KUndo2MagicString &, KUndo2Command *p)
{
    parents[this] = p;
}
KUndo2Command::~KUndo2Command()
{
    ++baseDeaths;
    parents.remove(this);
}
void KUndo2Command::undo()
{
    trace << "base-undo";
}
void KUndo2Command::redo()
{
    trace << "base-redo";
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
void KUndo2Command::setTimedID(int v)
{
    m_timedID = v;
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
void KUndo2Command::setTime(const QTime &t)
{
    m_timeOfCreation = t;
}
QTime KUndo2Command::time() const
{
    return m_timeOfCreation;
}
void KUndo2Command::setEndTime(const QTime &t)
{
    m_endOfCommand = t;
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

KoShapeContainer *KoShape::parent() const
{
    qFatal("production parent reader reached");
    return nullptr;
}
void KoShapeContainer::removeShape(KoShape *)
{
    qFatal("production parent remover reached");
}
void KoShapeContainer::addShape(KoShape *)
{
    qFatal("production parent adder reached");
}

namespace KoShapeDeleteCommandTesting
{
using Reader = KoShapeContainer *(*)(const KoShape *);
using Operation = void (*)(KoShapeContainer *, KoShape *);
using Deleter = void (*)(KoShape *);
void setShapeAccessForTesting(Reader, Operation, Operation, Deleter);
void resetShapeAccessForTesting();
} // namespace KoShapeDeleteCommandTesting
namespace
{
KoShapeContainer *readParent(const KoShape *s)
{
    trace << "parent";
    return shapeParents.value(s);
}
void removeShape(KoShapeContainer *, KoShape *)
{
    trace << "remove";
}
void addShape(KoShapeContainer *, KoShape *)
{
    trace << "add";
}
void deleteShape(KoShape *s)
{
    trace << "delete";
    deleted << s;
}
class Scope
{
public:
    Scope()
    {
        trace.clear();
        deleted.clear();
        shapeParents.clear();
        KoShapeDeleteCommandTesting::setShapeAccessForTesting(readParent, removeShape, addShape, deleteShape);
    }
    ~Scope()
    {
        KoShapeDeleteCommandTesting::resetShapeAccessForTesting();
    }
};
} // namespace

class KoShapeDeleteCommandContractTest : public QObject
{
    Q_OBJECT private Q_SLOTS : void constructorsCaptureParentsAndBorrow();
    void nullControllerDoesNothing();
    void redoRemovesAndArmsDeletion();
    void undoRestoresAndReturnsBorrowing();
    void executedDestructionDeletesOnce();
};
void KoShapeDeleteCommandContractTest::constructorsCaptureParentsAndBorrow()
{
    Scope s;
    Token a, b, p;
    shapeParents[shape(a)] = container(p);
    shapeParents[shape(b)] = nullptr;
    KUndo2Command parent;
    int n = baseDeaths;
    {
        KoShapeDeleteCommand c(nullptr, QList<KoShape *>{shape(a), shape(b)}, &parent);
        QCOMPARE(parents.value(&c), &parent);
        QCOMPARE(trace, QStringList({"parent", "parent"}));
    }
    QCOMPARE(baseDeaths, n + 1);
    QVERIFY(deleted.isEmpty());
}
void KoShapeDeleteCommandContractTest::nullControllerDoesNothing()
{
    Scope s;
    Token a, p;
    shapeParents[shape(a)] = container(p);
    KoShapeDeleteCommand c(nullptr, shape(a));
    trace.clear();
    c.redo();
    c.undo();
    QCOMPARE(trace, QStringList({"base-redo", "base-undo"}));
}
void KoShapeDeleteCommandContractTest::redoRemovesAndArmsDeletion()
{
    Scope s;
    Token a, b, p;
    shapeParents[shape(a)] = container(p);
    shapeParents[shape(b)] = nullptr;
    {
        KoShapeDeleteCommand c(reinterpret_cast<KoShapeControllerBase *>(1), QList<KoShape *>{shape(a), shape(b)});
        trace.clear();
        c.redo();
        QCOMPARE(trace, QStringList({"base-redo", "remove"}));
        c.undo();
    }
}
void KoShapeDeleteCommandContractTest::undoRestoresAndReturnsBorrowing()
{
    Scope s;
    Token a, p;
    shapeParents[shape(a)] = container(p);
    {
        KoShapeDeleteCommand c(reinterpret_cast<KoShapeControllerBase *>(1), shape(a));
        c.redo();
        trace.clear();
        c.undo();
        QCOMPARE(trace, QStringList({"base-undo", "add"}));
    }
    QVERIFY(deleted.isEmpty());
}
void KoShapeDeleteCommandContractTest::executedDestructionDeletesOnce()
{
    Scope s;
    Token a, b;
    shapeParents[shape(a)] = nullptr;
    shapeParents[shape(b)] = nullptr;
    {
        KoShapeDeleteCommand c(reinterpret_cast<KoShapeControllerBase *>(1), QList<KoShape *>{shape(a), shape(b)});
        c.redo();
    }
    QCOMPARE(deleted, QList<KoShape *>({shape(a), shape(b)}));
}
QTEST_GUILESS_MAIN(KoShapeDeleteCommandContractTest)
#include "KoShapeDeleteCommandContractTest.moc"
