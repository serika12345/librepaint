/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "generator/kis_generator_layer.h"
#include "kis_adjustment_layer.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_GENERATOR_LAYER_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGeneratorLayer::method)), signature>)

#define ASSERT_ADJUSTMENT_LAYER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAdjustmentLayer::method)), signature>)

} // namespace

class KisGeneratorLayerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void generatorLayerTypeLifetimeAndConstructionSchemaRemainStable();
    void generatorLayerConfigurationAndRegenerationSignaturesRemainStable();
    void generatorLayerDirtyRegionAndPositionSignaturesRemainStable();
    void generatorLayerHierarchyPresentationAndVisitorSignaturesRemainStable();
    void generatorLayerTimedUpdateSignaturesRemainStable();
    void adjustmentLayerTypeLifetimeAndCloneSchemaRemainStable();
    void adjustmentLayerVisitorSignaturesRemainStable();
    void adjustmentLayerPresentationSignaturesRemainStable();
    void adjustmentLayerConfigurationSignaturesRemainStable();
};

void KisGeneratorLayerSchemaContractTest::generatorLayerTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisGeneratorLayer>);
    static_assert(std::is_constructible_v<KisGeneratorLayer,
                                          KisImageWSP,
                                          const QString &,
                                          KisFilterConfigurationSP,
                                          KisSelectionSP>);
    static_assert(std::is_constructible_v<KisGeneratorLayer, const KisGeneratorLayer &>);
    static_assert(std::has_virtual_destructor_v<KisGeneratorLayer>);
}

void KisGeneratorLayerSchemaContractTest::generatorLayerConfigurationAndRegenerationSignaturesRemainStable()
{
    ASSERT_GENERATOR_LAYER_SIGNATURE(previewWithStroke,
                                     QWeakPointer<boost::none_t> (KisGeneratorLayer::*)(KisStrokeId));
    ASSERT_GENERATOR_LAYER_SIGNATURE(resetCache, void (KisGeneratorLayer::*)(const KoColorSpace *));
    ASSERT_GENERATOR_LAYER_SIGNATURE(setFilter, void (KisGeneratorLayer::*)(KisFilterConfigurationSP, bool));
    ASSERT_GENERATOR_LAYER_SIGNATURE(setFilterWithoutUpdate,
                                     void (KisGeneratorLayer::*)(KisFilterConfigurationSP, bool));
    ASSERT_GENERATOR_LAYER_SIGNATURE(update, void (KisGeneratorLayer::*)());

    using SetFilterWithDefaultComparison =
        decltype(std::declval<KisGeneratorLayer &>().setFilter(std::declval<KisFilterConfigurationSP>()));
    using SetFilterWithoutUpdateWithDefaultComparison =
        decltype(std::declval<KisGeneratorLayer &>().setFilterWithoutUpdate(std::declval<KisFilterConfigurationSP>()));
    static_assert(std::is_same_v<SetFilterWithDefaultComparison, void>);
    static_assert(std::is_same_v<SetFilterWithoutUpdateWithDefaultComparison, void>);
}

void KisGeneratorLayerSchemaContractTest::generatorLayerDirtyRegionAndPositionSignaturesRemainStable()
{
    ASSERT_GENERATOR_LAYER_SIGNATURE(setDirty, void (KisGeneratorLayer::*)(const QVector<QRect> &));
    ASSERT_GENERATOR_LAYER_SIGNATURE(setDirtyWithoutUpdate, void (KisGeneratorLayer::*)(const QVector<QRect> &));
    ASSERT_GENERATOR_LAYER_SIGNATURE(setX, void (KisGeneratorLayer::*)(qint32));
    ASSERT_GENERATOR_LAYER_SIGNATURE(setY, void (KisGeneratorLayer::*)(qint32));
}

