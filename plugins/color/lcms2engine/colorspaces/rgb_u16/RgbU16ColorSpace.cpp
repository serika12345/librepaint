/*
 *  SPDX-FileCopyrightText: 2006 Cyrille Berger <cberger@cberger.net>
 *  SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "RgbU16ColorSpace.h"

#include <QDomElement>

#include <klocalizedstring.h>

#include "LcmsRGBP2020PQColorSpace.h"
#include "compositeops/KoCompositeOps.h"
#include "compositeops/RgbCompositeOpBumpmap.h"
#include "compositeops/RgbCompositeOpIn.h"
#include "compositeops/RgbCompositeOpOut.h"
#include "dithering/KisRgbDitherOpFactory.h"
#include "kis_dom_utils.h"
#include <KoColorConversions.h>
#include <KoColorSpacePreserveLightnessUtils.h>
#include <KoColorProfileQuery.h>

RgbU16ColorSpace::RgbU16ColorSpace(const QString &name, KoColorProfile *p) :
    LcmsColorSpace<KoBgrU16Traits>(colorSpaceId(), name, TYPE_BGRA_16, cmsSigRgbData, p)
{
    addChannel(new KoChannelInfo(i18n("Blue"), 0 * sizeof(quint16), 2, KoChannelInfo::COLOR, KoChannelInfo::UINT16, 2, QColor(0, 0, 255)));
    addChannel(new KoChannelInfo(i18n("Green"), 1 * sizeof(quint16), 1, KoChannelInfo::COLOR, KoChannelInfo::UINT16, 2, QColor(0, 255, 0)));
    addChannel(new KoChannelInfo(i18n("Red"), 2 * sizeof(quint16), 0, KoChannelInfo::COLOR, KoChannelInfo::UINT16, 2, QColor(255, 0, 0)));
    addChannel(new KoChannelInfo(i18n("Alpha"), 3 * sizeof(quint16), 3, KoChannelInfo::ALPHA, KoChannelInfo::UINT16, 2));
    init();

    addStandardCompositeOps<KoBgrU16Traits>(this);
    addStandardDitherOps<KoBgrU16Traits>(this);

    addCompositeOp(new RgbCompositeOpIn<KoBgrU16Traits>(this));
    addCompositeOp(new RgbCompositeOpOut<KoBgrU16Traits>(this));
    addCompositeOp(new RgbCompositeOpBumpmap<KoBgrU16Traits>(this));
}

bool RgbU16ColorSpace::willDegrade(ColorSpaceIndependence independence) const
{
    if (independence == TO_RGBA8) {
        return true;
    } else {
        return false;
    }
}

KoColorSpace *RgbU16ColorSpace::clone() const
{
    return new RgbU16ColorSpace(name(), profile()->clone());
}

void RgbU16ColorSpace::colorToXML(const quint8 *pixel, QDomDocument &doc, QDomElement &colorElt) const
{
    const KoBgrU16Traits::Pixel *p = reinterpret_cast<const KoBgrU16Traits::Pixel *>(pixel);
    QDomElement labElt = doc.createElement("RGB");
    labElt.setAttribute("r", KisDomUtils::toString(KoColorSpaceMaths< KoBgrU16Traits::channels_type, qreal>::scaleToA(p->red)));
    labElt.setAttribute("g", KisDomUtils::toString(KoColorSpaceMaths< KoBgrU16Traits::channels_type, qreal>::scaleToA(p->green)));
    labElt.setAttribute("b", KisDomUtils::toString(KoColorSpaceMaths< KoBgrU16Traits::channels_type, qreal>::scaleToA(p->blue)));
    labElt.setAttribute("space", profile()->name());
    colorElt.appendChild(labElt);
}

void RgbU16ColorSpace::colorFromXML(quint8 *pixel, const QDomElement &elt) const
{
    KoBgrU16Traits::Pixel *p = reinterpret_cast<KoBgrU16Traits::Pixel *>(pixel);
    p->red = KoColorSpaceMaths< qreal, KoBgrU16Traits::channels_type >::scaleToA(KisDomUtils::toDouble(elt.attribute("r")));
    p->green = KoColorSpaceMaths< qreal, KoBgrU16Traits::channels_type >::scaleToA(KisDomUtils::toDouble(elt.attribute("g")));
    p->blue = KoColorSpaceMaths< qreal, KoBgrU16Traits::channels_type >::scaleToA(KisDomUtils::toDouble(elt.attribute("b")));
    p->alpha = KoColorSpaceMathsTraits<quint16>::max;
}

void RgbU16ColorSpace::toHSY(const QVector<double> &channelValues, qreal *hue, qreal *sat, qreal *luma) const
{
    RGBToHSY(channelValues[0],channelValues[1],channelValues[2], hue, sat, luma, lumaCoefficients()[0], lumaCoefficients()[1], lumaCoefficients()[2]);
}

QVector <double> RgbU16ColorSpace::fromHSY(qreal *hue, qreal *sat, qreal *luma) const
{
    QVector <double> channelValues(4);
    HSYToRGB(*hue, *sat, *luma, &channelValues[0],&channelValues[1],&channelValues[2], lumaCoefficients()[0], lumaCoefficients()[1], lumaCoefficients()[2]);
    channelValues[3]=1.0;
    return channelValues;
}

void RgbU16ColorSpace::toYUV(const QVector<double> &channelValues, qreal *y, qreal *u, qreal *v) const
{

    
    RGBToYUV(channelValues[0],channelValues[1],channelValues[2], y, u, v, lumaCoefficients()[0], lumaCoefficients()[1], lumaCoefficients()[2]);
}

QVector <double> RgbU16ColorSpace::fromYUV(qreal *y, qreal *u, qreal *v) const
{
    QVector <double> channelValues(4);

    YUVToRGB(*y, *u, *v, &channelValues[0],&channelValues[1],&channelValues[2], lumaCoefficients()[0], lumaCoefficients()[1], lumaCoefficients()[2]);
    channelValues[3]=1.0;
    return channelValues;
}

void RgbU16ColorSpace::fillGrayBrushWithColorAndLightnessOverlay(quint8* dst, const QRgb* brush, quint8* brushColor, qint32 nPixels) const
{
    fillGrayBrushWithColorPreserveLightnessRGB<KoBgrU16Traits>(dst, brush, brushColor, 1.0, nPixels);
}

void RgbU16ColorSpace::fillGrayBrushWithColorAndLightnessWithStrength(quint8* dst, const QRgb* brush, quint8* brushColor, qreal strength, qint32 nPixels) const
{
    fillGrayBrushWithColorPreserveLightnessRGB<KoBgrU16Traits>(dst, brush, brushColor, strength, nPixels);
}

void RgbU16ColorSpace::modulateLightnessByGrayBrush(quint8 *dst, const QRgb *brush, qreal strength, qint32 nPixels) const
{
   modulateLightnessByGrayBrushRGB<KoBgrU16Traits>(dst, brush, strength, nPixels);
}

QList<KoColorProfileQuery> RgbU16ColorSpaceFactory::requiredConnectionProfiles(const KoColorProfile *profile) const
{
    if (profile->getTransferCharacteristics() == TRC_ITU_R_BT_2100_0_PQ
        && profile->getColorPrimaries() != PRIMARIES_UNSPECIFIED) {
        return {KoColorProfileQuery(profile->getColorPrimaries(), TRC_LINEAR)};
    }
    return {};
}

QList<KoColorConversionTransformationFactory *> RgbU16ColorSpaceFactory::colorConversionLinksFromProfile(const KoColorProfile *profile) const
{
    if (profile->getTransferCharacteristics() == TRC_ITU_R_BT_2100_0_PQ
        && profile->getColorPrimaries() != PRIMARIES_UNSPECIFIED) {

        KoColorSpaceRegistry *registry = KoColorSpaceRegistry::instance();
        KoColorProfileQuery query(profile->getColorPrimaries(), TRC_LINEAR);

        /// we cannot generate a profile at this stage, it should have been generated
        /// by the registry thanks to the call to requiredConnectionProfiles()
        const KoColorProfile *connectionProfile = registry->profileFor(query, false);
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(connectionProfile, QList<KoColorConversionTransformationFactory *>());

        LcmsRGBP2020PQColorSpaceFactoryWrapper<RgbU16ColorSpaceFactory> factory(profile->name(), connectionProfile->name());
        return factory.colorConversionLinks();
    } else if (profile->name() == "High Dynamic Range UHDTV Wide Color Gamut Display (Rec. 2020) - SMPTE ST 2084 PQ EOTF") {
        KoColorSpaceRegistry *registry = KoColorSpaceRegistry::instance();
        LcmsRGBP2020PQColorSpaceFactoryWrapper<RgbU16ColorSpaceFactory> factory(profile->name(), registry->p2020G10Profile()->name());
        return factory.colorConversionLinks();
    }
    return QList<KoColorConversionTransformationFactory*>();
}
