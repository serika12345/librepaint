/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_transform_mask.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_TRANSFORM_MASK_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisTransformMask::method)), signature>)

} // namespace

class KisTransformMaskSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void transformMaskTypeLifetimeAndVisitorSchemaRemainStable();
    void transformMaskDeviceAndGeometrySignaturesRemainStable();
    void transformMaskParametersAndCacheSignaturesRemainStable();
    void transformMaskPositionAndDelayedUpdateSignaturesRemainStable();
    void transformMaskLodTestingAndNotificationSignaturesRemainStable();
};

void KisTransformMaskSchemaContractTest::transformMaskTypeLifetimeAndVisitorSchemaRemainStable()
{
    static_assert(std::is_class_v<KisTransformMask>);
    static_assert(std::is_base_of_v<KisEffectMask, KisTransformMask>);
    static_assert(std::is_base_of_v<KisDelayedUpdateNodeInterface, KisTransformMask>);
    static_assert(std::is_constructible_v<KisTransformMask, KisImageWSP, const QString &>);
    static_assert(std::is_copy_constructible_v<KisTransformMask>);
    static_assert(std::has_virtual_destructor_v<KisTransformMask>);
    ASSERT_TRANSFORM_MASK_SIGNATURE(clone, KisNodeSP (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(accept, bool (KisTransformMask::*)(KisNodeVisitor &));
    ASSERT_TRANSFORM_MASK_SIGNATURE(accept, void (KisTransformMask::*)(KisProcessingVisitor &, KisUndoAdapter *));
    ASSERT_TRANSFORM_MASK_SIGNATURE(icon, QIcon (KisTransformMask::*)() const);

    QVERIFY(true);
}

void KisTransformMaskSchemaContractTest::transformMaskDeviceAndGeometrySignaturesRemainStable()
{
    ASSERT_TRANSFORM_MASK_SIGNATURE(paintDevice, KisPaintDeviceSP (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(decorateRect,
                                    QRect (KisTransformMask::*)(KisPaintDeviceSP &,
                                                                KisPaintDeviceSP &,
                                                                const QRect &,
                                                                KisNode::PositionToFilthy,
                                                                KisRenderPassFlags) const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(changeRect,
                                    QRect (KisTransformMask::*)(const QRect &, KisNode::PositionToFilthy) const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(needRect,
                                    QRect (KisTransformMask::*)(const QRect &, KisNode::PositionToFilthy) const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(extent, QRect (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(exactBounds, QRect (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(sourceDataBounds, QRect (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(setImage, void (KisTransformMask::*)(KisImageWSP));
    static_assert(
        std::is_same_v<decltype(std::declval<const KisTransformMask &>().changeRect(std::declval<const QRect &>())),
                       QRect>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisTransformMask &>().needRect(std::declval<const QRect &>())),
                       QRect>);

    QVERIFY(true);
}

void KisTransformMaskSchemaContractTest::transformMaskParametersAndCacheSignaturesRemainStable()
{
    ASSERT_TRANSFORM_MASK_SIGNATURE(setTransformParamsWithUndo,
                                    void (KisTransformMask::*)(KisTransformMaskParamsInterfaceSP, KUndo2Command *));
    ASSERT_TRANSFORM_MASK_SIGNATURE(setTransformParams, void (KisTransformMask::*)(KisTransformMaskParamsInterfaceSP));
    ASSERT_TRANSFORM_MASK_SIGNATURE(transformParams, KisTransformMaskParamsInterfaceSP (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(staticImageCacheIsValid, bool (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(recalculateStaticImage, void (KisTransformMask::*)());
    ASSERT_TRANSFORM_MASK_SIGNATURE(buildPreviewDevice, KisPaintDeviceSP (KisTransformMask::*)());
    ASSERT_TRANSFORM_MASK_SIGNATURE(buildSourcePreviewDevice, KisPaintDeviceSP (KisTransformMask::*)());
    ASSERT_TRANSFORM_MASK_SIGNATURE(overrideStaticCacheDevice, void (KisTransformMask::*)(KisPaintDeviceSP));

    QVERIFY(true);
}

void KisTransformMaskSchemaContractTest::transformMaskPositionAndDelayedUpdateSignaturesRemainStable()
{
    ASSERT_TRANSFORM_MASK_SIGNATURE(x, qint32 (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(y, qint32 (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(setX, void (KisTransformMask::*)(qint32));
    ASSERT_TRANSFORM_MASK_SIGNATURE(setY, void (KisTransformMask::*)(qint32));
    ASSERT_TRANSFORM_MASK_SIGNATURE(forceUpdateTimedNode, void (KisTransformMask::*)());
    ASSERT_TRANSFORM_MASK_SIGNATURE(hasPendingTimedUpdates, bool (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(threadSafeForceStaticImageUpdate, void (KisTransformMask::*)(const QRect &));
    ASSERT_TRANSFORM_MASK_SIGNATURE(threadSafeForceStaticImageUpdate, void (KisTransformMask::*)());

    QVERIFY(true);
}

void KisTransformMaskSchemaContractTest::transformMaskLodTestingAndNotificationSignaturesRemainStable()
{
    ASSERT_TRANSFORM_MASK_SIGNATURE(syncLodCache, void (KisTransformMask::*)());
    ASSERT_TRANSFORM_MASK_SIGNATURE(getLodCapableDevices, KisPaintDeviceList (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(setTestingInterface,
                                    void (KisTransformMask::*)(KisTransformMaskTestingInterface *));
    ASSERT_TRANSFORM_MASK_SIGNATURE(testingInterface, KisTransformMaskTestingInterface * (KisTransformMask::*)() const);
    ASSERT_TRANSFORM_MASK_SIGNATURE(sigInternalForceStaticImageUpdate, void (KisTransformMask::*)());

    QVERIFY(true);
}

#undef ASSERT_TRANSFORM_MASK_SIGNATURE

QTEST_GUILESS_MAIN(KisTransformMaskSchemaContractTest)

#include "KisTransformMaskSchemaContractTest.moc"
