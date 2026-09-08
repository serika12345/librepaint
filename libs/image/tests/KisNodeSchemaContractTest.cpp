/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_node.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_NODE_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisNode::method)), signature>)

class NodeConstructionProbe final : public KisNode
{
public:
    using KisNode::KisNode;
    explicit NodeConstructionProbe(const KisNode &rhs)
        : KisNode(rhs)
    {
    }

    KisNodeSP clone() const override
    {
        return {};
    }
    bool allowAsChild(KisNodeSP) const override
    {
        return false;
    }
    KisPaintDeviceSP paintDevice() const override
    {
        return {};
    }
    KisPaintDeviceSP original() const override
    {
        return {};
    }
    KisPaintDeviceSP projection() const override
    {
        return {};
    }
    const KoColorSpace *colorSpace() const override
    {
        return nullptr;
    }
    const KoCompositeOp *compositeOp() const override
    {
        return nullptr;
    }
};
} // namespace

class KisNodeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeTypePositionAndConstructionSchemaRemainStable();
    void nodeVisitorLayerAndProjectionSignaturesRemainStable();
    void nodeDirtyAndAnimationSignaturesRemainStable();
    void nodeGraphHierarchySignaturesRemainStable();
    void nodeProgressAndNotificationSignaturesRemainStable();
};

void KisNodeSchemaContractTest::nodeTypePositionAndConstructionSchemaRemainStable()
{
    using Node = KisNode;

    static_assert(std::is_class_v<Node>);
    static_assert(std::is_enum_v<Node::PositionToFilthy>);
    static_assert(Node::N_ABOVE_FILTHY == 0x08);
    static_assert(Node::N_FILTHY_PROJECTION == 0x20);
    static_assert(Node::N_FILTHY == 0x40);
    static_assert(Node::N_BELOW_FILTHY == 0x80);
    static_assert(std::is_constructible_v<NodeConstructionProbe, KisImageWSP>);
    static_assert(std::is_constructible_v<NodeConstructionProbe, const Node &>);
    static_assert(std::has_virtual_destructor_v<Node>);
}

void KisNodeSchemaContractTest::nodeVisitorLayerAndProjectionSignaturesRemainStable()
{
    using Node = KisNode;

    ASSERT_NODE_SIGNATURE(clone, KisNodeSP (Node::*)() const);
    ASSERT_NODE_SIGNATURE(accept, bool (Node::*)(KisNodeVisitor &));
    ASSERT_NODE_SIGNATURE(accept, void (Node::*)(KisProcessingVisitor &, KisUndoAdapter *));
    ASSERT_NODE_SIGNATURE(allowAsChild, bool (Node::*)(KisNodeSP) const);
    ASSERT_NODE_SIGNATURE(setImage, void (Node::*)(KisImageWSP));
    ASSERT_NODE_SIGNATURE(projectionPlane, KisAbstractProjectionPlaneSP (Node::*)() const);
    ASSERT_NODE_SIGNATURE(syncLodCache, void (Node::*)());
    ASSERT_NODE_SIGNATURE(getLodCapableDevices, KisPaintDeviceList (Node::*)() const);
    ASSERT_NODE_SIGNATURE(projectionLeaf, KisProjectionLeafSP (Node::*)() const);
}

void KisNodeSchemaContractTest::nodeDirtyAndAnimationSignaturesRemainStable()
{
    using Node = KisNode;

    ASSERT_NODE_SIGNATURE(setDirty, void (Node::*)());
    ASSERT_NODE_SIGNATURE(setDirty, void (Node::*)(const QRect &));
    ASSERT_NODE_SIGNATURE(setDirty, void (Node::*)(const QVector<QRect> &));
    ASSERT_NODE_SIGNATURE(setDirty, void (Node::*)(const KisRegion &));
    ASSERT_NODE_SIGNATURE(setDirtyDontResetAnimationCache, void (Node::*)());
    ASSERT_NODE_SIGNATURE(setDirtyDontResetAnimationCache, void (Node::*)(const QRect &));
    ASSERT_NODE_SIGNATURE(setDirtyDontResetAnimationCache, void (Node::*)(const QVector<QRect> &));
    ASSERT_NODE_SIGNATURE(invalidateFrames, void (Node::*)(const KisTimeSpan &, const QRect &));
    ASSERT_NODE_SIGNATURE(handleKeyframeChannelFrameChange, void (Node::*)(const KisKeyframeChannel *, int));
    ASSERT_NODE_SIGNATURE(handleKeyframeChannelFrameAdded, void (Node::*)(const KisKeyframeChannel *, int));
    ASSERT_NODE_SIGNATURE(handleKeyframeChannelFrameAboutToBeRemoved, void (Node::*)(const KisKeyframeChannel *, int));
    ASSERT_NODE_SIGNATURE(handleKeyframeChannelFrameHasBeenRemoved, void (Node::*)(const KisKeyframeChannel *, int));
    ASSERT_NODE_SIGNATURE(requestTimeSwitch, void (Node::*)(int));
}

void KisNodeSchemaContractTest::nodeGraphHierarchySignaturesRemainStable()
{
    using Node = KisNode;

    ASSERT_NODE_SIGNATURE(graphSequenceNumber, int (Node::*)() const);
    ASSERT_NODE_SIGNATURE(graphListener, KisNodeGraphListener * (Node::*)() const);
    ASSERT_NODE_SIGNATURE(setGraphListener, void (Node::*)(KisNodeGraphListener *));
    ASSERT_NODE_SIGNATURE(parent, KisNodeSP (Node::*)() const);
    ASSERT_NODE_SIGNATURE(firstChild, KisNodeSP (Node::*)() const);
    ASSERT_NODE_SIGNATURE(lastChild, KisNodeSP (Node::*)() const);
    ASSERT_NODE_SIGNATURE(prevSibling, KisNodeSP (Node::*)() const);
    ASSERT_NODE_SIGNATURE(nextSibling, KisNodeSP (Node::*)() const);
    ASSERT_NODE_SIGNATURE(childCount, quint32 (Node::*)() const);
    ASSERT_NODE_SIGNATURE(at, KisNodeSP (Node::*)(quint32) const);
    ASSERT_NODE_SIGNATURE(index, int (Node::*)(KisNodeSP) const);
    ASSERT_NODE_SIGNATURE(childNodes, QList<KisNodeSP> (Node::*)(const QStringList &, const KoProperties &) const);
}

void KisNodeSchemaContractTest::nodeProgressAndNotificationSignaturesRemainStable()
{
    using Node = KisNode;

    ASSERT_NODE_SIGNATURE(nodeProgressProxy, KisNodeProgressProxy * (Node::*)() const);
    ASSERT_NODE_SIGNATURE(busyProgressIndicator, KisBusyProgressIndicator * (Node::*)() const);
    ASSERT_NODE_SIGNATURE(sigNodeChangedInternal, void (Node::*)());
}

#undef ASSERT_NODE_SIGNATURE

QTEST_APPLESS_MAIN(KisNodeSchemaContractTest)

#include "KisNodeSchemaContractTest.moc"
