/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_graph_listener.h"

#include "kis_time_span.h"

#include <QRect>
#include <QTest>
#include <QVector>

#include <memory>

namespace
{
enum class ListenerCall {
    None,
    AboutToAdd,
    Added,
    AboutToRemove,
    Removed,
    AboutToMove,
    Moved,
    NodeChanged,
    NodeCollapsedChanged,
    InvalidateAllFrames,
    SelectionChanged,
    ProjectionUpdate,
    InvalidateFrames,
    TimeSwitch,
    KeyframeAdded,
    KeyframeAboutToBeRemoved
};

class ListenerProbe : public KisNodeGraphListener
{
public:
    explicit ListenerProbe(KisNode *overlayNode = nullptr, bool *destroyed = nullptr)
        : overlayNode(overlayNode)
        , destroyed(destroyed)
    {
    }

    ~ListenerProbe() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    void aboutToAddANode(KisNode *parent, int index) override
    {
        call = ListenerCall::AboutToAdd;
        node = parent;
        firstIndex = index;
    }

    void nodeHasBeenAdded(KisNode *parent, int index, KisNodeAdditionFlags flags) override
    {
        call = ListenerCall::Added;
        node = parent;
        firstIndex = index;
        additionFlags = flags;
    }

    void aboutToRemoveANode(KisNode *parent, int index) override
    {
        call = ListenerCall::AboutToRemove;
        node = parent;
        firstIndex = index;
    }

    void nodeHasBeenRemoved(KisNode *parent, int index) override
    {
        call = ListenerCall::Removed;
        node = parent;
        firstIndex = index;
    }

    void aboutToMoveNode(KisNode *movedNode, int oldIndex, int newIndex) override
    {
        call = ListenerCall::AboutToMove;
        node = movedNode;
        firstIndex = oldIndex;
        secondIndex = newIndex;
    }

    void nodeHasBeenMoved(KisNode *movedNode, int oldIndex, int newIndex) override
    {
        call = ListenerCall::Moved;
        node = movedNode;
        firstIndex = oldIndex;
        secondIndex = newIndex;
    }

    void nodeChanged(KisNode *changedNode) override
    {
        call = ListenerCall::NodeChanged;
        node = changedNode;
    }

    void nodeCollapsedChanged(KisNode *changedNode) override
    {
        call = ListenerCall::NodeCollapsedChanged;
        node = changedNode;
    }

    void invalidateAllFrames() override
    {
        call = ListenerCall::InvalidateAllFrames;
    }

    void notifySelectionChanged() override
    {
        call = ListenerCall::SelectionChanged;
    }

    void requestProjectionUpdate(KisNode *changedNode,
                                 const QVector<QRect> &changedRects,
                                 KisProjectionUpdateFlags flags) override
    {
        call = ListenerCall::ProjectionUpdate;
        node = changedNode;
        rects = changedRects;
        projectionFlags = flags;
    }

    void invalidateFrames(const KisTimeSpan &changedRange, const QRect &changedRect) override
    {
        call = ListenerCall::InvalidateFrames;
        range = changedRange;
        rect = changedRect;
    }

    void requestTimeSwitch(int requestedTime) override
    {
        call = ListenerCall::TimeSwitch;
        time = requestedTime;
    }

    KisNode *graphOverlayNode() const override
    {
        overlayQueried = true;
        return overlayNode;
    }

    void keyframeChannelHasBeenAdded(KisNode *changedNode, KisKeyframeChannel *changedChannel) override
    {
        call = ListenerCall::KeyframeAdded;
        node = changedNode;
        channel = changedChannel;
    }

    void keyframeChannelAboutToBeRemoved(KisNode *changedNode, KisKeyframeChannel *changedChannel) override
    {
        call = ListenerCall::KeyframeAboutToBeRemoved;
        node = changedNode;
        channel = changedChannel;
    }

    ListenerCall call = ListenerCall::None;
    KisNode *node = nullptr;
    int firstIndex = -1;
    int secondIndex = -1;
    KisNodeAdditionFlags additionFlags;
    QVector<QRect> rects;
    KisProjectionUpdateFlags projectionFlags;
    KisTimeSpan range;
    QRect rect;
    int time = -1;
    KisKeyframeChannel *channel = nullptr;
    KisNode *overlayNode = nullptr;
    mutable bool overlayQueried = false;
    bool *destroyed = nullptr;
};

int callValue(ListenerCall call)
{
    return static_cast<int>(call);
}
} // namespace

class KisNodeGraphListenerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseListenerStartsWithEmptyGraphStateAndUsesVirtualDestruction();
    void hierarchyNotificationsAdvanceSequenceExactlyOnce();
    void otherDefaultNotificationsPreserveSequence();
    void derivedListenerReceivesEveryVirtualCallArgument();
};

void KisNodeGraphListenerContractTest::baseListenerStartsWithEmptyGraphStateAndUsesVirtualDestruction()
{
    KisNodeGraphListener listener;

    QCOMPARE(listener.graphSequenceNumber(), 0);
    QCOMPARE(listener.graphOverlayNode(), nullptr);

    bool destroyed = false;
    std::unique_ptr<KisNodeGraphListener> polymorphicListener(new ListenerProbe(nullptr, &destroyed));
    polymorphicListener.reset();

    QVERIFY(destroyed);
}

void KisNodeGraphListenerContractTest::hierarchyNotificationsAdvanceSequenceExactlyOnce()
{
    KisNodeGraphListener listener;
    int parentToken = 0;
    int nodeToken = 0;
    auto *parent = reinterpret_cast<KisNode *>(&parentToken);
    auto *node = reinterpret_cast<KisNode *>(&nodeToken);
    int expectedSequence = 0;

    listener.aboutToAddANode(parent, 2);
    QCOMPARE(listener.graphSequenceNumber(), ++expectedSequence);

    listener.nodeHasBeenAdded(parent, 2, KisNodeAdditionFlag::DontActivateNode);
    QCOMPARE(listener.graphSequenceNumber(), ++expectedSequence);

    listener.aboutToRemoveANode(parent, 5);
    QCOMPARE(listener.graphSequenceNumber(), ++expectedSequence);

    listener.nodeHasBeenRemoved(parent, 5);
    QCOMPARE(listener.graphSequenceNumber(), ++expectedSequence);

    listener.aboutToMoveNode(node, 1, 4);
    QCOMPARE(listener.graphSequenceNumber(), ++expectedSequence);

    listener.nodeHasBeenMoved(node, 1, 4);
    QCOMPARE(listener.graphSequenceNumber(), ++expectedSequence);
}

void KisNodeGraphListenerContractTest::otherDefaultNotificationsPreserveSequence()
{
    KisNodeGraphListener listener;
    int nodeToken = 0;
    int channelToken = 0;
    auto *node = reinterpret_cast<KisNode *>(&nodeToken);
    auto *channel = reinterpret_cast<KisKeyframeChannel *>(&channelToken);
    const QVector<QRect> rects{QRect(1, 2, 3, 4), QRect(8, 9, 10, 11)};
    const KisProjectionUpdateFlags flags =
        KisProjectionUpdateFlag::NoFilthy | KisProjectionUpdateFlag::DontInvalidateFrames;
    const KisTimeSpan range = KisTimeSpan::fromTimeToTime(7, 13);
    const QRect rect(12, 14, 16, 18);

    listener.nodeChanged(node);
    QCOMPARE(listener.graphSequenceNumber(), 0);
    listener.nodeCollapsedChanged(node);
    QCOMPARE(listener.graphSequenceNumber(), 0);
    listener.invalidateAllFrames();
    QCOMPARE(listener.graphSequenceNumber(), 0);
    listener.notifySelectionChanged();
    QCOMPARE(listener.graphSequenceNumber(), 0);
    listener.requestProjectionUpdate(node, rects, flags);
    QCOMPARE(listener.graphSequenceNumber(), 0);
    listener.invalidateFrames(range, rect);
    QCOMPARE(listener.graphSequenceNumber(), 0);
    listener.requestTimeSwitch(23);
    QCOMPARE(listener.graphSequenceNumber(), 0);
    listener.keyframeChannelHasBeenAdded(node, channel);
    QCOMPARE(listener.graphSequenceNumber(), 0);
    listener.keyframeChannelAboutToBeRemoved(node, channel);
    QCOMPARE(listener.graphSequenceNumber(), 0);
}

