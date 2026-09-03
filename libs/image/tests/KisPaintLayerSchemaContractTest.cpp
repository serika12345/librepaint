/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_layer.h"
#include "kis_paint_layer.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_PAINT_LAYER_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintLayer::method)), signature>)

#define ASSERT_LAYER_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayer::method)), signature>)

class LayerConstructorProbe final : public KisLayer
{
public:
    using KisLayer::KisLayer;

    explicit LayerConstructorProbe(const KisLayer &rhs)
        : KisLayer(rhs)
    {
    }

    KisNodeSP clone() const override
    {
        return nullptr;
    }

    bool allowAsChild(KisNodeSP) const override
    {
        return false;
    }

    KisPaintDeviceSP paintDevice() const override
    {
        return nullptr;
    }

    KisPaintDeviceSP original() const override
    {
        return nullptr;
    }
};

} // namespace

class KisPaintLayerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paintLayerTypeAndConstructionSchemaRemainStable();
    void paintLayerHierarchyAndPresentationSignaturesRemainStable();
    void paintLayerDeviceAndGeometrySignaturesRemainStable();
    void paintLayerEditingAndDecorationSignaturesRemainStable();
    void paintLayerVisitorAndKeyframeSignaturesRemainStable();
    void layerTypeAndConstructionSchemaRemainStable();
    void layerProjectionSelectionAndMetadataSignaturesRemainStable();
    void layerStateGeometryAndPresentationSignaturesRemainStable();
    void layerCompositionAndEffectSignaturesRemainStable();
    void layerCloneTrackingSignaturesRemainStable();
};

