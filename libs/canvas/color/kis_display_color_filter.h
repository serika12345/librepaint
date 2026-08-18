/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_DISPLAY_COLOR_FILTER_H
#define KIS_DISPLAY_COLOR_FILTER_H

#include <kis_projection_pixel_filter.h>

class KRITACANVAS_EXPORT KisDisplayColorFilter : public KisProjectionPixelFilter
{
public:
    ~KisDisplayColorFilter() override = default;

    virtual void approximateInverseTransformation(quint8 *pixels,
                                                  quint32 numPixels) = 0;
    virtual void approximateForwardTransformation(quint8 *pixels,
                                                  quint32 numPixels) = 0;
};

#endif
