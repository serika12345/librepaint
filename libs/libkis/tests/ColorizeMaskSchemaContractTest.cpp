/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <CloneLayer.h>
#include <ColorizeMask.h>
#include <FileLayer.h>
#include <FillLayer.h>
#include <FilterLayer.h>
#include <FilterMask.h>
#include <GroupLayer.h>
#include <QTest>
#include <SelectionMask.h>
#include <TransformMask.h>
#include <TransparencyMask.h>

#include <type_traits>
#include <utility>

#define ASSERT_COLORIZE_MASK_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&ColorizeMask::method)), signature>)

#define ASSERT_MASK_SIGNATURE(owner, method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&owner::method)), signature>)

class ColorizeMaskSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void colorizeMaskOwnershipLifetimeAndTypeSchemaRemainsStable();
    void colorizeMaskKeyStrokeDataSignaturesRemainStable();
    void colorizeMaskDetectionAndCleanupPolicySignaturesRemainStable();
    void colorizeMaskEditingBoundsAndOutputPolicySignaturesRemainStable();
    void colorizeMaskUpdateAndCacheSignaturesRemainStable();
    void transformMaskTypeAndLifetimeSchemaRemainStable();
    void transformMaskTransformAndSerializationSchemaRemainStable();
    void transparencyMaskTypeAndSelectionSchemaRemainStable();
    void selectionMaskTypeAndSelectionSchemaRemainStable();
    void filterMaskTypeAndFilterSchemaRemainStable();
    void fileLayerTypeAndFileSchemaRemainStable();
    void fillLayerTypeAndGeneratorSchemaRemainStable();
    void filterLayerTypeAndFilterSchemaRemainStable();
    void cloneLayerTypeAndSourceSchemaRemainStable();
    void groupLayerTypeAndCompositionSchemaRemainStable();
};

void ColorizeMaskSchemaContractTest::colorizeMaskOwnershipLifetimeAndTypeSchemaRemainsStable()
{
    static_assert(std::is_class_v<ColorizeMask>);
    static_assert(std::is_base_of_v<Node, ColorizeMask>);
    static_assert(std::is_constructible_v<ColorizeMask, KisImageSP, QString>);
    static_assert(std::is_constructible_v<ColorizeMask, KisImageSP, QString, QObject *>);
    static_assert(std::is_constructible_v<ColorizeMask, KisImageSP, KisColorizeMaskSP>);
    static_assert(std::is_constructible_v<ColorizeMask, KisImageSP, KisColorizeMaskSP, QObject *>);
    static_assert(std::has_virtual_destructor_v<ColorizeMask>);
    ASSERT_COLORIZE_MASK_SIGNATURE(type, QString (ColorizeMask::*)() const);
}

void ColorizeMaskSchemaContractTest::colorizeMaskKeyStrokeDataSignaturesRemainStable()
{
    ASSERT_COLORIZE_MASK_SIGNATURE(keyStrokesColors, QList<ManagedColor *> (ColorizeMask::*)() const);
    ASSERT_COLORIZE_MASK_SIGNATURE(initializeKeyStrokeColors, void (ColorizeMask::*)(QList<ManagedColor *>, int));
    ASSERT_COLORIZE_MASK_SIGNATURE(removeKeyStroke, void (ColorizeMask::*)(ManagedColor *));
    ASSERT_COLORIZE_MASK_SIGNATURE(transparencyIndex, int (ColorizeMask::*)() const);
    ASSERT_COLORIZE_MASK_SIGNATURE(keyStrokePixelData,
                                   QByteArray (ColorizeMask::*)(ManagedColor *, int, int, int, int) const);
    ASSERT_COLORIZE_MASK_SIGNATURE(setKeyStrokePixelData,
                                   bool (ColorizeMask::*)(QByteArray, ManagedColor *, int, int, int, int));

    using InitializeWithDefaultIndex =
        decltype(std::declval<ColorizeMask &>().initializeKeyStrokeColors(std::declval<QList<ManagedColor *>>()));
    static_assert(std::is_same_v<InitializeWithDefaultIndex, void>);
}

