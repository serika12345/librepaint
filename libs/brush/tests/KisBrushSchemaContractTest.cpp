/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_brush.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_BRUSH_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBrush::method)), signature>)
} // namespace

class BrushConstructionProbe final : public KisBrush
{
public:
    BrushConstructionProbe()
        : KisBrush()
    {
    }

    explicit BrushConstructionProbe(const QString &filename)
        : KisBrush(filename)
    {
    }

    BrushConstructionProbe(const BrushConstructionProbe &rhs)
        : KisBrush(rhs)
    {
    }

    KoResourceSP clone() const override;
    bool loadFromDevice(QIODevice *, KisResourcesInterfaceSP) override;
    qreal userEffectiveSize() const override;
    void setUserEffectiveSize(qreal) override;
};

class KisBrushSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void brushIdentityAndTypeSchemaRemainsStable();
    void brushColoringConstantsAndMetadataSchemaRemainsStable();
    void brushSizeAndAngleSignaturesRemainStable();
    void brushSpacingPolicySignaturesRemainStable();
    void brushApplicationPolicySignaturesRemainStable();
    void brushLifetimeAndConstructionSignaturesRemainStable();
    void brushTipGeometryAndOutlineSignaturesRemainStable();
    void brushDabAndMaskGenerationSignaturesRemainStable();
    void brushStrokePreparationAndCacheSignaturesRemainStable();
    void brushSerializationAndGradientSignaturesRemainStable();
};

void KisBrushSchemaContractTest::brushIdentityAndTypeSchemaRemainsStable()
{
    static_assert(std::is_same_v<KisBrushSP, QSharedPointer<KisBrush>>);
    static_assert(std::is_same_v<KoAbstractGradientSP, QSharedPointer<KoAbstractGradient>>);
    static_assert(std::is_class_v<KisBrush>);
    static_assert(std::is_class_v<KisBrush::ColoringInformation>);
    static_assert(std::is_enum_v<enumBrushType>);
    static_assert(INVALID == 0);
    static_assert(MASK == 1);
    static_assert(IMAGE == 2);
    static_assert(PIPE_MASK == 3);
    static_assert(PIPE_IMAGE == 4);
    static_assert(std::is_enum_v<enumBrushApplication>);
    static_assert(ALPHAMASK == 0);
    static_assert(IMAGESTAMP == 1);
    static_assert(LIGHTNESSMAP == 2);
    static_assert(GRADIENTMAP == 3);
}

void KisBrushSchemaContractTest::brushColoringConstantsAndMetadataSchemaRemainsStable()
{
    using ColoringInformation = KisBrush::ColoringInformation;

    static_assert(std::is_same_v<decltype(KisBrush::brushTypeMetaDataKey), const QString>);
    static_assert(std::is_same_v<decltype(DEFAULT_LIGHTNESS_STRENGTH), const qreal>);
    static_assert(std::is_same_v<decltype(DEFAULT_SOFTNESS_FACTOR), const qreal>);
    static_assert(std::has_virtual_destructor_v<ColoringInformation>);
    static_assert(
        std::is_same_v<decltype(&ColoringInformation::color), const quint8 *(ColoringInformation::*)() const>);
    static_assert(std::is_same_v<decltype(&ColoringInformation::nextColumn), void (ColoringInformation::*)()>);
    static_assert(std::is_same_v<decltype(&ColoringInformation::nextRow), void (ColoringInformation::*)()>);
}

