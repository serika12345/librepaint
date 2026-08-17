/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_PROJECTION_PIXEL_FILTER_H
#define KIS_PROJECTION_PIXEL_FILTER_H

#include <QtGlobal>

#include <kritacanvas_export.h>

class KRITACANVAS_EXPORT KisProjectionPixelFilter
{
public:
    virtual ~KisProjectionPixelFilter() = default;

    virtual void filter(quint8 *pixels, quint32 numPixels) = 0;
    virtual bool useInternalColorManagement() const = 0;
};

#endif
