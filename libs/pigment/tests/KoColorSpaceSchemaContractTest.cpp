/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorSpace.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_KO_COLOR_SPACE_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoColorSpace::method)), signature>)
} // namespace

class KoColorSpaceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSpaceIdentitySignaturesRemainStable();
    void channelTopologySignaturesRemainStable();
    void channelRepresentationSignaturesRemainStable();
    void profileCompatibilityAndIndependencePolicyRemainStable();
    void gamutMetadataSignaturesRemainStable();
    void pixelOpacityAndTransparencySignaturesRemainStable();
    void alphaMaskSignaturesRemainStable();
    void grayBrushColoringSignaturesRemainStable();
    void pixelToneAdjustmentSignaturesRemainStable();
    void pixelMeasurementSignaturesRemainStable();
    void colorSpaceOwnershipAndOperatorSchemaRemainsStable();
    void colorSpaceCompositionAndDitherSignaturesRemainStable();
    void colorSpaceConversionAndProofingSignaturesRemainStable();
    void colorSpaceCanonicalRepresentationSignaturesRemainStable();
    void colorSpaceAdjustmentAndXmlSignaturesRemainStable();
};

void KoColorSpaceSchemaContractTest::colorSpaceIdentitySignaturesRemainStable()
{
    static_assert(std::is_class_v<KoColorSpace>);
    static_assert(std::is_abstract_v<KoColorSpace>);
    static_assert(!std::is_destructible_v<KoColorSpace>);

    ASSERT_KO_COLOR_SPACE_SIGNATURE(id, QString (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(name, QString (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(colorModelId, KoID (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(colorDepthId, KoID (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(operator!=, bool (KoColorSpace::*)(const KoColorSpace &) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(operator==, bool (KoColorSpace::*)(const KoColorSpace &) const);
}

void KoColorSpaceSchemaContractTest::channelTopologySignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(alphaPos, quint32 (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(channelCount, quint32 (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(channelFlags, QBitArray (KoColorSpace::*)(bool, bool) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(channels, QList<KoChannelInfo *> (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(colorChannelCount, quint32 (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(pixelSize, quint32 (KoColorSpace::*)() const);

    static_assert(std::is_same_v<decltype(std::declval<const KoColorSpace &>().channelFlags()), QBitArray>);
}

void KoColorSpaceSchemaContractTest::channelRepresentationSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(channelValueText, QString (KoColorSpace::*)(const quint8 *, quint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fromNormalisedChannelsValue,
                                    void (KoColorSpace::*)(quint8 *, const QVector<float> &) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(normalisedChannelValueText,
                                    QString (KoColorSpace::*)(const quint8 *, quint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(normalisedChannelsValue,
                                    void (KoColorSpace::*)(const quint8 *, QVector<float> &) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(scaleToU8, quint8 (KoColorSpace::*)(const quint8 *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(singleChannelPixel,
                                    void (KoColorSpace::*)(quint8 *, const quint8 *, quint32) const);
}

void KoColorSpaceSchemaContractTest::profileCompatibilityAndIndependencePolicyRemainStable()
{
    static_assert(std::is_enum_v<ColorSpaceIndependence>);
    static_assert(FULLY_INDEPENDENT == 0);
    static_assert(TO_LAB16 == 1);
    static_assert(TO_RGBA8 == 2);
    static_assert(TO_RGBA16 == 3);

    ASSERT_KO_COLOR_SPACE_SIGNATURE(hasHighDynamicRange, bool (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(profile, const KoColorProfile *(KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(profileIsCompatible, bool (KoColorSpace::*)(const KoColorProfile *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(willDegrade, bool (KoColorSpace::*)(ColorSpaceIndependence) const);
}

void KoColorSpaceSchemaContractTest::gamutMetadataSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(estimatedTRCXYY, QPolygonF (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(gamutXYY, QPolygonF (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(lumaCoefficients, QVector<qreal> (KoColorSpace::*)() const);
}

void KoColorSpaceSchemaContractTest::pixelOpacityAndTransparencySignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(copyOpacityU8, void (KoColorSpace::*)(quint8 *, quint8 *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(multiplyAlpha, void (KoColorSpace::*)(quint8 *, quint8, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(opacityF, qreal (KoColorSpace::*)(const quint8 *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(opacityU8, quint8 (KoColorSpace::*)(const quint8 *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(setOpacity, void (KoColorSpace::*)(quint8 *, qreal, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(setOpacity, void (KoColorSpace::*)(quint8 *, quint8, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(transparentColor, void (KoColorSpace::*)(quint8 *, quint32) const);
}

void KoColorSpaceSchemaContractTest::alphaMaskSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(applyAlphaNormedFloatMask,
                                    void (KoColorSpace::*)(quint8 *, const float *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(applyAlphaU8Mask, void (KoColorSpace::*)(quint8 *, const quint8 *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(applyInverseAlphaU8Mask,
                                    void (KoColorSpace::*)(quint8 *, const quint8 *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(applyInverseNormedFloatMask,
                                    void (KoColorSpace::*)(quint8 *, const float *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fillInverseAlphaNormedFloatMaskWithColor,
                                    void (KoColorSpace::*)(quint8 *, const float *, const quint8 *, qint32) const);
}

void KoColorSpaceSchemaContractTest::grayBrushColoringSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fillGrayBrushWithColor,
                                    void (KoColorSpace::*)(quint8 *, const QRgb *, quint8 *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fillGrayBrushWithColorAndLightnessOverlay,
                                    void (KoColorSpace::*)(quint8 *, const QRgb *, quint8 *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fillGrayBrushWithColorAndLightnessWithStrength,
                                    void (KoColorSpace::*)(quint8 *, const QRgb *, quint8 *, qreal, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(modulateLightnessByGrayBrush,
                                    void (KoColorSpace::*)(quint8 *, const QRgb *, qreal, qint32) const);
}

void KoColorSpaceSchemaContractTest::pixelToneAdjustmentSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(decreaseHue, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(decreaseLuminosity, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(decreaseSaturation, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(increaseBlue, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(increaseGreen, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(increaseHue, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(increaseLuminosity, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(increaseRed, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(increaseSaturation, void (KoColorSpace::*)(quint8 *, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(increaseYellow, void (KoColorSpace::*)(quint8 *, qreal) const);
}

void KoColorSpaceSchemaContractTest::pixelMeasurementSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(difference, quint8 (KoColorSpace::*)(const quint8 *, const quint8 *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(differenceA, quint8 (KoColorSpace::*)(const quint8 *, const quint8 *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(intensity8, quint8 (KoColorSpace::*)(const quint8 *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(intensityF, qreal (KoColorSpace::*)(const quint8 *) const);
}

void KoColorSpaceSchemaContractTest::colorSpaceOwnershipAndOperatorSchemaRemainsStable()
{
    static_assert(std::is_enum_v<Deletability>);
    static_assert(OwnedByRegistryDoNotDelete == 0);
    static_assert(OwnedByRegistryRegistryDeletes == 1);
    static_assert(NotOwnedByRegistry == 2);
    static_assert(
        !std::is_constructible_v<KoColorSpace, const QString &, const QString &, KoMixColorsOp *, KoConvolutionOp *>);
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const KoColorSpace *)>(&operator<<)),
                                 QDebug (*)(QDebug, const KoColorSpace *)>);
}

void KoColorSpaceSchemaContractTest::colorSpaceCompositionAndDitherSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(addCompositeOp, void (KoColorSpace::*)(const KoCompositeOp *));
    ASSERT_KO_COLOR_SPACE_SIGNATURE(addDitherOp, void (KoColorSpace::*)(KisDitherOp *));
    ASSERT_KO_COLOR_SPACE_SIGNATURE(bitBlt,
                                    void (KoColorSpace::*)(const KoColorSpace *,
                                                           const KoCompositeOp::ParameterInfo &,
                                                           const KoCompositeOp *,
                                                           KoColorConversionTransformation::Intent,
                                                           KoColorConversionTransformation::ConversionFlags) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(compositeOp,
                                    const KoCompositeOp *(KoColorSpace::*)(const QString &, const KoColorSpace *)
                                        const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(compositeOps, QList<KoCompositeOp *> (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(convolutionOp, KoConvolutionOp * (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(ditherOp, const KisDitherOp *(KoColorSpace::*)(const QString &, DitherType) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(hasCompositeOp,
                                    bool (KoColorSpace::*)(const QString &, const KoColorSpace *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(mixColorsOp, KoMixColorsOp * (KoColorSpace::*)() const);

    static_assert(
        std::is_same_v<decltype(std::declval<const KoColorSpace &>().compositeOp(QString())), const KoCompositeOp *>);
    static_assert(std::is_same_v<decltype(std::declval<const KoColorSpace &>().hasCompositeOp(QString())), bool>);
}

void KoColorSpaceSchemaContractTest::colorSpaceConversionAndProofingSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(convertChannelToVisualRepresentation,
                                    void (KoColorSpace::*)(const quint8 *, quint8 *, quint32, QBitArray) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(convertChannelToVisualRepresentation,
                                    void (KoColorSpace::*)(const quint8 *, quint8 *, quint32, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(convertPixelsTo,
                                    bool (KoColorSpace::*)(const quint8 *,
                                                           quint8 *,
                                                           const KoColorSpace *,
                                                           quint32,
                                                           KoColorConversionTransformation::Intent,
                                                           KoColorConversionTransformation::ConversionFlags) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(convertToQImage,
                                    QImage (KoColorSpace::*)(const quint8 *,
                                                             qint32,
                                                             qint32,
                                                             const KoColorProfile *,
                                                             KoColorConversionTransformation::Intent,
                                                             KoColorConversionTransformation::ConversionFlags) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(createColorConverter,
                                    KoColorConversionTransformation
                                        * (KoColorSpace::*)(const KoColorSpace *,
                                                            KoColorConversionTransformation::Intent,
                                                            KoColorConversionTransformation::ConversionFlags) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(createProofingTransform,
                                    KoColorConversionTransformation
                                        * (KoColorSpace::*)(const KoColorSpace *,
                                                            const KoColorSpace *,
                                                            KoColorConversionTransformation::Intent,
                                                            KoColorConversionTransformation::Intent,
                                                            bool,
                                                            quint8 *,
                                                            KoColorConversionTransformation::ConversionFlags) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(
        proofPixelsTo,
        bool (KoColorSpace::*)(const quint8 *, quint8 *, quint32, KoColorConversionTransformation *) const);
}

void KoColorSpaceSchemaContractTest::colorSpaceCanonicalRepresentationSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fromHSY, QVector<double> (KoColorSpace::*)(qreal *, qreal *, qreal *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fromLabA16, void (KoColorSpace::*)(const quint8 *, quint8 *, quint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fromQColor, void (KoColorSpace::*)(const QColor &, quint8 *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fromRgbA16, void (KoColorSpace::*)(const quint8 *, quint8 *, quint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fromYUV, QVector<double> (KoColorSpace::*)(qreal *, qreal *, qreal *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(toHSY,
                                    void (KoColorSpace::*)(const QVector<double> &, qreal *, qreal *, qreal *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(toLabA16, void (KoColorSpace::*)(const quint8 *, quint8 *, quint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(toQColor, void (KoColorSpace::*)(const quint8 *, QColor *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(toQColor16, void (KoColorSpace::*)(const quint8 *, QColor *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(toRgbA16, void (KoColorSpace::*)(const quint8 *, quint8 *, quint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(toYUV,
                                    void (KoColorSpace::*)(const QVector<double> &, qreal *, qreal *, qreal *) const);
}

void KoColorSpaceSchemaContractTest::colorSpaceAdjustmentAndXmlSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(createBrightnessContrastAdjustment,
                                    KoColorTransformation * (KoColorSpace::*)(const quint16 *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(createColorTransformation,
                                    KoColorTransformation
                                        * (KoColorSpace::*)(const QString &, const QHash<QString, QVariant> &) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(createDarkenAdjustment,
                                    KoColorTransformation * (KoColorSpace::*)(qint32, bool, qreal) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(createInvertTransformation, KoColorTransformation * (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(createPerChannelAdjustment,
                                    KoColorTransformation * (KoColorSpace::*)(const quint16 *const *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(colorFromXML, void (KoColorSpace::*)(quint8 *, const QDomElement &) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(colorToXML,
                                    void (KoColorSpace::*)(const quint8 *, QDomDocument &, QDomElement &) const);
}

QTEST_GUILESS_MAIN(KoColorSpaceSchemaContractTest)

#include "KoColorSpaceSchemaContractTest.moc"
