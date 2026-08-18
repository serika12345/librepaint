/*
 * SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_display_color_transform.h"

#include <cmath>

#include <QColor>
#include <QHash>
#include <QImage>
#include <QScopedArrayPointer>
#include <QVector>

#include <KoChannelInfo.h>
#include <KoColor.h>
#include <KoColorConversions.h>
#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoID.h>

#include <kis_assert.h>
#include <kis_debug.h>
#include <kis_fixed_paint_device.h>
#include <kis_iterator_ng.h>
#include <kis_paint_device.h>

#include "kis_display_color_filter.h"

struct KisDisplayColorTransform::Private
{
    const KoColorProfile *widgetProfile {nullptr};
    const KoColorProfile *canvasProfile {nullptr};
    const KoColorProfile *inputImageProfile {nullptr};
    const KoColorSpace *paintingColorSpace {nullptr};
    KoColorConversionTransformation::Intent intent {
        KoColorConversionTransformation::internalRenderingIntent()};
    KoColorConversionTransformation::ConversionFlags conversionFlags {
        KoColorConversionTransformation::internalConversionFlags()};
    QSharedPointer<KisDisplayColorFilter> displayFilter;

    mutable const KoColorSpace *cachedOcioInputColorSpace {nullptr};
    mutable const KoColorSpace *cachedOcioOutputColorSpace {nullptr};
    mutable const KoColorSpace *cachedWidgetColorSpace {nullptr};
    mutable QHash<QString, const KoColorSpace *> cachedCanvasColorSpaces;
    mutable const KoColorSpace *cachedIntermediateColorSpace {nullptr};

    void invalidateDisplayColorSpaces()
    {
        cachedOcioInputColorSpace = nullptr;
        cachedOcioOutputColorSpace = nullptr;
        cachedWidgetColorSpace = nullptr;
        cachedCanvasColorSpaces.clear();
    }

    bool usesDisplayFilter() const
    {
        return displayFilter && paintingColorSpace &&
            paintingColorSpace->colorModelId() == RGBAColorModelID;
    }

    const KoColorProfile *ocioInputProfile() const
    {
        return displayFilter && displayFilter->useInternalColorManagement()
            ? canvasProfile
            : inputImageProfile;
    }

    const KoColorSpace *ocioInputColorSpace() const
    {
        if (!cachedOcioInputColorSpace) {
            cachedOcioInputColorSpace = KoColorSpaceRegistry::instance()->colorSpace(
                RGBAColorModelID.id(),
                Float32BitsColorDepthID.id(),
                ocioInputProfile());
        }
        return cachedOcioInputColorSpace;
    }

    const KoColorSpace *ocioOutputColorSpace() const
    {
        if (!cachedOcioOutputColorSpace) {
            cachedOcioOutputColorSpace = KoColorSpaceRegistry::instance()->colorSpace(
                RGBAColorModelID.id(),
                Float32BitsColorDepthID.id(),
                canvasProfile);
        }
        return cachedOcioOutputColorSpace;
    }

    const KoColorSpace *widgetColorSpace() const
    {
        if (!cachedWidgetColorSpace) {
            cachedWidgetColorSpace = KoColorSpaceRegistry::instance()->colorSpace(
                RGBAColorModelID.id(),
                Integer8BitsColorDepthID.id(),
                widgetProfile);
        }
        return cachedWidgetColorSpace;
    }

    const KoColorSpace *canvasColorSpace(const KoID &bitDepthId) const
    {
        const QString key = bitDepthId.id();
        const auto existing = cachedCanvasColorSpaces.constFind(key);
        if (existing != cachedCanvasColorSpaces.constEnd()) {
            return existing.value();
        }

        const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->colorSpace(
            RGBAColorModelID.id(), bitDepthId.id(), canvasProfile);
        cachedCanvasColorSpaces.insert(key, colorSpace);
        return colorSpace;
    }

    const KoColorSpace *intermediateColorSpace() const
    {
        if (!cachedIntermediateColorSpace) {
            cachedIntermediateColorSpace = KoColorSpaceRegistry::instance()->colorSpace(
                RGBAColorModelID.id(),
                Float32BitsColorDepthID.id(),
                KoColorSpaceRegistry::instance()->p2020G10Profile());
        }
        return cachedIntermediateColorSpace;
    }

    bool needsColorProofing(const KoColorSpace *sourceColorSpace) const
    {
        if (!paintingColorSpace || sourceColorSpace == paintingColorSpace ||
            *sourceColorSpace == *paintingColorSpace) {
            return false;
        }

        if (sourceColorSpace->colorModelId() == paintingColorSpace->colorModelId()) {
            const KoColorProfile *paintingProfile = paintingColorSpace->profile();
            const KoColorProfile *sourceProfile = sourceColorSpace->profile();
            const bool matchingProfiles =
                paintingProfile == sourceProfile ||
                (paintingProfile && sourceProfile &&
                 *paintingProfile == *sourceProfile);

            if (matchingProfiles &&
                (sourceColorSpace->colorDepthId() == Integer8BitsColorDepthID ||
                 sourceColorSpace->colorDepthId() == Integer16BitsColorDepthID ||
                 paintingColorSpace->colorDepthId() == Float16BitsColorDepthID ||
                 paintingColorSpace->colorDepthId() == Float32BitsColorDepthID)) {
                return false;
            }
        }
        return true;
    }

    KoColor approximateFromQColor(const QColor &color) const
    {
        if (!usesDisplayFilter()) {
            return KoColor(color, paintingColorSpace);
        }

        KoColor result(color, intermediateColorSpace());
        displayFilter->approximateInverseTransformation(result.data(), 1);
        result.convertTo(paintingColorSpace);
        return result;
    }

    QColor approximateToQColor(const KoColor &sourceColor) const
    {
        KoColor color(sourceColor);
        if (usesDisplayFilter()) {
            color.convertTo(intermediateColorSpace());
            displayFilter->approximateForwardTransformation(color.data(), 1);
        }
        return color.toQColor();
    }
};

KisDisplayColorTransform::KisDisplayColorTransform()
    : m_d(new Private)
{
    m_d->inputImageProfile = KoColorSpaceRegistry::instance()->p709SRGBProfile();
    m_d->paintingColorSpace = KoColorSpaceRegistry::instance()->rgb8();
}

KisDisplayColorTransform::~KisDisplayColorTransform() = default;

void KisDisplayColorTransform::setDisplayConfiguration(
    const KoColorProfile *widgetProfile,
    const KoColorProfile *canvasProfile,
    KoColorConversionTransformation::Intent intent,
    KoColorConversionTransformation::ConversionFlags conversionFlags)
{
    m_d->widgetProfile = widgetProfile;
    m_d->canvasProfile = canvasProfile;
    m_d->intent = intent;
    m_d->conversionFlags = conversionFlags;
    m_d->invalidateDisplayColorSpaces();
}

void KisDisplayColorTransform::setInputColorSpace(const KoColorSpace *colorSpace)
{
    m_d->inputImageProfile =
        colorSpace && colorSpace->colorModelId() == RGBAColorModelID
        ? colorSpace->profile()
        : KoColorSpaceRegistry::instance()->p709SRGBProfile();
    m_d->invalidateDisplayColorSpaces();
}

void KisDisplayColorTransform::setPaintingColorSpace(const KoColorSpace *colorSpace)
{
    m_d->paintingColorSpace =
        colorSpace ? colorSpace : KoColorSpaceRegistry::instance()->rgb8();
    m_d->invalidateDisplayColorSpaces();
}

void KisDisplayColorTransform::setDisplayFilter(
    QSharedPointer<KisDisplayColorFilter> displayFilter)
{
    m_d->displayFilter = std::move(displayFilter);
    m_d->invalidateDisplayColorSpaces();
}

const KoColorSpace *KisDisplayColorTransform::paintingColorSpace() const
{
    return m_d->paintingColorSpace;
}

QSharedPointer<KisDisplayColorFilter>
KisDisplayColorTransform::displayFilter() const
{
    return m_d->displayFilter;
}

KisDisplayColorTransform::ConversionOptions
KisDisplayColorTransform::conversionOptions() const
{
    return {m_d->intent, m_d->conversionFlags};
}

bool KisDisplayColorTransform::usesDisplayFilter() const
{
    return m_d->usesDisplayFilter();
}

QColor KisDisplayColorTransform::toQColor(const KoColor &sourceColor,
                                          bool proofToPaintColors) const
{
    KoColor color(sourceColor);
    if (proofToPaintColors && m_d->needsColorProofing(color.colorSpace())) {
        color.convertTo(m_d->paintingColorSpace, m_d->intent, m_d->conversionFlags);
    }

    if (m_d->usesDisplayFilter()) {
        KIS_ASSERT_RECOVER(m_d->ocioInputColorSpace()->pixelSize() == 16) {
            return QColor(Qt::green);
        }
        color.convertTo(m_d->ocioInputColorSpace());
        m_d->displayFilter->filter(color.data(), 1);
        color.setProfile(m_d->canvasProfile);
    }

    KIS_ASSERT_RECOVER(m_d->widgetColorSpace()->pixelSize() == 4) {
        return QColor(Qt::red);
    }
    color.convertTo(m_d->widgetColorSpace(), m_d->intent, m_d->conversionFlags);
    const quint8 *pixel = color.data();
    return QColor(pixel[2], pixel[1], pixel[0], pixel[3]);
}

KoColor KisDisplayColorTransform::approximateFromRenderedQColor(
    const QColor &color) const
{
    return m_d->approximateFromQColor(color);
}

bool KisDisplayColorTransform::canSkipDisplayConversion(
    const KoColorSpace *colorSpace) const
{
    const KoColorProfile *displayProfile = m_d->canvasProfile;
    return !m_d->usesDisplayFilter() &&
        colorSpace->colorModelId() == RGBAColorModelID &&
        (!!colorSpace->profile() == !!displayProfile) &&
        (!colorSpace->profile() ||
         colorSpace->profile()->uniqueId() == displayProfile->uniqueId());
}

KoColor KisDisplayColorTransform::applyDisplayFiltering(
    const KoColor &sourceColor,
    const KoID &bitDepthId) const
{
    KoColor color(sourceColor);
    if (m_d->usesDisplayFilter()) {
        KIS_ASSERT_RECOVER(m_d->ocioInputColorSpace()->pixelSize() == 16) {
            return sourceColor;
        }
        color.convertTo(m_d->ocioInputColorSpace());
        m_d->displayFilter->filter(color.data(), 1);
        color.setProfile(m_d->canvasProfile);
    }
    color.convertTo(m_d->canvasColorSpace(bitDepthId),
                    m_d->intent,
                    m_d->conversionFlags);
    return color;
}

void KisDisplayColorTransform::applyDisplayFilteringF32(
    KisFixedPaintDeviceSP device,
    const KoColorSpace *destinationColorSpace) const
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(
        device->colorSpace()->colorDepthId() == Float32BitsColorDepthID);
    KIS_SAFE_ASSERT_RECOVER_RETURN(
        device->colorSpace()->colorModelId() == RGBAColorModelID);
    KIS_SAFE_ASSERT_RECOVER_RETURN(device->bounds().isValid());

    if (m_d->usesDisplayFilter()) {
        KIS_ASSERT_RECOVER_RETURN(m_d->ocioInputColorSpace()->pixelSize() == 16);
        device->convertTo(m_d->ocioInputColorSpace());
        m_d->displayFilter->filter(
            device->data(), device->bounds().width() * device->bounds().height());
        device->setProfile(m_d->canvasProfile);
    }

    KIS_SAFE_ASSERT_RECOVER_RETURN(destinationColorSpace);
    device->convertTo(destinationColorSpace);
}

QImage KisDisplayColorTransform::toQImage(KisPaintDeviceSP sourceDevice,
                                          bool proofPaintColors) const
{
    KisPaintDeviceSP device = sourceDevice;
    const QRect bounds = sourceDevice->exactBounds();
    if (bounds.isEmpty()) {
        return QImage();
    }

    if (proofPaintColors &&
        m_d->needsColorProofing(sourceDevice->colorSpace())) {
        sourceDevice->convertTo(
            m_d->paintingColorSpace, m_d->intent, m_d->conversionFlags);
    }

    if (m_d->usesDisplayFilter()) {
        KIS_ASSERT_RECOVER(m_d->ocioInputColorSpace()->pixelSize() == 16) {
            return QImage();
        }
        device = new KisPaintDevice(*sourceDevice);
        device->convertTo(m_d->ocioInputColorSpace());

        KisSequentialIterator iterator(device, bounds);
        int consecutivePixels = iterator.nConseqPixels();
        while (iterator.nextPixels(consecutivePixels)) {
            consecutivePixels = iterator.nConseqPixels();
            m_d->displayFilter->filter(iterator.rawData(), consecutivePixels);
        }
        device->setProfile(m_d->canvasProfile, nullptr);
    }

    KIS_ASSERT_RECOVER(m_d->widgetColorSpace()->pixelSize() == 4) {
        return QImage();
    }
    return device->convertToQImage(
        m_d->widgetProfile, bounds, m_d->intent, m_d->conversionFlags);
}

QImage KisDisplayColorTransform::toQImage(const KoColorSpace *sourceColorSpace,
                                          const quint8 *sourcePixels,
                                          QSize size,
                                          bool proofPaintColors) const
{
    const int pixelCount = size.width() * size.height();
    const KoColorSpace *colorSpace = sourceColorSpace;
    const quint8 *pixels = sourcePixels;

    QScopedArrayPointer<quint8> proofBuffer;
    if (proofPaintColors && m_d->needsColorProofing(sourceColorSpace)) {
        proofBuffer.reset(
            new quint8[pixelCount * m_d->paintingColorSpace->pixelSize()]);
        colorSpace->convertPixelsTo(pixels,
                                    proofBuffer.data(),
                                    m_d->paintingColorSpace,
                                    pixelCount,
                                    m_d->intent,
                                    m_d->conversionFlags);
        colorSpace = m_d->paintingColorSpace;
        pixels = proofBuffer.data();
    }

    QScopedArrayPointer<quint8> filterBuffer;
    if (m_d->usesDisplayFilter()) {
        filterBuffer.reset(
            new quint8[pixelCount * m_d->ocioInputColorSpace()->pixelSize()]);
        colorSpace->convertPixelsTo(pixels,
                                    filterBuffer.data(),
                                    m_d->ocioInputColorSpace(),
                                    pixelCount,
                                    m_d->intent,
                                    m_d->conversionFlags);
        m_d->displayFilter->filter(filterBuffer.data(), pixelCount);
        return m_d->ocioOutputColorSpace()->convertToQImage(
            filterBuffer.data(),
            size.width(),
            size.height(),
            m_d->widgetProfile,
            m_d->intent,
            m_d->conversionFlags);
    }

    return colorSpace->convertToQImage(pixels,
                                      size.width(),
                                      size.height(),
                                      m_d->widgetProfile,
                                      m_d->intent,
                                      m_d->conversionFlags);
}

QColor KisDisplayColorTransform::convertColorToDisplayColorSpace(
    const KoColor &sourceColor,
    bool applyDisplayFilter) const
{
    KoColor color = applyDisplayFilter
        ? applyDisplayFiltering(sourceColor, Float32BitsColorDepthID)
        : KoColor(sourceColor);
    if (!applyDisplayFilter) {
        color.convertTo(m_d->canvasColorSpace(Float32BitsColorDepthID),
                        m_d->intent,
                        m_d->conversionFlags);
    }

    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(
        color.colorSpace()->colorModelId() == RGBAColorModelID,
        color.toQColor());

    QVector<float> channels(color.colorSpace()->channelCount());
    color.colorSpace()->normalisedChannelsValue(color.data(), channels);
    QVector<float> rgba = channels;
    for (int channel = 0; channel < channels.size(); ++channel) {
        const KoChannelInfo *info = color.colorSpace()->channels().at(channel);
        rgba[info->displayPosition()] = channels[channel];
    }
    return QColor::fromRgbF(rgba[0], rgba[1], rgba[2], rgba[3]);
}

QImage KisDisplayColorTransform::convertImageToDisplayColorSpace(
    KisPaintDeviceSP sourceDevice,
    QRect source,
    bool applyDisplayFilter) const
{
    KisPaintDeviceSP device = new KisPaintDevice(*sourceDevice.data());
    const QRect bounds = source.isValid() ? source : device->exactBounds();

    if (m_d->usesDisplayFilter() && applyDisplayFilter) {
        KIS_ASSERT_RECOVER(m_d->ocioInputColorSpace()->pixelSize() == 16) {
            return QImage();
        }
        device->convertTo(m_d->ocioInputColorSpace());
        KisSequentialIterator iterator(device, bounds);
        int consecutivePixels = iterator.nConseqPixels();
        while (iterator.nextPixels(consecutivePixels)) {
            consecutivePixels = iterator.nConseqPixels();
            m_d->displayFilter->filter(iterator.rawData(), consecutivePixels);
        }
        device->setProfile(m_d->canvasProfile, nullptr);
    }

    device->convertTo(m_d->canvasColorSpace(Float32BitsColorDepthID),
                      m_d->intent,
                      m_d->conversionFlags);
    return device->convertToQImage(
        m_d->canvasProfile, bounds, m_d->intent, m_d->conversionFlags);
}

KoColor KisDisplayColorTransform::captureVisualRepresentation(
    const KoColor &sourceColor) const
{
    KoColor color(sourceColor);
    color.convertTo(m_d->intermediateColorSpace());
    if (m_d->displayFilter) {
        m_d->displayFilter->approximateForwardTransformation(color.data(), 1);
    }
    return color;
}

KoColor KisDisplayColorTransform::restorePaintingColor(
    const KoColor &visualRepresentation) const
{
    KoColor color(visualRepresentation);
    if (m_d->displayFilter) {
        m_d->displayFilter->approximateInverseTransformation(color.data(), 1);
    }
    color.convertTo(m_d->paintingColorSpace);
    return color;
}

KoColor KisDisplayColorTransform::fromHsv(int h,
                                          int s,
                                          int v,
                                          int a) const
{
    return m_d->approximateFromQColor(QColor::fromHsv(h, s, v, a));
}

void KisDisplayColorTransform::getHsv(const KoColor &sourceColor,
                                      int *h,
                                      int *s,
                                      int *v,
                                      int *a) const
{
    m_d->approximateToQColor(sourceColor).getHsv(h, s, v, a);
}

KoColor KisDisplayColorTransform::fromHsvF(qreal h,
                                           qreal s,
                                           qreal v,
                                           qreal a) const
{
    return m_d->approximateFromQColor(QColor::fromHsvF(h, s, v, a));
}

void KisDisplayColorTransform::getHsvF(const KoColor &sourceColor,
                                       qreal *h,
                                       qreal *s,
                                       qreal *v,
                                       qreal *a) const
{
    const QColor color = m_d->approximateToQColor(sourceColor);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    color.getHsvF(h, s, v, a);
#else
    float hue = *h;
    float saturation = *s;
    float value = *v;
    float alpha = a ? *a : 1.0f;
    color.getHsvF(&hue, &saturation, &value, &alpha);
    *h = hue;
    *s = saturation;
    *v = value;
    if (a) {
        *a = alpha;
    }
#endif
}

KoColor KisDisplayColorTransform::fromHslF(qreal h,
                                           qreal s,
                                           qreal l,
                                           qreal a) const
{
    QColor color = QColor::fromHslF(h, s, l, a);
    if (!color.isValid()) {
        warnKrita << "Could not construct valid color from h" << h << "s" << s
                  << "l" << l << "a" << a;
        color = Qt::black;
    }
    return m_d->approximateFromQColor(color);
}

void KisDisplayColorTransform::getHslF(const KoColor &sourceColor,
                                       qreal *h,
                                       qreal *s,
                                       qreal *l,
                                       qreal *a) const
{
    const QColor color = m_d->approximateToQColor(sourceColor);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    color.getHslF(h, s, l, a);
#else
    float hue = *h;
    float saturation = *s;
    float lightness = *l;
    float alpha = a ? *a : 1.0f;
    color.getHslF(&hue, &saturation, &lightness, &alpha);
    *h = hue;
    *s = saturation;
    *l = lightness;
    if (a) {
        *a = alpha;
    }
#endif
}

KoColor KisDisplayColorTransform::fromHsiF(qreal h,
                                           qreal s,
                                           qreal i) const
{
    qreal red = 0.0;
    qreal green = 0.0;
    qreal blue = 0.0;
    HSIToRGB(h, s, i, &red, &green, &blue);
    QColor color;
    color.setRgbF(qBound(0.0, red, 1.0),
                  qBound(0.0, green, 1.0),
                  qBound(0.0, blue, 1.0),
                  1.0);
    return m_d->approximateFromQColor(color);
}

void KisDisplayColorTransform::getHsiF(const KoColor &sourceColor,
                                       qreal *h,
                                       qreal *s,
                                       qreal *i) const
{
    const QColor color = m_d->approximateToQColor(sourceColor);
    RGBToHSI(color.redF(), color.greenF(), color.blueF(), h, s, i);
}

KoColor KisDisplayColorTransform::fromHsyF(qreal h,
                                           qreal s,
                                           qreal y,
                                           qreal redWeight,
                                           qreal greenWeight,
                                           qreal blueWeight,
                                           qreal gamma) const
{
    QVector<qreal> channels(3);
    y = std::pow(y, gamma);
    HSYToRGB(h,
             s,
             y,
             &channels[0],
             &channels[1],
             &channels[2],
             redWeight,
             greenWeight,
             blueWeight);
    KoColorSpaceRegistry::instance()->rgb8()->profile()->delinearizeFloatValueFast(
        channels);
    QColor color;
    color.setRgbF(qBound(0.0, channels[0], 1.0),
                  qBound(0.0, channels[1], 1.0),
                  qBound(0.0, channels[2], 1.0),
                  1.0);
    return m_d->approximateFromQColor(color);
}

void KisDisplayColorTransform::getHsyF(const KoColor &sourceColor,
                                       qreal *h,
                                       qreal *s,
                                       qreal *y,
                                       qreal redWeight,
                                       qreal greenWeight,
                                       qreal blueWeight,
                                       qreal gamma) const
{
    const QColor color = m_d->approximateToQColor(sourceColor);
    QVector<qreal> channels {color.redF(), color.greenF(), color.blueF()};
    KoColorSpaceRegistry::instance()->rgb8()->profile()->linearizeFloatValueFast(
        channels);
    RGBToHSY(channels[0],
             channels[1],
             channels[2],
             h,
             s,
             y,
             redWeight,
             greenWeight,
             blueWeight);
    *y = std::pow(*y, 1.0 / gamma);
}
