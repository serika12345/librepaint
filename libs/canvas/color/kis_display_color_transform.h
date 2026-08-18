/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_DISPLAY_COLOR_TRANSFORM_H
#define KIS_DISPLAY_COLOR_TRANSFORM_H

#include <QRect>
#include <QScopedPointer>
#include <QSharedPointer>

#include <KoColorConversionTransformation.h>

#include <kis_types.h>
#include <kritacanvas_export.h>

class QColor;
class QImage;
class KoColor;
class KoColorProfile;
class KoColorSpace;
class KoID;
class KisDisplayColorFilter;

class KRITACANVAS_EXPORT KisDisplayColorTransform
{
public:
    using ConversionOptions =
        std::pair<KoColorConversionTransformation::Intent,
                  KoColorConversionTransformation::ConversionFlags>;

    KisDisplayColorTransform();
    ~KisDisplayColorTransform();

    void setDisplayConfiguration(
        const KoColorProfile *widgetProfile,
        const KoColorProfile *canvasProfile,
        KoColorConversionTransformation::Intent intent,
        KoColorConversionTransformation::ConversionFlags conversionFlags);
    void setInputColorSpace(const KoColorSpace *colorSpace);
    void setPaintingColorSpace(const KoColorSpace *colorSpace);
    void setDisplayFilter(QSharedPointer<KisDisplayColorFilter> displayFilter);

    const KoColorSpace *paintingColorSpace() const;
    QSharedPointer<KisDisplayColorFilter> displayFilter() const;
    ConversionOptions conversionOptions() const;
    bool usesDisplayFilter() const;

    QColor toQColor(const KoColor &color,
                    bool proofToPaintColors = false) const;
    KoColor approximateFromRenderedQColor(const QColor &color) const;
    bool canSkipDisplayConversion(const KoColorSpace *colorSpace) const;
    KoColor applyDisplayFiltering(const KoColor &color,
                                  const KoID &bitDepthId) const;
    void applyDisplayFilteringF32(KisFixedPaintDeviceSP device,
                                  const KoColorSpace *destinationColorSpace) const;

    QImage toQImage(KisPaintDeviceSP device,
                    bool proofPaintColors = false) const;
    QImage toQImage(const KoColorSpace *sourceColorSpace,
                    const quint8 *pixels,
                    QSize size,
                    bool proofPaintColors = false) const;
    QColor convertColorToDisplayColorSpace(const KoColor &color,
                                           bool applyDisplayFilter = false) const;
    QImage convertImageToDisplayColorSpace(
        KisPaintDeviceSP sourceDevice,
        QRect source = QRect(),
        bool applyDisplayFilter = false) const;

    KoColor captureVisualRepresentation(const KoColor &color) const;
    KoColor restorePaintingColor(const KoColor &visualRepresentation) const;

    KoColor fromHsv(int h, int s, int v, int a = 255) const;
    KoColor fromHsvF(qreal h, qreal s, qreal v, qreal a = 1.0) const;
    KoColor fromHslF(qreal h, qreal s, qreal l, qreal a = 1.0) const;
    KoColor fromHsiF(qreal h, qreal s, qreal i) const;
    KoColor fromHsyF(qreal h,
                     qreal s,
                     qreal y,
                     qreal redWeight = 0.2126,
                     qreal greenWeight = 0.7152,
                     qreal blueWeight = 0.0722,
                     qreal gamma = 2.2) const;

    void getHsv(const KoColor &color,
                int *h,
                int *s,
                int *v,
                int *a = nullptr) const;
    void getHsvF(const KoColor &color,
                 qreal *h,
                 qreal *s,
                 qreal *v,
                 qreal *a = nullptr) const;
    void getHslF(const KoColor &color,
                 qreal *h,
                 qreal *s,
                 qreal *l,
                 qreal *a = nullptr) const;
    void getHsiF(const KoColor &color,
                 qreal *h,
                 qreal *s,
                 qreal *i) const;
    void getHsyF(const KoColor &color,
                 qreal *h,
                 qreal *s,
                 qreal *y,
                 qreal redWeight = 0.2126,
                 qreal greenWeight = 0.7152,
                 qreal blueWeight = 0.0722,
                 qreal gamma = 2.2) const;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif
