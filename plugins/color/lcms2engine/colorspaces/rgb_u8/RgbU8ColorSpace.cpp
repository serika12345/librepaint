/*
 *  SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2004 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "RgbU8ColorSpace.h"

#include <QColor>
#include <QDomElement>

#include <klocalizedstring.h>

#include "LcmsRGBP2020PQColorSpace.h"
#include "compositeops/KoCompositeOps.h"
#include "compositeops/RgbCompositeOpBumpmap.h"
#include "compositeops/RgbCompositeOpIn.h"
#include "compositeops/RgbCompositeOpOut.h"
#include "dithering/KisRgbDitherOpFactory.h"
#include <KoColorConversions.h>
#include <KoColorSpacePreserveLightnessUtils.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfileQuery.h>
#include <KoIntegerMaths.h>
#include <kis_dom_utils.h>

#define downscale(quantum)  (quantum) //((unsigned char) ((quantum)/257UL))
#define upscale(value)  (value) // ((quint8) (257UL*(value)))

RgbU8ColorSpace::RgbU8ColorSpace(const QString &name, KoColorProfile *p) :
    LcmsColorSpace<KoBgrU8Traits>(colorSpaceId(), name, TYPE_BGRA_8, cmsSigRgbData, p)
{
    addChannel(new KoChannelInfo(i18n("Blue"), 0, 2, KoChannelInfo::COLOR, KoChannelInfo::UINT8, 1, QColor(0, 0, 255)));
    addChannel(new KoChannelInfo(i18n("Green"), 1, 1, KoChannelInfo::COLOR, KoChannelInfo::UINT8, 1, QColor(0, 255, 0)));
    addChannel(new KoChannelInfo(i18n("Red"), 2, 0, KoChannelInfo::COLOR, KoChannelInfo::UINT8, 1, QColor(255, 0, 0)));
    addChannel(new KoChannelInfo(i18n("Alpha"), 3, 3, KoChannelInfo::ALPHA, KoChannelInfo::UINT8));

    init();

    addStandardCompositeOps<KoBgrU8Traits>(this);
    addStandardDitherOps<KoBgrU8Traits>(this);

    addCompositeOp(new RgbCompositeOpIn<KoBgrU8Traits>(this));
    addCompositeOp(new RgbCompositeOpOut<KoBgrU8Traits>(this));
    addCompositeOp(new RgbCompositeOpBumpmap<KoBgrU8Traits>(this));
}

KoColorSpace *RgbU8ColorSpace::clone() const
{
    return new RgbU8ColorSpace(name(), profile()->clone());
}

void RgbU8ColorSpace::colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const
{
    const KoBgrU8Traits::Pixel *p = reinterpret_cast<const KoBgrU8Traits::Pixel *>(pixel);
    QDomElement labElt = doc.createElement("RGB");
    labElt.setAttribute("r", KisDomUtils::toString(KoColorSpaceMaths< KoBgrU8Traits::channels_type, qreal>::scaleToA(p->red)));
    labElt.setAttribute("g", KisDomUtils::toString(KoColorSpaceMaths< KoBgrU8Traits::channels_type, qreal>::scaleToA(p->green)));
    labElt.setAttribute("b", KisDomUtils::toString(KoColorSpaceMaths< KoBgrU8Traits::channels_type, qreal>::scaleToA(p->blue)));
    labElt.setAttribute("space", profile()->name());
    colorElt.appendChild(labElt);
}

void RgbU8ColorSpace::colorFromXML(quint8 *pixel, const QDomElement &elt) const
{
    KoBgrU8Traits::Pixel *p = reinterpret_cast<KoBgrU8Traits::Pixel *>(pixel);
    p->red = KoColorSpaceMaths< qreal, KoBgrU8Traits::channels_type >::scaleToA(KisDomUtils::toDouble(elt.attribute("r")));
    p->green = KoColorSpaceMaths< qreal, KoBgrU8Traits::channels_type >::scaleToA(KisDomUtils::toDouble(elt.attribute("g")));
    p->blue = KoColorSpaceMaths< qreal, KoBgrU8Traits::channels_type >::scaleToA(KisDomUtils::toDouble(elt.attribute("b")));
    p->alpha = KoColorSpaceMathsTraits<quint8>::max;
}

quint8 RgbU8ColorSpace::intensity8(const quint8 *src) const
{
    const KoBgrU8Traits::Pixel *p = reinterpret_cast<const KoBgrU8Traits::Pixel *>(src);
    // Integer version of:
    //      static_cast<quint8>(qRound(p->red * 0.30 + p->green * 0.59 + p->blue * 0.11))
    // The "+ 50" is used for rounding
    return static_cast<quint8>((p->red * 30 + p->green * 59 + p->blue * 11 + 50) / 100);
}

qreal RgbU8ColorSpace::intensityF(const quint8 *src) const
{
    const KoBgrU8Traits::Pixel *p = reinterpret_cast<const KoBgrU8Traits::Pixel *>(src);
    // Integer version of:
    //      (p->red * 0.30 + p->green * 0.59 + p->blue * 0.11) / 255.0
    return static_cast<qreal>(p->red * 30 + p->green * 59 + p->blue * 11) / (100.0 * 255.0);
}

void RgbU8ColorSpace::toHSY(const QVector<double> &channelValues, qreal *hue, qreal *sat, qreal *luma) const
{
    RGBToHSY(channelValues[0],channelValues[1],channelValues[2], hue, sat, luma, lumaCoefficients()[0], lumaCoefficients()[1], lumaCoefficients()[2]);
}

QVector <double> RgbU8ColorSpace::fromHSY(qreal *hue, qreal *sat, qreal *luma) const
{
    QVector <double> channelValues(4);
    HSYToRGB(*hue, *sat, *luma, &channelValues[0],&channelValues[1],&channelValues[2], lumaCoefficients()[0], lumaCoefficients()[1], lumaCoefficients()[2]);
    channelValues[3]=1.0;
    return channelValues;
}

void RgbU8ColorSpace::toYUV(const QVector<double> &channelValues, qreal *y, qreal *u, qreal *v) const
{

    
    RGBToYUV(channelValues[0],channelValues[1],channelValues[2], y, u, v, lumaCoefficients()[0], lumaCoefficients()[1], lumaCoefficients()[2]);
}

QVector <double> RgbU8ColorSpace::fromYUV(qreal *y, qreal *u, qreal *v) const
{
    QVector <double> channelValues(4);

    YUVToRGB(*y, *u, *v, &channelValues[0],&channelValues[1],&channelValues[2], lumaCoefficients()[0], lumaCoefficients()[1], lumaCoefficients()[2]);
    channelValues[3]=1.0;
    return channelValues;
}

void RgbU8ColorSpace::fillGrayBrushWithColorAndLightnessOverlay(quint8* dst, const QRgb* brush, quint8* brushColor, qint32 nPixels) const
{
    fillGrayBrushWithColorPreserveLightnessRGB<KoBgrU8Traits>(dst, brush, brushColor, 1.0, nPixels);
}

void RgbU8ColorSpace::fillGrayBrushWithColorAndLightnessWithStrength(quint8* dst, const QRgb* brush, quint8* brushColor, qreal strength, qint32 nPixels) const
{
    fillGrayBrushWithColorPreserveLightnessRGB<KoBgrU8Traits>(dst, brush, brushColor, strength, nPixels);
}

void RgbU8ColorSpace::modulateLightnessByGrayBrush(quint8 *dst, const QRgb *brush, qreal strength, qint32 nPixels) const
{
    modulateLightnessByGrayBrushRGB<KoBgrU8Traits>(dst, brush, strength, nPixels);
}

QList<KoColorProfileQuery> RgbU8ColorSpaceFactory::requiredConnectionProfiles(const KoColorProfile *profile) const
{
    if (profile->getTransferCharacteristics() == TRC_ITU_R_BT_2100_0_PQ
        && profile->getColorPrimaries() != PRIMARIES_UNSPECIFIED) {
        return {KoColorProfileQuery(profile->getColorPrimaries(), TRC_LINEAR)};
    }
    return {};
}

QList<KoColorConversionTransformationFactory*> RgbU8ColorSpaceFactory::colorConversionLinksFromProfile(const KoColorProfile *profile) const
{
    if (profile->getTransferCharacteristics() == TRC_ITU_R_BT_2100_0_PQ
        && profile->getColorPrimaries() != PRIMARIES_UNSPECIFIED) {

        KoColorSpaceRegistry *registry = KoColorSpaceRegistry::instance();
        KoColorProfileQuery query(profile->getColorPrimaries(), TRC_LINEAR);

        /// we cannot generate a profile at this stage, it should have been generated
        /// by the registry thanks to the call to requiredConnectionProfiles()
        const KoColorProfile *connectionProfile = registry->profileFor(query, false);
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(connectionProfile, QList<KoColorConversionTransformationFactory *>());

        LcmsRGBP2020PQColorSpaceFactoryWrapper<RgbU8ColorSpaceFactory> factory(profile->name(), connectionProfile->name());
        return factory.colorConversionLinks();
    } else if (profile->name() == "High Dynamic Range UHDTV Wide Color Gamut Display (Rec. 2020) - SMPTE ST 2084 PQ EOTF") {
        KoColorSpaceRegistry *registry = KoColorSpaceRegistry::instance();
        LcmsRGBP2020PQColorSpaceFactoryWrapper<RgbU8ColorSpaceFactory> factory(profile->name(), registry->p2020G10Profile()->name());
        return factory.colorConversionLinks();
    }
    return QList<KoColorConversionTransformationFactory*>();
}
