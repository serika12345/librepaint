/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_mask.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_MASK_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMask::method)), signature>)

class MaskConstructorProbe final : public KisMask
{
public:
    using KisMask::KisMask;

    explicit MaskConstructorProbe(const KisMask &rhs)
        : KisMask(rhs)
    {
    }

    KisNodeSP clone() const override
    {
        return nullptr;
    }
};

} // namespace

class KisMaskSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void maskTypeAndConstructionSchemaRemainStable();
    void maskHierarchyAndSelectionSignaturesRemainStable();
    void maskDeviceAndProjectionSignaturesRemainStable();
    void maskGeometryAndThumbnailSignaturesRemainStable();
    void maskPositionAndEditingSignaturesRemainStable();
};

void KisMaskSchemaContractTest::maskTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMask>);
    static_assert(std::is_abstract_v<KisMask>);
    static_assert(std::is_base_of_v<KisNode, KisMask>);
    static_assert(std::is_base_of_v<KisIndirectPaintingSupport, KisMask>);
    static_assert(std::is_constructible_v<MaskConstructorProbe, KisImageWSP, const QString &>);
    static_assert(std::is_constructible_v<MaskConstructorProbe, const KisMask &>);
    static_assert(std::has_virtual_destructor_v<KisMask>);

    QVERIFY(true);
}

void KisMaskSchemaContractTest::maskHierarchyAndSelectionSignaturesRemainStable()
{
    ASSERT_MASK_SIGNATURE(allowAsChild, bool (KisMask::*)(KisNodeSP) const);
    ASSERT_MASK_SIGNATURE(initSelection, void (KisMask::*)(KisLayerSP));
    ASSERT_MASK_SIGNATURE(initSelection, void (KisMask::*)(KisPaintDeviceSP, KisLayerSP));
    ASSERT_MASK_SIGNATURE(initSelection, void (KisMask::*)(KisSelectionSP, KisLayerSP));
    ASSERT_MASK_SIGNATURE(selection, KisSelectionSP (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(setImage, void (KisMask::*)(KisImageWSP));
    ASSERT_MASK_SIGNATURE(setSelection, void (KisMask::*)(KisSelectionSP));
    ASSERT_MASK_SIGNATURE(testingInitSelection, void (KisMask::*)(const QRect &, KisLayerSP));

    QVERIFY(true);
}

void KisMaskSchemaContractTest::maskDeviceAndProjectionSignaturesRemainStable()
{
    ASSERT_MASK_SIGNATURE(colorSpace, const KoColorSpace *(KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(compositeOp, const KoCompositeOp *(KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(original, KisPaintDeviceSP (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(paintDevice, KisPaintDeviceSP (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(projection, KisPaintDeviceSP (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(projectionPlane, KisAbstractProjectionPlaneSP (KisMask::*)() const);

    QVERIFY(true);
}

void KisMaskSchemaContractTest::maskGeometryAndThumbnailSignaturesRemainStable()
{
    ASSERT_MASK_SIGNATURE(changeRect, QRect (KisMask::*)(const QRect &, KisNode::PositionToFilthy) const);
    ASSERT_MASK_SIGNATURE(createThumbnail,
                          QImage (KisMask::*)(qint32, qint32, Qt::AspectRatioMode, KisThumbnailBoundsMode));
    ASSERT_MASK_SIGNATURE(exactBounds, QRect (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(extent, QRect (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(needRect, QRect (KisMask::*)(const QRect &, KisNode::PositionToFilthy) const);
    ASSERT_MASK_SIGNATURE(nonDependentExtent, QRect (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(supportsLodPainting, bool (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(thumbnailSeqNo, int (KisMask::*)() const);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisMask &>().changeRect(std::declval<const QRect &>())), QRect>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisMask &>().needRect(std::declval<const QRect &>())), QRect>);

    QVERIFY(true);
}

void KisMaskSchemaContractTest::maskPositionAndEditingSignaturesRemainStable()
{
    ASSERT_MASK_SIGNATURE(select, void (KisMask::*)(const QRect &, quint8));
    ASSERT_MASK_SIGNATURE(setX, void (KisMask::*)(qint32));
    ASSERT_MASK_SIGNATURE(setY, void (KisMask::*)(qint32));
    ASSERT_MASK_SIGNATURE(x, qint32 (KisMask::*)() const);
    ASSERT_MASK_SIGNATURE(y, qint32 (KisMask::*)() const);
    static_assert(std::is_same_v<decltype(std::declval<KisMask &>().select(std::declval<const QRect &>())), void>);

    QVERIFY(true);
}

#undef ASSERT_MASK_SIGNATURE

QTEST_GUILESS_MAIN(KisMaskSchemaContractTest)

#include "KisMaskSchemaContractTest.moc"