void ColorizeMaskSchemaContractTest::colorizeMaskDetectionAndCleanupPolicySignaturesRemainStable()
{
    ASSERT_COLORIZE_MASK_SIGNATURE(setUseEdgeDetection, void (ColorizeMask::*)(bool));
    ASSERT_COLORIZE_MASK_SIGNATURE(useEdgeDetection, bool (ColorizeMask::*)() const);
    ASSERT_COLORIZE_MASK_SIGNATURE(setEdgeDetectionSize, void (ColorizeMask::*)(qreal));
    ASSERT_COLORIZE_MASK_SIGNATURE(edgeDetectionSize, qreal (ColorizeMask::*)() const);
    ASSERT_COLORIZE_MASK_SIGNATURE(setCleanUpAmount, void (ColorizeMask::*)(qreal));
    ASSERT_COLORIZE_MASK_SIGNATURE(cleanUpAmount, qreal (ColorizeMask::*)() const);
}

void ColorizeMaskSchemaContractTest::colorizeMaskEditingBoundsAndOutputPolicySignaturesRemainStable()
{
    ASSERT_COLORIZE_MASK_SIGNATURE(setLimitToDeviceBounds, void (ColorizeMask::*)(bool));
    ASSERT_COLORIZE_MASK_SIGNATURE(limitToDeviceBounds, bool (ColorizeMask::*)() const);
    ASSERT_COLORIZE_MASK_SIGNATURE(setShowOutput, void (ColorizeMask::*)(bool));
    ASSERT_COLORIZE_MASK_SIGNATURE(showOutput, bool (ColorizeMask::*)() const);
    ASSERT_COLORIZE_MASK_SIGNATURE(setEditKeyStrokes, void (ColorizeMask::*)(bool));
    ASSERT_COLORIZE_MASK_SIGNATURE(editKeyStrokes, bool (ColorizeMask::*)() const);
}

void ColorizeMaskSchemaContractTest::colorizeMaskUpdateAndCacheSignaturesRemainStable()
{
    ASSERT_COLORIZE_MASK_SIGNATURE(updateMask, void (ColorizeMask::*)(bool));
    ASSERT_COLORIZE_MASK_SIGNATURE(resetCache, void (ColorizeMask::*)());

    using UpdateWithDefaultForce = decltype(std::declval<ColorizeMask &>().updateMask());
    static_assert(std::is_same_v<UpdateWithDefaultForce, void>);
}

void ColorizeMaskSchemaContractTest::transformMaskTypeAndLifetimeSchemaRemainStable()
{
    using Mask = TransformMask;

    static_assert(std::is_class_v<Mask>);
    static_assert(std::is_base_of_v<Node, Mask>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, KisTransformMaskSP>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, KisTransformMaskSP, QObject *>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, QString>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, QString, QObject *>);
    static_assert(std::has_virtual_destructor_v<Mask>);
    ASSERT_MASK_SIGNATURE(Mask, type, QString (Mask::*)() const);
}

void ColorizeMaskSchemaContractTest::transformMaskTransformAndSerializationSchemaRemainStable()
{
    using Mask = TransformMask;

    ASSERT_MASK_SIGNATURE(Mask, finalAffineTransform, QTransform (Mask::*)() const);
    ASSERT_MASK_SIGNATURE(Mask, fromXML, bool (Mask::*)(const QString &));
    ASSERT_MASK_SIGNATURE(Mask, toXML, QString (Mask::*)() const);
}