void KisBrushSchemaContractTest::brushSizeAndAngleSignaturesRemainStable()
{
    ASSERT_BRUSH_SIGNATURE(width, qint32 (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(height, qint32 (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(userEffectiveSize, qreal (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setUserEffectiveSize, void (KisBrush::*)(qreal));
    ASSERT_BRUSH_SIGNATURE(scale, qreal (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setScale, void (KisBrush::*)(qreal));
    ASSERT_BRUSH_SIGNATURE(angle, qreal (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setAngle, void (KisBrush::*)(qreal));
}

void KisBrushSchemaContractTest::brushSpacingPolicySignaturesRemainStable()
{
    ASSERT_BRUSH_SIGNATURE(spacing, double (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setSpacing, void (KisBrush::*)(double));
    ASSERT_BRUSH_SIGNATURE(autoSpacingActive, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(autoSpacingCoeff, qreal (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setAutoSpacing, void (KisBrush::*)(bool, qreal));
}

void KisBrushSchemaContractTest::brushApplicationPolicySignaturesRemainStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KisBrush::*)() const;

    ASSERT_BRUSH_SIGNATURE(brushType, enumBrushType (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(brushApplication, enumBrushApplication (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setBrushApplication, void (KisBrush::*)(enumBrushApplication));
    ASSERT_BRUSH_SIGNATURE(applyingGradient, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(preserveLightness, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(supportsCaching, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(isPiercedApprox, bool (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(resourceType, ResourceTypeSignature);
}

void KisBrushSchemaContractTest::brushLifetimeAndConstructionSignaturesRemainStable()
{
    using Probe = BrushConstructionProbe;

    static_assert(std::is_default_constructible_v<Probe>);
    static_assert(std::is_copy_constructible_v<Probe>);
    static_assert(std::is_constructible_v<Probe, const QString &>);
    static_assert(!std::is_copy_assignable_v<KisBrush>);
    static_assert(std::has_virtual_destructor_v<KisBrush>);
}

void KisBrushSchemaContractTest::brushTipGeometryAndOutlineSignaturesRemainStable()
{
    ASSERT_BRUSH_SIGNATURE(brushTipImage, QImage (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(characteristicSize, QSizeF (KisBrush::*)(const KisDabShape &) const);
    ASSERT_BRUSH_SIGNATURE(hotSpot, QPointF (KisBrush::*)(const KisDabShape &, const KisPaintInformation &) const);
    ASSERT_BRUSH_SIGNATURE(maskAngle, double (KisBrush::*)(double) const);
    ASSERT_BRUSH_SIGNATURE(maskHeight,
                           qint32 (KisBrush::*)(const KisDabShape &, qreal, qreal, const KisPaintInformation &) const);
    ASSERT_BRUSH_SIGNATURE(maskWidth,
                           qint32 (KisBrush::*)(const KisDabShape &, qreal, qreal, const KisPaintInformation &) const);
    ASSERT_BRUSH_SIGNATURE(outline, KisOptimizedBrushOutline (KisBrush::*)(bool) const);
    ASSERT_BRUSH_SIGNATURE(outlineSourceImage, KisFixedPaintDeviceSP (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(setBrushTipImage, void (KisBrush::*)(const QImage &));

    static_assert(std::is_same_v<decltype(std::declval<const KisBrush &>().maskAngle()), double>);
    static_assert(std::is_same_v<decltype(std::declval<const KisBrush &>().outline()), KisOptimizedBrushOutline>);
}

void KisBrushSchemaContractTest::brushDabAndMaskGenerationSignaturesRemainStable()
{
    using GenerateMaskSignature = void (KisBrush::*)(KisFixedPaintDeviceSP,
                                                     KisBrush::ColoringInformation *,
                                                     const KisDabShape &,
                                                     const KisPaintInformation &,
                                                     double,
                                                     double,
                                                     qreal,
                                                     qreal) const;
    using GenerateMaskWithDefaultsSignature = void (KisBrush::*)(KisFixedPaintDeviceSP,
                                                                 KisBrush::ColoringInformation *,
                                                                 const KisDabShape &,
                                                                 const KisPaintInformation &,
                                                                 double,
                                                                 double,
                                                                 qreal) const;
    using PaintDeviceMaskSignature = void (KisBrush::*)(KisFixedPaintDeviceSP,
                                                        KisPaintDeviceSP,
                                                        const KisDabShape &,
                                                        const KisPaintInformation &,
                                                        double,
                                                        double,
                                                        qreal,
                                                        qreal) const;
    using ColorMaskSignature = void (KisBrush::*)(KisFixedPaintDeviceSP,
                                                  const KoColor &,
                                                  const KisDabShape &,
                                                  const KisPaintInformation &,
                                                  double,
                                                  double,
                                                  qreal,
                                                  qreal) const;

    ASSERT_BRUSH_SIGNATURE(canPaintFor, bool (KisBrush::*)(const KisPaintInformation &));
    ASSERT_BRUSH_SIGNATURE(generateMaskAndApplyMaskOrCreateDab, GenerateMaskSignature);
    ASSERT_BRUSH_SIGNATURE(generateMaskAndApplyMaskOrCreateDab, GenerateMaskWithDefaultsSignature);
    ASSERT_BRUSH_SIGNATURE(mask, PaintDeviceMaskSignature);
    ASSERT_BRUSH_SIGNATURE(mask, ColorMaskSignature);
    ASSERT_BRUSH_SIGNATURE(
        paintDevice,
        KisFixedPaintDeviceSP (
            KisBrush::*)(const KoColorSpace *, const KisDabShape &, const KisPaintInformation &, double, double) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisBrush &>().generateMaskAndApplyMaskOrCreateDab(
                                     std::declval<KisFixedPaintDeviceSP>(),
                                     std::declval<KisBrush::ColoringInformation *>(),
                                     std::declval<const KisDabShape &>(),
                                     std::declval<const KisPaintInformation &>())),
                                 void>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisBrush &>().mask(std::declval<KisFixedPaintDeviceSP>(),
                                                                      std::declval<KisPaintDeviceSP>(),
                                                                      std::declval<const KisDabShape &>(),
                                                                      std::declval<const KisPaintInformation &>())),
                       void>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisBrush &>().mask(std::declval<KisFixedPaintDeviceSP>(),
                                                                      std::declval<const KoColor &>(),
                                                                      std::declval<const KisDabShape &>(),
                                                                      std::declval<const KisPaintInformation &>())),
                       void>);
    static_assert(std::is_same_v<decltype(std::declval<const KisBrush &>().paintDevice(
                                     std::declval<const KoColorSpace *>(),
                                     std::declval<const KisDabShape &>(),
                                     std::declval<const KisPaintInformation &>())),
                                 KisFixedPaintDeviceSP>);
}

void KisBrushSchemaContractTest::brushStrokePreparationAndCacheSignaturesRemainStable()
{
    ASSERT_BRUSH_SIGNATURE(brushIndex, quint32 (KisBrush::*)() const);
    ASSERT_BRUSH_SIGNATURE(clearBrushPyramid, void (KisBrush::*)());
    ASSERT_BRUSH_SIGNATURE(coldInitBrush, void (KisBrush::*)());
    ASSERT_BRUSH_SIGNATURE(lodLimitations, void (KisBrush::*)(KisPaintopLodLimitations *) const);
    ASSERT_BRUSH_SIGNATURE(notifyBrushIsGoingToBeClonedForStroke, void (KisBrush::*)());
    ASSERT_BRUSH_SIGNATURE(notifyStrokeStarted, void (KisBrush::*)());
    ASSERT_BRUSH_SIGNATURE(prepareForSeqNo, void (KisBrush::*)(const KisPaintInformation &, int));
}

void KisBrushSchemaContractTest::brushSerializationAndGradientSignaturesRemainStable()
{
    using FromXmlSignature = KisBrushSP (*)(const QDomElement &, KisResourcesInterfaceSP);
    using FromXmlLoadResultSignature = KoResourceLoadResult (*)(const QDomElement &, KisResourcesInterfaceSP);

    static_assert(std::is_same_v<decltype(static_cast<FromXmlSignature>(&KisBrush::fromXML)), FromXmlSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FromXmlLoadResultSignature>(&KisBrush::fromXMLLoadResult)),
                                 FromXmlLoadResultSignature>);
    ASSERT_BRUSH_SIGNATURE(setGradient, void (KisBrush::*)(KoAbstractGradientSP));
    ASSERT_BRUSH_SIGNATURE(toXML, void (KisBrush::*)(QDomDocument &, QDomElement &) const);
}

QTEST_GUILESS_MAIN(KisBrushSchemaContractTest)

#include "KisBrushSchemaContractTest.moc"
