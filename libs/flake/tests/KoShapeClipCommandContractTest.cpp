/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include <KoClipPath.h>
#include <KoPathShape.h>
#include <KoShapeClipCommand.h>
#include <KoShapeContainer.h>
#include <QHash>
#include <QTest>
#include <QTime>
#include <cstddef>

class KoClipPath::Private : public QSharedData
{
};

namespace
{
QStringList trace;
QList<KoClipPath *> deletedClips;
QList<KoClipPath *> generatedClips;
QList<QPair<KoShape *, KoClipPath *>> setCalls;
QList<QPair<KoShapeContainer *, KoPathShape *>> removeCalls;
QList<QPair<KoShapeContainer *, KoPathShape *>> addCalls;
QHash<KoShape *, KoClipPath *> clips;
QHash<KoPathShape *, KoShapeContainer *> parentsMap;
struct alignas(std::max_align_t) Token {
    char c;
};
template<class T>
T *tok(Token &v)
{
    return reinterpret_cast<T *>(&v);
}
} // namespace
KUndo2MagicString::KUndo2MagicString() = default;
KUndo2MagicString::KUndo2MagicString(const QString &t)
    : m_text(t)
{
}
QString KUndo2MagicString::toString() const
{
    return m_text;
}
QString KUndo2MagicString::toSecondaryString() const
{
    return {};
}
bool KUndo2MagicString::isEmpty() const
{
    return m_text.isEmpty();
}
bool KUndo2MagicString::operator==(const KUndo2MagicString &r) const
{
    return m_text == r.m_text;
}
KUndo2Command::KUndo2Command(KUndo2Command *)
{
}
KUndo2Command::KUndo2Command(const KUndo2MagicString &, KUndo2Command *)
{
}
KUndo2Command::~KUndo2Command() = default;
void KUndo2Command::redo()
{
    trace << "base-redo";
}
void KUndo2Command::undo()
{
    trace << "base-undo";
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
    return {};
}
bool KUndo2Command::isMerged() const
{
    return false;
}
void KUndo2Command::undoMergedCommands()
{
}
void KUndo2Command::redoMergedCommands()
{
}
KoClipPath *KoShape::clipPath() const
{
    qFatal("product clip reader");
    return nullptr;
}
void KoShape::setClipPath(KoClipPath *)
{
    qFatal("product clip setter");
}
void KoShape::update() const
{
    qFatal("product updater");
}
KoShapeContainer *KoShape::parent() const
{
    qFatal("product parent reader");
    return nullptr;
}
void KoShapeContainer::removeShape(KoShape *)
{
    qFatal("product remover");
}
void KoShapeContainer::addShape(KoShape *)
{
    qFatal("product adder");
}
KoClipPath::KoClipPath(QList<KoShape *>, KoFlake::CoordinateSystem)
{
    qFatal("product clip factory");
}
KoClipPath::~KoClipPath()
{
    qFatal("product clip deleter");
}
namespace KoShapeClipCommandTesting
{
using CR = KoClipPath *(*)(KoShape *);
using PR = KoShapeContainer *(*)(KoPathShape *);
using F = KoClipPath *(*)(const QList<KoPathShape *> &);
using S = void (*)(KoShape *, KoClipPath *);
using U = void (*)(KoShape *);
using P = void (*)(KoShapeContainer *, KoPathShape *);
using D = void (*)(KoClipPath *);
void setAccess(CR, PR, F, S, U, P, P, D);
void resetAccess();
} // namespace KoShapeClipCommandTesting
namespace
{
int serial;
KoClipPath *cr(KoShape *s)
{
    trace << "read-clip";
    return clips.value(s);
}
KoShapeContainer *pr(KoPathShape *s)
{
    trace << "read-parent";
    return parentsMap.value(s);
}
KoClipPath *factory(const QList<KoPathShape *> &)
{
    trace << "factory";
    static Token n[20];
    KoClipPath *result = tok<KoClipPath>(n[serial++]);
    generatedClips << result;
    return result;
}
void setter(KoShape *shape, KoClipPath *clip)
{
    trace << "set";
    setCalls << qMakePair(shape, clip);
}
void updater(KoShape *)
{
    trace << "update";
}
void remover(KoShapeContainer *parent, KoPathShape *path)
{
    trace << "remove";
    removeCalls << qMakePair(parent, path);
}
void adder(KoShapeContainer *parent, KoPathShape *path)
{
    trace << "add";
    addCalls << qMakePair(parent, path);
}
void deleter(KoClipPath *p)
{
    trace << "delete";
    deletedClips << p;
}
struct Scope {
    Scope()
    {
        trace.clear();
        deletedClips.clear();
        generatedClips.clear();
        setCalls.clear();
        removeCalls.clear();
        addCalls.clear();
        clips.clear();
        parentsMap.clear();
        serial = 0;
        KoShapeClipCommandTesting::setAccess(cr, pr, factory, setter, updater, remover, adder, deleter);
    }
    ~Scope()
    {
        KoShapeClipCommandTesting::resetAccess();
    }
};
} // namespace
class KoShapeClipCommandContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void constructorsCaptureClipAndParentsAndAllocatePerTarget();
    void redoInstallsNewClipsThenDetachesSources();
    void undoRestoresOldClipsThenReattachesSources();
    void destructionDeletesOnlyCurrentlyOwnedClipSet();
    void nullParentsAndEmptyListsAreStable();
};
void KoShapeClipCommandContractTest::constructorsCaptureClipAndParentsAndAllocatePerTarget()
{
    Scope x;
    Token a, b, c, d, p, o1, o2;
    clips[tok<KoShape>(a)] = tok<KoClipPath>(o1);
    clips[tok<KoShape>(b)] = tok<KoClipPath>(o2);
    parentsMap[tok<KoPathShape>(c)] = tok<KoShapeContainer>(p);
    KoShapeClipCommand cmd(nullptr,
                           QList<KoShape *>{tok<KoShape>(a), tok<KoShape>(b)},
                           QList<KoPathShape *>{tok<KoPathShape>(c), tok<KoPathShape>(d)});
    QCOMPARE(trace, QStringList({"read-clip", "factory", "read-clip", "factory", "read-parent", "read-parent"}));
    QCOMPARE(generatedClips.size(), 2);
    QVERIFY(generatedClips.at(0) != generatedClips.at(1));
    cmd.redo();
    QCOMPARE(setCalls,
             (QList<QPair<KoShape *, KoClipPath *>>{{tok<KoShape>(a), generatedClips.at(0)},
                                                    {tok<KoShape>(b), generatedClips.at(1)}}));
}
void KoShapeClipCommandContractTest::redoInstallsNewClipsThenDetachesSources()
{
    Scope x;
    Token a, c, p;
    parentsMap[tok<KoPathShape>(c)] = tok<KoShapeContainer>(p);
    KoShapeClipCommand cmd(nullptr, tok<KoShape>(a), {tok<KoPathShape>(c)});
    KoClipPath *newClip = generatedClips.constFirst();
    trace.clear();
    cmd.redo();
    QCOMPARE(trace, QStringList({"set", "update", "remove", "base-redo"}));
    QCOMPARE(setCalls, (QList<QPair<KoShape *, KoClipPath *>>{{tok<KoShape>(a), newClip}}));
    QCOMPARE(removeCalls,
             (QList<QPair<KoShapeContainer *, KoPathShape *>>{{tok<KoShapeContainer>(p), tok<KoPathShape>(c)}}));
}
void KoShapeClipCommandContractTest::undoRestoresOldClipsThenReattachesSources()
{
    Scope x;
    Token a, c, p, old;
    clips[tok<KoShape>(a)] = tok<KoClipPath>(old);
    parentsMap[tok<KoPathShape>(c)] = tok<KoShapeContainer>(p);
    KoShapeClipCommand cmd(nullptr, tok<KoShape>(a), {tok<KoPathShape>(c)});
    cmd.redo();
    setCalls.clear();
    trace.clear();
    cmd.undo();
    QCOMPARE(trace, QStringList({"base-undo", "set", "update", "add"}));
    QCOMPARE(setCalls, (QList<QPair<KoShape *, KoClipPath *>>{{tok<KoShape>(a), tok<KoClipPath>(old)}}));
    QCOMPARE(addCalls,
             (QList<QPair<KoShapeContainer *, KoPathShape *>>{{tok<KoShapeContainer>(p), tok<KoPathShape>(c)}}));
}
void KoShapeClipCommandContractTest::destructionDeletesOnlyCurrentlyOwnedClipSet()
{
    Scope x;
    Token a, o;
    clips[tok<KoShape>(a)] = tok<KoClipPath>(o);
    {
        KoShapeClipCommand cmd(nullptr, tok<KoShape>(a), {});
    }
    QCOMPARE(deletedClips.size(), 1);
    QVERIFY(deletedClips.first() != tok<KoClipPath>(o));
    deletedClips.clear();
    {
        KoShapeClipCommand cmd(nullptr, tok<KoShape>(a), {});
        cmd.redo();
    }
    QCOMPARE(deletedClips, QList<KoClipPath *>{tok<KoClipPath>(o)});
    deletedClips.clear();
    {
        KoShapeClipCommand cmd(nullptr, tok<KoShape>(a), {});
        cmd.redo();
        cmd.undo();
    }
    QCOMPARE(deletedClips.size(), 1);
    QVERIFY(deletedClips.first() != tok<KoClipPath>(o));
}
void KoShapeClipCommandContractTest::nullParentsAndEmptyListsAreStable()
{
    Scope x;
    {
        KoShapeClipCommand cmd(nullptr, QList<KoShape *>{}, QList<KoPathShape *>{});
        trace.clear();
        cmd.redo();
        cmd.undo();
        QCOMPARE(trace, QStringList({"base-redo", "base-undo"}));
    }
    Token shapeToken, pathToken;
    trace.clear();
    KoShapeClipCommand cmd(nullptr, tok<KoShape>(shapeToken), {tok<KoPathShape>(pathToken)});
    trace.clear();
    cmd.redo();
    cmd.undo();
    QCOMPARE(trace, QStringList({"set", "update", "base-redo", "base-undo", "set", "update"}));
}
QTEST_GUILESS_MAIN(KoShapeClipCommandContractTest)
#include "KoShapeClipCommandContractTest.moc"