void KisPaintLayerSchemaContractTest::paintLayerTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisPaintLayer>);
    static_assert(std::is_base_of_v<KisLayer, KisPaintLayer>);
    static_assert(std::is_base_of_v<KisIndirectPaintingSupport, KisPaintLayer>);
    static_assert(std::is_base_of_v<KisDecoratedNodeInterface, KisPaintLayer>);
    static_assert(std::is_same_v<KisPaintLayerSP, KisSharedPtr<KisPaintLayer>>);
    static_assert(std::is_constructible_v<KisPaintLayer, KisImageWSP, const QString &, quint8>);
    static_assert(std::is_constructible_v<KisPaintLayer, KisImageWSP, const QString &, quint8, KisPaintDeviceSP>);
    static_assert(std::is_constructible_v<KisPaintLayer, KisImageWSP, const QString &, quint8, const KoColorSpace *>);
    static_assert(std::is_copy_constructible_v<KisPaintLayer>);
    static_assert(std::has_virtual_destructor_v<KisPaintLayer>);

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::paintLayerHierarchyAndPresentationSignaturesRemainStable()
{
    ASSERT_PAINT_LAYER_SIGNATURE(allowAsChild, bool (KisPaintLayer::*)(KisNodeSP) const);
    ASSERT_PAINT_LAYER_SIGNATURE(clone, KisNodeSP (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(icon, QIcon (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(needProjection, bool (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(sectionModelProperties, KisBaseNode::PropertyList (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(setImage, void (KisPaintLayer::*)(KisImageWSP));
    ASSERT_PAINT_LAYER_SIGNATURE(setSectionModelProperties, void (KisPaintLayer::*)(const KisBaseNode::PropertyList &));

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::paintLayerDeviceAndGeometrySignaturesRemainStable()
{
    ASSERT_PAINT_LAYER_SIGNATURE(exactBounds, QRect (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(extent, QRect (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(getLodCapableDevices, KisPaintDeviceList (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(layer, KisLayer * (KisPaintLayer::*)());
    ASSERT_PAINT_LAYER_SIGNATURE(original, KisPaintDeviceSP (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(paintDevice, KisPaintDeviceSP (KisPaintLayer::*)() const);

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::paintLayerEditingAndDecorationSignaturesRemainStable()
{
    ASSERT_PAINT_LAYER_SIGNATURE(alphaLocked, bool (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(channelLockFlags, const QBitArray &(KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(decorationsVisible, bool (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(flushOnionSkinCache, void (KisPaintLayer::*)());
    ASSERT_PAINT_LAYER_SIGNATURE(onionSkinEnabled, bool (KisPaintLayer::*)() const);
    ASSERT_PAINT_LAYER_SIGNATURE(setAlphaLocked, void (KisPaintLayer::*)(bool));
    ASSERT_PAINT_LAYER_SIGNATURE(setChannelLockFlags, void (KisPaintLayer::*)(const QBitArray &));
    ASSERT_PAINT_LAYER_SIGNATURE(setDecorationsVisible, void (KisPaintLayer::*)(bool, bool));
    ASSERT_PAINT_LAYER_SIGNATURE(setOnionSkinEnabled, void (KisPaintLayer::*)(bool));

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::paintLayerVisitorAndKeyframeSignaturesRemainStable()
{
    ASSERT_PAINT_LAYER_SIGNATURE(accept, bool (KisPaintLayer::*)(KisNodeVisitor &));
    ASSERT_PAINT_LAYER_SIGNATURE(accept, void (KisPaintLayer::*)(KisProcessingVisitor &, KisUndoAdapter *));
    ASSERT_PAINT_LAYER_SIGNATURE(handleKeyframeChannelFrameAdded,
                                 void (KisPaintLayer::*)(const KisKeyframeChannel *, int));
    ASSERT_PAINT_LAYER_SIGNATURE(handleKeyframeChannelFrameChange,
                                 void (KisPaintLayer::*)(const KisKeyframeChannel *, int));
    ASSERT_PAINT_LAYER_SIGNATURE(slotExternalUpdateOnionSkins, void (KisPaintLayer::*)());

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::layerTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_same_v<KisLayerProjectionPlaneSP, QSharedPointer<KisLayerProjectionPlane>>);
    static_assert(std::is_class_v<KisLayer>);
    static_assert(std::is_abstract_v<KisLayer>);
    static_assert(std::is_base_of_v<KisNode, KisLayer>);
    static_assert(std::is_constructible_v<LayerConstructorProbe, KisImageWSP, const QString &, quint8>);
    static_assert(std::is_constructible_v<LayerConstructorProbe, const KisLayer &>);
    static_assert(std::has_virtual_destructor_v<KisLayer>);

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::layerProjectionSelectionAndMetadataSignaturesRemainStable()
{
    ASSERT_LAYER_SIGNATURE(colorSpace, const KoColorSpace *(KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(compositeOp, const KoCompositeOp *(KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(layerStyle, KisPSDLayerStyleSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(setLayerStyle, void (KisLayer::*)(KisPSDLayerStyleSP));
    ASSERT_LAYER_SIGNATURE(projectionPlane, KisAbstractProjectionPlaneSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(internalProjectionPlane, KisLayerProjectionPlaneSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(needProjection, bool (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(projection, KisPaintDeviceSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(original, KisPaintDeviceSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(selectionMask, KisSelectionMaskSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(selection, KisSelectionSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(metaData, KisMetaData::Store * (KisLayer::*)());

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::layerStateGeometryAndPresentationSignaturesRemainStable()
{
    ASSERT_LAYER_SIGNATURE(sectionModelProperties, KisBaseNode::PropertyList (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(setSectionModelProperties, void (KisLayer::*)(const KisBaseNode::PropertyList &));
    ASSERT_LAYER_SIGNATURE(disableAlphaChannel, void (KisLayer::*)(bool));
    ASSERT_LAYER_SIGNATURE(alphaChannelDisabled, bool (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(setChannelFlags, void (KisLayer::*)(const QBitArray &));
    ASSERT_LAYER_SIGNATURE(channelFlags, QBitArray & (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(temporary, bool (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(setTemporary, void (KisLayer::*)(bool));
    ASSERT_LAYER_SIGNATURE(setImage, void (KisLayer::*)(KisImageWSP));
    ASSERT_LAYER_SIGNATURE(x, qint32 (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(y, qint32 (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(setX, void (KisLayer::*)(qint32));
    ASSERT_LAYER_SIGNATURE(setY, void (KisLayer::*)(qint32));
    ASSERT_LAYER_SIGNATURE(extent, QRect (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(exactBounds, QRect (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(createThumbnail,
                           QImage (KisLayer::*)(qint32, qint32, Qt::AspectRatioMode, KisThumbnailBoundsMode));
    ASSERT_LAYER_SIGNATURE(thumbnailSeqNo, int (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(createThumbnailForFrame,
                           QImage (KisLayer::*)(qint32, qint32, int, Qt::AspectRatioMode, KisThumbnailBoundsMode));
    ASSERT_LAYER_SIGNATURE(tightUserVisibleBounds, QRect (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(looseUserVisibleBounds, QRect (KisLayer::*)() const);

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::layerCompositionAndEffectSignaturesRemainStable()
{
    ASSERT_LAYER_SIGNATURE(partialChangeRect, QRect (KisLayer::*)(KisNodeSP, const QRect &));
    ASSERT_LAYER_SIGNATURE(buildProjectionUpToNode, void (KisLayer::*)(KisPaintDeviceSP, KisNodeSP, const QRect &));
    ASSERT_LAYER_SIGNATURE(tryCreateInternallyMergedLayerFromMutipleLayers,
                           KisLayerSP (KisLayer::*)(QList<KisLayerSP>));
    ASSERT_LAYER_SIGNATURE(createMergedLayerTemplate, KisLayerSP (KisLayer::*)(KisLayerSP));
    ASSERT_LAYER_SIGNATURE(fillMergedLayerTemplate, void (KisLayer::*)(KisLayerSP, KisLayerSP, bool));
    ASSERT_LAYER_SIGNATURE(hasEffectMasks, bool (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(effectMasks, QList<KisEffectMaskSP> (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(effectMasks, QList<KisEffectMaskSP> (KisLayer::*)(KisNodeSP) const);
    ASSERT_LAYER_SIGNATURE(colorOverlayMask, KisFilterMaskSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(parentLayer, KisLayerSP (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(notifyChildMaskChanged, void (KisLayer::*)());
    static_assert(
        std::is_same_v<decltype(std::declval<KisLayer &>().fillMergedLayerTemplate(std::declval<KisLayerSP>(),
                                                                                   std::declval<KisLayerSP>())),
                       void>);

    QVERIFY(true);
}

void KisPaintLayerSchemaContractTest::layerCloneTrackingSignaturesRemainStable()
{
    ASSERT_LAYER_SIGNATURE(registerClone, void (KisLayer::*)(KisCloneLayerWSP));
    ASSERT_LAYER_SIGNATURE(unregisterClone, void (KisLayer::*)(KisCloneLayerWSP));
    ASSERT_LAYER_SIGNATURE(registeredClones, const QList<KisCloneLayerWSP> (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(hasClones, bool (KisLayer::*)() const);
    ASSERT_LAYER_SIGNATURE(updateClones, void (KisLayer::*)(const QRect &, bool));

    QVERIFY(true);
}

#undef ASSERT_LAYER_SIGNATURE
#undef ASSERT_PAINT_LAYER_SIGNATURE

QTEST_GUILESS_MAIN(KisPaintLayerSchemaContractTest)

#include "KisPaintLayerSchemaContractTest.moc"
