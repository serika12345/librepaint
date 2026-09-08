/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <document/kis_file_layer.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_FILE_LAYER_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisFileLayer::method)), signature>)
} // namespace

class KisFileLayerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fileLayerTypeScalingAndConstructionSchemaRemainStable();
    void fileLayerNodeAndVisitorSignaturesRemainStable();
    void fileLayerDeviceAndCacheSignaturesRemainStable();
    void fileLayerPathAndScalingSignaturesRemainStable();
    void fileLayerSectionGeometryAndNotificationSignaturesRemainStable();
};

void KisFileLayerSchemaContractTest::fileLayerTypeScalingAndConstructionSchemaRemainStable()
{
    using Layer = KisFileLayer;

    static_assert(std::is_class_v<Layer>);
    static_assert(std::is_enum_v<Layer::ScalingMethod>);
    static_assert(Layer::None == 0);
    static_assert(Layer::ToImageSize == 1);
    static_assert(Layer::ToImagePPI == 2);
    static_assert(std::is_constructible_v<Layer, KisImageWSP, const QString &, quint8>);
    static_assert(std::is_constructible_v<Layer,
                                          KisImageWSP,
                                          const QString &,
                                          const QString &,
                                          Layer::ScalingMethod,
                                          QString,
                                          const QString &,
                                          quint8>);
    static_assert(std::is_copy_constructible_v<Layer>);
    static_assert(std::has_virtual_destructor_v<Layer>);
}

void KisFileLayerSchemaContractTest::fileLayerNodeAndVisitorSignaturesRemainStable()
{
    using Layer = KisFileLayer;

    ASSERT_FILE_LAYER_SIGNATURE(clone, KisNodeSP (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(allowAsChild, bool (Layer::*)(KisNodeSP) const);
    ASSERT_FILE_LAYER_SIGNATURE(accept, bool (Layer::*)(KisNodeVisitor &));
    ASSERT_FILE_LAYER_SIGNATURE(accept, void (Layer::*)(KisProcessingVisitor &, KisUndoAdapter *));
}

void KisFileLayerSchemaContractTest::fileLayerDeviceAndCacheSignaturesRemainStable()
{
    using Layer = KisFileLayer;

    ASSERT_FILE_LAYER_SIGNATURE(icon, QIcon (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(resetCache, void (Layer::*)(const KoColorSpace *));
    ASSERT_FILE_LAYER_SIGNATURE(original, KisPaintDeviceSP (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(paintDevice, KisPaintDeviceSP (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(setImage, void (Layer::*)(KisImageWSP));

    static_assert(std::is_same_v<decltype(std::declval<Layer &>().resetCache()), void>);
}

void KisFileLayerSchemaContractTest::fileLayerPathAndScalingSignaturesRemainStable()
{
    using Layer = KisFileLayer;

    ASSERT_FILE_LAYER_SIGNATURE(setFileName, void (Layer::*)(const QString &, const QString &));
    ASSERT_FILE_LAYER_SIGNATURE(fileName, QString (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(path, QString (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(scalingMethod, Layer::ScalingMethod (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(setScalingMethod, void (Layer::*)(Layer::ScalingMethod));
    ASSERT_FILE_LAYER_SIGNATURE(scalingFilter, QString (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(setScalingFilter, void (Layer::*)(QString));
}

void KisFileLayerSchemaContractTest::fileLayerSectionGeometryAndNotificationSignaturesRemainStable()
{
    using Layer = KisFileLayer;

    ASSERT_FILE_LAYER_SIGNATURE(sectionModelProperties, KisBaseNode::PropertyList (Layer::*)() const);
    ASSERT_FILE_LAYER_SIGNATURE(setSectionModelProperties, void (Layer::*)(const KisBaseNode::PropertyList &));
    ASSERT_FILE_LAYER_SIGNATURE(crop, KUndo2Command * (Layer::*)(const QRect &));
    ASSERT_FILE_LAYER_SIGNATURE(transform, KUndo2Command * (Layer::*)(const QTransform &));
    ASSERT_FILE_LAYER_SIGNATURE(sigRequestOpenFile, void (Layer::*)());
}

#undef ASSERT_FILE_LAYER_SIGNATURE

QTEST_APPLESS_MAIN(KisFileLayerSchemaContractTest)

#include "KisFileLayerSchemaContractTest.moc"
