/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QRect>

class KisNode
{
public:
    void setDirty(const QRect &rect);
};

#include "commands_new/KisUpdateCommandEx.h"
#include "commands_new/kis_update_command.h"
#include "kis_image_interfaces.h"

#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

namespace
{

struct NodeToken {
    int references = 0;
    QVector<QRect> dirtyRects;
};

KisNode *nodePointer(NodeToken *token)
{
    return reinterpret_cast<KisNode *>(token);
}

NodeToken *nodeToken(KisNode *node)
{
    return reinterpret_cast<NodeToken *>(node);
}

struct RefreshCall {
    KisNode *node;
    QVector<QRect> rects;
    QRect cropRect;
    KisProjectionUpdateFlags flags;
};

class RecordingUpdatesFacade final : public KisUpdatesFacade
{
public:
    explicit RecordingUpdatesFacade(bool *destroyed = nullptr)
        : m_destroyed(destroyed)
    {
    }

    ~RecordingUpdatesFacade() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    void blockUpdates() override
    {
    }

    void unblockUpdates() override
    {
    }

    void disableUIUpdates() override
    {
    }

    QVector<QRect> enableUIUpdates() override
    {
        return {};
    }

    bool hasUpdatesRunning() const override
    {
        return false;
    }

    void notifyBatchUpdateStarted() override
    {
    }

    void notifyBatchUpdateEnded() override
    {
    }

    void notifyUIUpdateCompleted(const QRect &) override
    {
    }

    QRect bounds() const override
    {
        return imageBounds;
    }

    void disableDirtyRequests() override
    {
    }

    void enableDirtyRequests() override
    {
    }

    void refreshGraphAsync(KisNodeSP root,
                           const QVector<QRect> &rects,
                           const QRect &cropRect,
                           KisProjectionUpdateFlags flags) override
    {
        refreshCalls.append({root.data(), rects, cropRect, flags});
    }

    KisProjectionUpdatesFilterCookie addProjectionUpdatesFilter(KisProjectionUpdatesFilterSP) override
    {
        return nullptr;
    }

    KisProjectionUpdatesFilterSP removeProjectionUpdatesFilter(KisProjectionUpdatesFilterCookie) override
    {
        return {};
    }

    KisProjectionUpdatesFilterCookie currentProjectionUpdatesFilter() const override
    {
        return nullptr;
    }

    QRect imageBounds{0, 0, 512, 384};
    QVector<RefreshCall> refreshCalls;

private:
    bool *m_destroyed;
};

KisBatchNodeUpdateSP
makeBatch(NodeToken *firstNode, const QRect &firstRect, NodeToken *secondNode, const QRect &secondRect)
{
    KisBatchNodeUpdateSP batch = KisBatchNodeUpdateSP::create();
    batch->emplace_back(KisNodeSP(nodePointer(firstNode)), firstRect);
    batch->emplace_back(KisNodeSP(nodePointer(secondNode)), secondRect);
    return batch;
}

void compareRefresh(const RecordingUpdatesFacade &facade, int index, KisNode *node, const QRect &rect)
{
    const RefreshCall &call = facade.refreshCalls.at(index);
    QCOMPARE(call.node, node);
    QCOMPARE(call.rects, QVector<QRect>({rect}));
    QCOMPARE(call.cropRect, facade.imageBounds);
    QVERIFY(call.flags == KisProjectionUpdateFlag::None);
}

} // namespace

void kisSharedPtrAddReference(KisNode *node)
{
    ++nodeToken(node)->references;
}

bool kisSharedPtrRelease(KisNode *node)
{
    --nodeToken(node)->references;
    return true;
}

void KisNode::setDirty(const QRect &rect)
{
    nodeToken(this)->dirtyRects.append(rect);
}