void ColorizeMaskSchemaContractTest::transparencyMaskTypeAndSelectionSchemaRemainStable()
{
    using Mask = TransparencyMask;

    static_assert(std::is_class_v<Mask>);
    static_assert(std::is_base_of_v<Node, Mask>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, KisTransparencyMaskSP>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, KisTransparencyMaskSP, QObject *>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, QString>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, QString, QObject *>);
    static_assert(std::has_virtual_destructor_v<Mask>);
    ASSERT_MASK_SIGNATURE(Mask, selection, Selection * (Mask::*)() const);
    ASSERT_MASK_SIGNATURE(Mask, setSelection, void (Mask::*)(Selection *));
    ASSERT_MASK_SIGNATURE(Mask, type, QString (Mask::*)() const);
}

void ColorizeMaskSchemaContractTest::selectionMaskTypeAndSelectionSchemaRemainStable()
{
    using Mask = SelectionMask;

    static_assert(std::is_class_v<Mask>);
    static_assert(std::is_base_of_v<Node, Mask>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, KisSelectionMaskSP>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, KisSelectionMaskSP, QObject *>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, QString>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, QString, QObject *>);
    static_assert(std::has_virtual_destructor_v<Mask>);
    ASSERT_MASK_SIGNATURE(Mask, selection, Selection * (Mask::*)() const);
    ASSERT_MASK_SIGNATURE(Mask, setSelection, void (Mask::*)(Selection *));
    ASSERT_MASK_SIGNATURE(Mask, type, QString (Mask::*)() const);
}

void ColorizeMaskSchemaContractTest::filterMaskTypeAndFilterSchemaRemainStable()
{
    using Mask = FilterMask;

    static_assert(std::is_class_v<Mask>);
    static_assert(std::is_base_of_v<Node, Mask>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, KisFilterMaskSP>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, KisFilterMaskSP, QObject *>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, QString, Filter &>);
    static_assert(std::is_constructible_v<Mask, KisImageSP, QString, Filter &, QObject *>);
    static_assert(std::has_virtual_destructor_v<Mask>);
    ASSERT_MASK_SIGNATURE(Mask, filter, Filter * (Mask::*)());
    ASSERT_MASK_SIGNATURE(Mask, setFilter, void (Mask::*)(Filter &));
    ASSERT_MASK_SIGNATURE(Mask, type, QString (Mask::*)() const);
}

void ColorizeMaskSchemaContractTest::fileLayerTypeAndFileSchemaRemainStable()
{
    using Layer = FileLayer;

    static_assert(std::is_class_v<Layer>);
    static_assert(std::is_base_of_v<Node, Layer>);
    static_assert(std::is_constructible_v<Layer, KisFileLayerSP>);
    static_assert(std::is_constructible_v<Layer, KisFileLayerSP, QObject *>);
    static_assert(std::is_constructible_v<Layer, KisImageSP>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, QString>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, QString, QString>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, QString, QString, QString>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, QString, QString, QString, QString>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, QString, QString, QString, QString, QObject *>);
    static_assert(std::has_virtual_destructor_v<Layer>);
    ASSERT_MASK_SIGNATURE(Layer, path, QString (Layer::*)() const);
    ASSERT_MASK_SIGNATURE(Layer, resetCache, void (Layer::*)());
    ASSERT_MASK_SIGNATURE(Layer, scalingFilter, QString (Layer::*)() const);
    ASSERT_MASK_SIGNATURE(Layer, scalingMethod, QString (Layer::*)() const);
    ASSERT_MASK_SIGNATURE(Layer, setProperties, void (Layer::*)(QString, QString, QString));
    ASSERT_MASK_SIGNATURE(Layer, type, QString (Layer::*)() const);

    using SetPropertiesWithDefaults = decltype(std::declval<Layer &>().setProperties(std::declval<QString>()));
    using SetPropertiesWithDefaultFilter =
        decltype(std::declval<Layer &>().setProperties(std::declval<QString>(), std::declval<QString>()));
    static_assert(std::is_same_v<SetPropertiesWithDefaults, void>);
    static_assert(std::is_same_v<SetPropertiesWithDefaultFilter, void>);
}

