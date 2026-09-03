/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_paint_layer.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_PAINT_LAYER_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintLayer::method)), signature>)

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

#undef ASSERT_PAINT_LAYER_SIGNATURE

QTEST_GUILESS_MAIN(KisPaintLayerSchemaContractTest)

#include "KisPaintLayerSchemaContractTest.moc"
