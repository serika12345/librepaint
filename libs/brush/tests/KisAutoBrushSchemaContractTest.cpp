/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_auto_brush.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_AUTO_BRUSH_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAutoBrush::method)), signature>)
} // namespace

class KisAutoBrushSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndResourceIoSchemaRemainStable();
    void maskGeneratorAndAttributeSchemaRemainStable();
    void geometryAndOutlineSchemaRemainStable();
    void dabRenderingSchemaRemainStable();
    void initializationAndSerializationSchemaRemainStable();
};

void KisAutoBrushSchemaContractTest::typeConstructionAndResourceIoSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAutoBrush>);
    static_assert(std::is_constructible_v<KisAutoBrush, KisMaskGenerator *, qreal, qreal>);
    static_assert(std::is_constructible_v<KisAutoBrush, KisMaskGenerator *, qreal, qreal, qreal>);
    static_assert(std::is_copy_constructible_v<KisAutoBrush>);
    static_assert(!std::is_copy_assignable_v<KisAutoBrush>);
    static_assert(std::has_virtual_destructor_v<KisAutoBrush>);
    ASSERT_AUTO_BRUSH_SIGNATURE(clone, KoResourceSP (KisAutoBrush::*)() const);
    ASSERT_AUTO_BRUSH_SIGNATURE(loadFromDevice, bool (KisAutoBrush::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_AUTO_BRUSH_SIGNATURE(saveToDevice, bool (KisAutoBrush::*)(QIODevice *) const);
}

void KisAutoBrushSchemaContractTest::maskGeneratorAndAttributeSchemaRemainStable()
{
    ASSERT_AUTO_BRUSH_SIGNATURE(maskGenerator, const KisMaskGenerator *(KisAutoBrush::*)() const);
    ASSERT_AUTO_BRUSH_SIGNATURE(randomness, qreal (KisAutoBrush::*)() const);
    ASSERT_AUTO_BRUSH_SIGNATURE(density, qreal (KisAutoBrush::*)() const);
    ASSERT_AUTO_BRUSH_SIGNATURE(userEffectiveSize, qreal (KisAutoBrush::*)() const);
    ASSERT_AUTO_BRUSH_SIGNATURE(setUserEffectiveSize, void (KisAutoBrush::*)(qreal));
    ASSERT_AUTO_BRUSH_SIGNATURE(isEphemeral, bool (KisAutoBrush::*)() const);
    ASSERT_AUTO_BRUSH_SIGNATURE(isPiercedApprox, bool (KisAutoBrush::*)() const);
    ASSERT_AUTO_BRUSH_SIGNATURE(supportsCaching, bool (KisAutoBrush::*)() const);
}

void KisAutoBrushSchemaContractTest::geometryAndOutlineSchemaRemainStable()
{
    using MaskSizeSignature =
        qint32 (KisAutoBrush::*)(const KisDabShape &, qreal, qreal, const KisPaintInformation &) const;

    ASSERT_AUTO_BRUSH_SIGNATURE(maskWidth, MaskSizeSignature);
    ASSERT_AUTO_BRUSH_SIGNATURE(maskHeight, MaskSizeSignature);
    ASSERT_AUTO_BRUSH_SIGNATURE(characteristicSize, QSizeF (KisAutoBrush::*)(const KisDabShape &) const);
    ASSERT_AUTO_BRUSH_SIGNATURE(outline, KisOptimizedBrushOutline (KisAutoBrush::*)(bool) const);
    ASSERT_AUTO_BRUSH_SIGNATURE(outlineSourceImage, KisFixedPaintDeviceSP (KisAutoBrush::*)() const);
}

void KisAutoBrushSchemaContractTest::dabRenderingSchemaRemainStable()
{
    using PaintDeviceSignature = KisFixedPaintDeviceSP (
        KisAutoBrush::*)(const KoColorSpace *, const KisDabShape &, const KisPaintInformation &, double, double) const;
    using GenerateDabSignature = void (KisAutoBrush::*)(KisFixedPaintDeviceSP,
                                                        KisBrush::ColoringInformation *,
                                                        const KisDabShape &,
                                                        const KisPaintInformation &,
                                                        double,
                                                        double,
                                                        qreal,
                                                        qreal) const;

    ASSERT_AUTO_BRUSH_SIGNATURE(paintDevice, PaintDeviceSignature);
    ASSERT_AUTO_BRUSH_SIGNATURE(generateMaskAndApplyMaskOrCreateDab, GenerateDabSignature);
}

void KisAutoBrushSchemaContractTest::initializationAndSerializationSchemaRemainStable()
{
    ASSERT_AUTO_BRUSH_SIGNATURE(notifyBrushIsGoingToBeClonedForStroke, void (KisAutoBrush::*)());
    ASSERT_AUTO_BRUSH_SIGNATURE(coldInitBrush, void (KisAutoBrush::*)());
    ASSERT_AUTO_BRUSH_SIGNATURE(toXML, void (KisAutoBrush::*)(QDomDocument &, QDomElement &) const);
    ASSERT_AUTO_BRUSH_SIGNATURE(lodLimitations, void (KisAutoBrush::*)(KisPaintopLodLimitations *) const);
}

QTEST_GUILESS_MAIN(KisAutoBrushSchemaContractTest)

#include "KisAutoBrushSchemaContractTest.moc"