void ColorizeMaskSchemaContractTest::fillLayerTypeAndGeneratorSchemaRemainStable()
{
    using Layer = FillLayer;

    static_assert(std::is_class_v<Layer>);
    static_assert(std::is_base_of_v<Node, Layer>);
    static_assert(std::is_constructible_v<Layer, KisGeneratorLayerSP>);
    static_assert(std::is_constructible_v<Layer, KisGeneratorLayerSP, QObject *>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, KisFilterConfigurationSP, Selection &>);
    static_assert(
        std::is_constructible_v<Layer, KisImageSP, QString, KisFilterConfigurationSP, Selection &, QObject *>);
    static_assert(std::has_virtual_destructor_v<Layer>);
    ASSERT_MASK_SIGNATURE(Layer, filterConfig, InfoObject * (Layer::*)());
    ASSERT_MASK_SIGNATURE(Layer, generatorName, QString (Layer::*)());
    ASSERT_MASK_SIGNATURE(Layer, setGenerator, bool (Layer::*)(const QString &, InfoObject *));
    ASSERT_MASK_SIGNATURE(Layer, type, QString (Layer::*)() const);
}

void ColorizeMaskSchemaContractTest::filterLayerTypeAndFilterSchemaRemainStable()
{
    using Layer = FilterLayer;

    static_assert(std::is_class_v<Layer>);
    static_assert(std::is_base_of_v<Node, Layer>);
    static_assert(std::is_constructible_v<Layer, KisAdjustmentLayerSP>);
    static_assert(std::is_constructible_v<Layer, KisAdjustmentLayerSP, QObject *>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, Filter &, Selection &>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, Filter &, Selection &, QObject *>);
    static_assert(std::has_virtual_destructor_v<Layer>);
    ASSERT_MASK_SIGNATURE(Layer, filter, Filter * (Layer::*)());
    ASSERT_MASK_SIGNATURE(Layer, setFilter, void (Layer::*)(Filter &));
    ASSERT_MASK_SIGNATURE(Layer, type, QString (Layer::*)() const);
}

void ColorizeMaskSchemaContractTest::cloneLayerTypeAndSourceSchemaRemainStable()
{
    using Layer = CloneLayer;

    static_assert(std::is_class_v<Layer>);
    static_assert(std::is_base_of_v<Node, Layer>);
    static_assert(std::is_constructible_v<Layer, KisCloneLayerSP>);
    static_assert(std::is_constructible_v<Layer, KisCloneLayerSP, QObject *>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, KisLayerSP>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, KisLayerSP, QObject *>);
    static_assert(std::has_virtual_destructor_v<Layer>);
    ASSERT_MASK_SIGNATURE(Layer, setSourceNode, void (Layer::*)(Node *));
    ASSERT_MASK_SIGNATURE(Layer, sourceNode, Node * (Layer::*)() const);
    ASSERT_MASK_SIGNATURE(Layer, type, QString (Layer::*)() const);
}

void ColorizeMaskSchemaContractTest::groupLayerTypeAndCompositionSchemaRemainStable()
{
    using Layer = GroupLayer;

    static_assert(std::is_class_v<Layer>);
    static_assert(std::is_base_of_v<Node, Layer>);
    static_assert(std::is_constructible_v<Layer, KisGroupLayerSP>);
    static_assert(std::is_constructible_v<Layer, KisGroupLayerSP, QObject *>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString>);
    static_assert(std::is_constructible_v<Layer, KisImageSP, QString, QObject *>);
    static_assert(std::has_virtual_destructor_v<Layer>);
    ASSERT_MASK_SIGNATURE(Layer, passThroughMode, bool (Layer::*)() const);
    ASSERT_MASK_SIGNATURE(Layer, setPassThroughMode, void (Layer::*)(bool));
    ASSERT_MASK_SIGNATURE(Layer, type, QString (Layer::*)() const);
}

QTEST_GUILESS_MAIN(ColorizeMaskSchemaContractTest)
#include "ColorizeMaskSchemaContractTest.moc"
