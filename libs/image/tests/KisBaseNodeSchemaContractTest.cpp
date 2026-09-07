/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_base_node.h"

#include <QTest>

#include <type_traits>

#define ASSERT_BASE_NODE_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBaseNode::method)), signature>)
#define ASSERT_BASE_NODE_PROPERTY_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBaseNode::Property::method)), signature>)
#define ASSERT_BASE_NODE_PROPERTY_MEMBER(member, type)                                                                 \
    static_assert(std::is_same_v<decltype(KisBaseNode::Property::member), type>)

class BaseNodeProbe final : public KisBaseNode
{
public:
    using KisBaseNode::KisBaseNode;

    explicit BaseNodeProbe(const KisBaseNode &rhs)
        : KisBaseNode(rhs)
    {
    }

    KisPaintDeviceSP paintDevice() const override;
    KisPaintDeviceSP original() const override;
    KisPaintDeviceSP projection() const override;
    const KoColorSpace *colorSpace() const override;
    const KoCompositeOp *compositeOp() const override;
};

class KisBaseNodeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void propertyValueSchemaRemainsStable();
    void baseNodeTypeConstructionAndLifetimeSchemaRemainStable();
    void deviceColorAndCompositionSignaturesRemainStable();
    void identityAndPresentationSignaturesRemainStable();
    void opacityVisibilityAndEditingSignaturesRemainStable();
    void arbitraryPropertySignaturesRemainStable();
    void thumbnailSignaturesRemainStable();
    void geometryAndLodSignaturesRemainStable();
    void keyframeAndAnimationSignaturesRemainStable();
    void visitorImageAndSettingsSignaturesRemainStable();
};