void KisNodeGraphListenerContractTest::derivedListenerReceivesEveryVirtualCallArgument()
{
    int parentToken = 0;
    int nodeToken = 0;
    int overlayToken = 0;
    int channelToken = 0;
    auto *parent = reinterpret_cast<KisNode *>(&parentToken);
    auto *node = reinterpret_cast<KisNode *>(&nodeToken);
    auto *overlay = reinterpret_cast<KisNode *>(&overlayToken);
    auto *channel = reinterpret_cast<KisKeyframeChannel *>(&channelToken);
    const KisNodeAdditionFlags additionFlags = KisNodeAdditionFlag::DontActivateNode;
    const QVector<QRect> rects{QRect(2, 3, 5, 7), QRect(11, 13, 17, 19)};
    const KisProjectionUpdateFlags projectionFlags =
        KisProjectionUpdateFlag::NoFilthy | KisProjectionUpdateFlag::DontInvalidateFrames;
    const KisTimeSpan range = KisTimeSpan::fromTimeToTime(29, 31);
    const QRect invalidatedRect(23, 29, 31, 37);
    ListenerProbe probe(overlay);
    KisNodeGraphListener &listener = probe;

    listener.aboutToAddANode(parent, 3);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::AboutToAdd));
    QCOMPARE(probe.node, parent);
    QCOMPARE(probe.firstIndex, 3);

    listener.nodeHasBeenAdded(parent, 4, additionFlags);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::Added));
    QCOMPARE(probe.node, parent);
    QCOMPARE(probe.firstIndex, 4);
    QCOMPARE(static_cast<int>(probe.additionFlags), static_cast<int>(additionFlags));

    listener.aboutToRemoveANode(parent, 5);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::AboutToRemove));
    QCOMPARE(probe.node, parent);
    QCOMPARE(probe.firstIndex, 5);

    listener.nodeHasBeenRemoved(parent, 6);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::Removed));
    QCOMPARE(probe.node, parent);
    QCOMPARE(probe.firstIndex, 6);

    listener.aboutToMoveNode(node, 7, 8);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::AboutToMove));
    QCOMPARE(probe.node, node);
    QCOMPARE(probe.firstIndex, 7);
    QCOMPARE(probe.secondIndex, 8);

    listener.nodeHasBeenMoved(node, 9, 10);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::Moved));
    QCOMPARE(probe.node, node);
    QCOMPARE(probe.firstIndex, 9);
    QCOMPARE(probe.secondIndex, 10);

    listener.nodeChanged(node);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::NodeChanged));
    QCOMPARE(probe.node, node);

    listener.nodeCollapsedChanged(node);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::NodeCollapsedChanged));
    QCOMPARE(probe.node, node);

    listener.invalidateAllFrames();
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::InvalidateAllFrames));

    listener.notifySelectionChanged();
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::SelectionChanged));

    listener.requestProjectionUpdate(node, rects, projectionFlags);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::ProjectionUpdate));
    QCOMPARE(probe.node, node);
    QCOMPARE(probe.rects, rects);
    QCOMPARE(static_cast<int>(probe.projectionFlags), static_cast<int>(projectionFlags));

    listener.invalidateFrames(range, invalidatedRect);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::InvalidateFrames));
    QCOMPARE(probe.range.start(), 29);
    QCOMPARE(probe.range.end(), 31);
    QCOMPARE(probe.rect, invalidatedRect);

    listener.requestTimeSwitch(41);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::TimeSwitch));
    QCOMPARE(probe.time, 41);

    QCOMPARE(listener.graphOverlayNode(), overlay);
    QVERIFY(probe.overlayQueried);

    listener.keyframeChannelHasBeenAdded(node, channel);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::KeyframeAdded));
    QCOMPARE(probe.node, node);
    QCOMPARE(probe.channel, channel);

    listener.keyframeChannelAboutToBeRemoved(node, channel);
    QCOMPARE(callValue(probe.call), callValue(ListenerCall::KeyframeAboutToBeRemoved));
    QCOMPARE(probe.node, node);
    QCOMPARE(probe.channel, channel);
}

QTEST_MAIN(KisNodeGraphListenerContractTest)

#include "KisNodeGraphListenerContractTest.moc"