namespace KisCommandUtils
{

FlipFlopCommand::FlipFlopCommand(State initialState, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_currentState(initialState)
{
}

void FlipFlopCommand::redo()
{
    if (m_currentState == INITIALIZING) {
        partA();
    } else {
        partB();
    }

    m_firstRedo = false;
}

void FlipFlopCommand::undo()
{
    if (m_currentState == FINALIZING) {
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

KisUpdatesFacade::~KisUpdatesFacade() = default;

void KisUpdatesFacade::refreshGraphAsync(KisNodeSP root, KisProjectionUpdateFlags flags)
{
    refreshGraphAsync(root, bounds(), bounds(), flags);
}

void KisUpdatesFacade::refreshGraphAsync(KisNodeSP root, const QRect &rect, KisProjectionUpdateFlags flags)
{
    refreshGraphAsync(root, rect, bounds(), flags);
}

void KisUpdatesFacade::refreshGraphAsync(KisNodeSP root,
                                         const QRect &rect,
                                         const QRect &cropRect,
                                         KisProjectionUpdateFlags flags)
{
    refreshGraphAsync(root, QVector<QRect>({rect}), cropRect, flags);
}

class KisUpdateCommandExContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void finalizingRedoDispatchesUpdatesInInsertionOrder();
    void initializingUndoDispatchesUpdatesInInsertionOrder();
    void cookieBlocksDirectPartBUntilItExpires();
    void commandOwnsBatchAndBorrowsFacade();
    void valueRectUpdateCopiesRectAndRunsOnRedoAndUndo();
    void sharedRectUpdateReadsCurrentRectOnRedoAndUndo();
    void fullRefreshRoutesThroughFacadeOnRedoAndUndo();
    void updateCommandOwnsNodeAndSharedRectAndBorrowsFacade();
};

void KisUpdateCommandExContractTest::finalizingRedoDispatchesUpdatesInInsertionOrder()
{
    NodeToken firstNode;
    NodeToken secondNode;
    const QRect firstRect(3, 5, 7, 11);
    const QRect secondRect(13, 17, 19, 23);
    RecordingUpdatesFacade facade;
    KisUpdateCommandEx command(makeBatch(&firstNode, firstRect, &secondNode, secondRect),
                               &facade,
                               KisUpdateCommandEx::FINALIZING);

    command.redo();
    QCOMPARE(facade.refreshCalls.size(), 2);
    compareRefresh(facade, 0, nodePointer(&firstNode), firstRect);
    compareRefresh(facade, 1, nodePointer(&secondNode), secondRect);

    command.undo();
    QCOMPARE(facade.refreshCalls.size(), 2);

    command.redo();
    QCOMPARE(facade.refreshCalls.size(), 4);
    compareRefresh(facade, 2, nodePointer(&firstNode), firstRect);
    compareRefresh(facade, 3, nodePointer(&secondNode), secondRect);
}

void KisUpdateCommandExContractTest::initializingUndoDispatchesUpdatesInInsertionOrder()
{
    NodeToken firstNode;
    NodeToken secondNode;
    const QRect firstRect(29, 31, 37, 41);
    const QRect secondRect(43, 47, 53, 59);
    RecordingUpdatesFacade facade;
    KisUpdateCommandEx command(makeBatch(&firstNode, firstRect, &secondNode, secondRect),
                               &facade,
                               KisUpdateCommandEx::INITIALIZING);

    command.redo();
    QVERIFY(facade.refreshCalls.isEmpty());

    command.undo();
    QCOMPARE(facade.refreshCalls.size(), 2);
    compareRefresh(facade, 0, nodePointer(&firstNode), firstRect);
    compareRefresh(facade, 1, nodePointer(&secondNode), secondRect);

    command.redo();
    QCOMPARE(facade.refreshCalls.size(), 2);
}

void KisUpdateCommandExContractTest::cookieBlocksDirectPartBUntilItExpires()
{
    NodeToken firstNode;
    NodeToken secondNode;
    const QRect firstRect(61, 67, 71, 73);
    const QRect secondRect(79, 83, 89, 97);
    RecordingUpdatesFacade facade;
    QSharedPointer<boost::none_t> cookie(new boost::none_t(boost::none));
    KisUpdateCommandEx command(makeBatch(&firstNode, firstRect, &secondNode, secondRect),
                               &facade,
                               KisUpdateCommandEx::FINALIZING,
                               cookie.toWeakRef());

    command.partB();
    QVERIFY(facade.refreshCalls.isEmpty());

    cookie.clear();
    command.partB();
    QCOMPARE(facade.refreshCalls.size(), 2);
    compareRefresh(facade, 0, nodePointer(&firstNode), firstRect);
    compareRefresh(facade, 1, nodePointer(&secondNode), secondRect);
}

void KisUpdateCommandExContractTest::commandOwnsBatchAndBorrowsFacade()
{
    NodeToken firstNode;
    NodeToken secondNode;
    bool facadeDestroyed = false;
    auto *facade = new RecordingUpdatesFacade(&facadeDestroyed);
    KisBatchNodeUpdateSP batch = makeBatch(&firstNode, QRect(1, 2, 3, 4), &secondNode, QRect(5, 6, 7, 8));
    const KisBatchNodeUpdateWSP weakBatch(batch);

    KUndo2Command *command = new KisUpdateCommandEx(batch, facade, KisUpdateCommandEx::FINALIZING);
    batch.clear();

    QVERIFY(!weakBatch.toStrongRef().isNull());
    QCOMPARE(firstNode.references, 1);
    QCOMPARE(secondNode.references, 1);

    delete command;
    QVERIFY(weakBatch.toStrongRef().isNull());
    QCOMPARE(firstNode.references, 0);
    QCOMPARE(secondNode.references, 0);
    QVERIFY(!facadeDestroyed);

    delete facade;
    QVERIFY(facadeDestroyed);
}

void KisUpdateCommandExContractTest::valueRectUpdateCopiesRectAndRunsOnRedoAndUndo()
{
    NodeToken node;
    QRect dirtyRect(3, 5, 7, 11);
    const QRect capturedRect = dirtyRect;
    RecordingUpdatesFacade facade;
    KisUpdateCommand command(KisNodeSP(nodePointer(&node)), dirtyRect, &facade);

    dirtyRect = QRect(13, 17, 19, 23);

    QCOMPARE(command.text().toString(), QStringLiteral("UPDATE_COMMAND"));
    command.redo();
    command.undo();

    QCOMPARE(node.dirtyRects, QVector<QRect>({capturedRect, capturedRect}));
    QVERIFY(facade.refreshCalls.isEmpty());
}

void KisUpdateCommandExContractTest::sharedRectUpdateReadsCurrentRectOnRedoAndUndo()
{
    NodeToken node;
    const QRect redoRect(29, 31, 37, 41);
    const QRect undoRect(43, 47, 53, 59);
    QSharedPointer<QRect> dirtyRect = QSharedPointer<QRect>::create(redoRect);
    RecordingUpdatesFacade facade;
    KisUpdateCommand command(KisNodeSP(nodePointer(&node)), dirtyRect, &facade);

    command.redo();
    *dirtyRect = undoRect;
    command.undo();

    QCOMPARE(node.dirtyRects, QVector<QRect>({redoRect, undoRect}));
    QVERIFY(facade.refreshCalls.isEmpty());
}

void KisUpdateCommandExContractTest::fullRefreshRoutesThroughFacadeOnRedoAndUndo()
{
    NodeToken node;
    QRect dirtyRect(61, 67, 71, 73);
    const QRect capturedRect = dirtyRect;
    RecordingUpdatesFacade facade;
    KisUpdateCommand command(KisNodeSP(nodePointer(&node)), dirtyRect, &facade, true);

    dirtyRect = QRect(79, 83, 89, 97);

    command.redo();
    command.undo();

    QVERIFY(node.dirtyRects.isEmpty());
    QCOMPARE(facade.refreshCalls.size(), 2);
    compareRefresh(facade, 0, nodePointer(&node), capturedRect);
    compareRefresh(facade, 1, nodePointer(&node), capturedRect);
}

void KisUpdateCommandExContractTest::updateCommandOwnsNodeAndSharedRectAndBorrowsFacade()
{
    NodeToken node;
    bool facadeDestroyed = false;
    auto *facade = new RecordingUpdatesFacade(&facadeDestroyed);
    KisNodeSP externalNode(nodePointer(&node));
    QSharedPointer<QRect> sharedRect = QSharedPointer<QRect>::create(101, 103, 107, 109);
    const QWeakPointer<QRect> weakRect(sharedRect);

    QCOMPARE(node.references, 1);
    KUndo2Command *command = new KisUpdateCommand(externalNode, sharedRect, facade);
    QCOMPARE(node.references, 2);

    externalNode.clear();
    sharedRect.clear();
    QCOMPARE(node.references, 1);
    QVERIFY(!weakRect.isNull());

    delete command;
    QCOMPARE(node.references, 0);
    QVERIFY(weakRect.isNull());
    QVERIFY(!facadeDestroyed);

    delete facade;
    QVERIFY(facadeDestroyed);
}

QTEST_GUILESS_MAIN(KisUpdateCommandExContractTest)

#include "KisUpdateCommandExContractTest.moc"