void KisBaseNodeSchemaContractTest::propertyValueSchemaRemainsStable()
{
    using Property = KisBaseNode::Property;

    static_assert(std::is_class_v<Property>);
    static_assert(std::is_same_v<KisBaseNode::PropertyList, QList<Property>>);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(id, QString);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(name, QString);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(isMutable, bool);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(onIcon, QIcon);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(offIcon, QIcon);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(state, QVariant);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(canHaveStasis, bool);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(isInStasis, bool);
    ASSERT_BASE_NODE_PROPERTY_MEMBER(stateInStasis, bool);
    static_assert(std::is_default_constructible_v<Property>);
    static_assert(std::is_constructible_v<Property, const KoID &, const QIcon &, const QIcon &, bool>);
    static_assert(std::is_constructible_v<Property, const KoID &, const QIcon &, const QIcon &, bool, bool, bool>);
    static_assert(std::is_constructible_v<Property, const KoID &, const QString &>);
    ASSERT_BASE_NODE_PROPERTY_SIGNATURE(operator==, bool (Property::*)(const Property &) const);
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const Property &)>(&operator<<)),
                                 QDebug (*)(QDebug, const Property &)>);

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::baseNodeTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Node = KisBaseNode;

    static_assert(std::is_class_v<Node>);
    static_assert(std::is_base_of_v<QObject, Node>);
    static_assert(std::is_base_of_v<KisShared, Node>);
    static_assert(std::is_abstract_v<Node>);
    static_assert(std::is_constructible_v<BaseNodeProbe, KisImageWSP>);
    static_assert(std::is_constructible_v<BaseNodeProbe, const KisBaseNode &>);
    static_assert(std::has_virtual_destructor_v<Node>);

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::deviceColorAndCompositionSignaturesRemainStable()
{
    using Node = KisBaseNode;

    ASSERT_BASE_NODE_SIGNATURE(paintDevice, KisPaintDeviceSP (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(original, KisPaintDeviceSP (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(projection, KisPaintDeviceSP (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(colorSampleSourceDevice, KisPaintDeviceSP (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(colorSpace, const KoColorSpace *(Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(compositeOp, const KoCompositeOp *(Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(compositeOpId, const QString &(Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setCompositeOpId, void (Node::*)(const QString &));

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::identityAndPresentationSignaturesRemainStable()
{
    using Node = KisBaseNode;

    ASSERT_BASE_NODE_SIGNATURE(uuid, QUuid (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setUuid, void (Node::*)(const QUuid &));
    ASSERT_BASE_NODE_SIGNATURE(name, QString (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setName, void (Node::*)(const QString &));
    ASSERT_BASE_NODE_SIGNATURE(icon, QIcon (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(collapsed, bool (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setCollapsed, void (Node::*)(bool));
    ASSERT_BASE_NODE_SIGNATURE(colorLabelIndex, int (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setColorLabelIndex, void (Node::*)(int));

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::opacityVisibilityAndEditingSignaturesRemainStable()
{
    using Node = KisBaseNode;

    ASSERT_BASE_NODE_SIGNATURE(opacity, quint8 (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setOpacity, void (Node::*)(quint8));
    ASSERT_BASE_NODE_SIGNATURE(percentOpacity, quint8 (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setPercentOpacity, void (Node::*)(quint8));
    ASSERT_BASE_NODE_SIGNATURE(opacityChanged, void (Node::*)(quint8));
    ASSERT_BASE_NODE_SIGNATURE(visible, bool (Node::*)(bool) const);
    ASSERT_BASE_NODE_SIGNATURE(setVisible, void (Node::*)(bool, bool));
    ASSERT_BASE_NODE_SIGNATURE(userLocked, bool (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setUserLocked, void (Node::*)(bool));
    ASSERT_BASE_NODE_SIGNATURE(belongsToIsolatedGroup, bool (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(isIsolatedRoot, bool (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(isEditable, bool (Node::*)(bool) const);
    ASSERT_BASE_NODE_SIGNATURE(hasEditablePaintDevice, bool (Node::*)() const);

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::arbitraryPropertySignaturesRemainStable()
{
    using Node = KisBaseNode;

    ASSERT_BASE_NODE_SIGNATURE(sectionModelProperties, Node::PropertyList (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setSectionModelProperties, void (Node::*)(const Node::PropertyList &));
    ASSERT_BASE_NODE_SIGNATURE(nodeProperties, const KoProperties &(Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setNodeProperty, void (Node::*)(const QString &, const QVariant &));
    ASSERT_BASE_NODE_SIGNATURE(mergeNodeProperties, void (Node::*)(const KoProperties &));
    ASSERT_BASE_NODE_SIGNATURE(check, bool (Node::*)(const KoProperties &) const);

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::thumbnailSignaturesRemainStable()
{
    using Node = KisBaseNode;

    ASSERT_BASE_NODE_SIGNATURE(createThumbnail,
                               QImage (Node::*)(qint32, qint32, Qt::AspectRatioMode, KisThumbnailBoundsMode));
    ASSERT_BASE_NODE_SIGNATURE(createPreferredThumbnail, QImage (Node::*)(qint32, qint32, Qt::AspectRatioMode));
    ASSERT_BASE_NODE_SIGNATURE(preferredThumbnailBoundsMode, KisThumbnailBoundsMode (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setPreferredThumbnailBoundsMode, void (Node::*)(KisThumbnailBoundsMode) const);
    ASSERT_BASE_NODE_SIGNATURE(thumbnailSeqNo, int (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(createThumbnailForFrame,
                               QImage (Node::*)(qint32, qint32, int, Qt::AspectRatioMode, KisThumbnailBoundsMode));
    ASSERT_BASE_NODE_SIGNATURE(createPreferredThumbnailForFrame,
                               QImage (Node::*)(qint32, qint32, int, Qt::AspectRatioMode));

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::geometryAndLodSignaturesRemainStable()
{
    using Node = KisBaseNode;

    ASSERT_BASE_NODE_SIGNATURE(x, qint32 (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setX, void (Node::*)(qint32));
    ASSERT_BASE_NODE_SIGNATURE(y, qint32 (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setY, void (Node::*)(qint32));
    ASSERT_BASE_NODE_SIGNATURE(extent, QRect (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(exactBounds, QRect (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(supportsLodMoves, bool (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(supportsLodPainting, bool (Node::*)() const);

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::keyframeAndAnimationSignaturesRemainStable()
{
    using Node = KisBaseNode;
    using ChannelMap = QMap<QString, KisKeyframeChannel *>;

    ASSERT_BASE_NODE_SIGNATURE(keyframeChannels, ChannelMap (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(getKeyframeChannel, KisKeyframeChannel * (Node::*)(const QString &, bool));
    ASSERT_BASE_NODE_SIGNATURE(getKeyframeChannel, KisKeyframeChannel * (Node::*)(const QString &) const);
    ASSERT_BASE_NODE_SIGNATURE(supportsKeyframeChannel, bool (Node::*)(const QString &));
    ASSERT_BASE_NODE_SIGNATURE(keyframeChannelAdded, void (Node::*)(KisKeyframeChannel *));
    ASSERT_BASE_NODE_SIGNATURE(isPinnedToTimeline, bool (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setPinnedToTimeline, void (Node::*)(bool));
    ASSERT_BASE_NODE_SIGNATURE(isAnimated, bool (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(enableAnimation, void (Node::*)());

    QVERIFY(true);
}

void KisBaseNodeSchemaContractTest::visitorImageAndSettingsSignaturesRemainStable()
{
    using Node = KisBaseNode;

    ASSERT_BASE_NODE_SIGNATURE(accept, bool (Node::*)(KisNodeVisitor &));
    ASSERT_BASE_NODE_SIGNATURE(accept, void (Node::*)(KisProcessingVisitor &, KisUndoAdapter *));
    ASSERT_BASE_NODE_SIGNATURE(image, KisImageWSP (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(setImage, void (Node::*)(KisImageWSP));
    ASSERT_BASE_NODE_SIGNATURE(isFakeNode, bool (Node::*)() const);
    ASSERT_BASE_NODE_SIGNATURE(updateSettings, void (Node::*)());

    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KisBaseNodeSchemaContractTest)

#include "KisBaseNodeSchemaContractTest.moc"