void KisGeneratorLayerSchemaContractTest::generatorLayerHierarchyPresentationAndVisitorSignaturesRemainStable()
{
    ASSERT_GENERATOR_LAYER_SIGNATURE(accept, bool (KisGeneratorLayer::*)(KisNodeVisitor &));
    ASSERT_GENERATOR_LAYER_SIGNATURE(accept, void (KisGeneratorLayer::*)(KisProcessingVisitor &, KisUndoAdapter *));
    ASSERT_GENERATOR_LAYER_SIGNATURE(clone, KisNodeSP (KisGeneratorLayer::*)() const);
    ASSERT_GENERATOR_LAYER_SIGNATURE(icon, QIcon (KisGeneratorLayer::*)() const);
    ASSERT_GENERATOR_LAYER_SIGNATURE(layer, KisLayer * (KisGeneratorLayer::*)());
    ASSERT_GENERATOR_LAYER_SIGNATURE(sectionModelProperties, KisBaseNode::PropertyList (KisGeneratorLayer::*)() const);
}

void KisGeneratorLayerSchemaContractTest::generatorLayerTimedUpdateSignaturesRemainStable()
{
    ASSERT_GENERATOR_LAYER_SIGNATURE(forceUpdateTimedNode, void (KisGeneratorLayer::*)());
    ASSERT_GENERATOR_LAYER_SIGNATURE(hasPendingTimedUpdates, bool (KisGeneratorLayer::*)() const);
}

void KisGeneratorLayerSchemaContractTest::adjustmentLayerTypeLifetimeAndCloneSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAdjustmentLayer>);
    static_assert(std::is_base_of_v<KisSelectionBasedLayer, KisAdjustmentLayer>);
    static_assert(std::is_constructible_v<KisAdjustmentLayer,
                                          KisImageWSP,
                                          const QString &,
                                          KisFilterConfigurationSP,
                                          KisSelectionSP>);
    static_assert(std::is_copy_constructible_v<KisAdjustmentLayer>);
    static_assert(std::has_virtual_destructor_v<KisAdjustmentLayer>);
    ASSERT_ADJUSTMENT_LAYER_SIGNATURE(clone, KisNodeSP (KisAdjustmentLayer::*)() const);
}

void KisGeneratorLayerSchemaContractTest::adjustmentLayerVisitorSignaturesRemainStable()
{
    ASSERT_ADJUSTMENT_LAYER_SIGNATURE(accept, bool (KisAdjustmentLayer::*)(KisNodeVisitor &));
    ASSERT_ADJUSTMENT_LAYER_SIGNATURE(accept, void (KisAdjustmentLayer::*)(KisProcessingVisitor &, KisUndoAdapter *));
}

void KisGeneratorLayerSchemaContractTest::adjustmentLayerPresentationSignaturesRemainStable()
{
    ASSERT_ADJUSTMENT_LAYER_SIGNATURE(icon, QIcon (KisAdjustmentLayer::*)() const);
    ASSERT_ADJUSTMENT_LAYER_SIGNATURE(sectionModelProperties,
                                      KisBaseNode::PropertyList (KisAdjustmentLayer::*)() const);
    ASSERT_ADJUSTMENT_LAYER_SIGNATURE(layer, KisLayer * (KisAdjustmentLayer::*)());
}

void KisGeneratorLayerSchemaContractTest::adjustmentLayerConfigurationSignaturesRemainStable()
{
    ASSERT_ADJUSTMENT_LAYER_SIGNATURE(setFilter, void (KisAdjustmentLayer::*)(KisFilterConfigurationSP, bool));
    ASSERT_ADJUSTMENT_LAYER_SIGNATURE(setChannelFlags, void (KisAdjustmentLayer::*)(const QBitArray &));
    static_assert(std::is_same_v<decltype(std::declval<KisAdjustmentLayer &>().setFilter(
                                     std::declval<KisFilterConfigurationSP>())),
                                 void>);
}

QTEST_GUILESS_MAIN(KisGeneratorLayerSchemaContractTest)

#include "KisGeneratorLayerSchemaContractTest.moc"
