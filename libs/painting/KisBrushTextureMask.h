/*
 * SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_BRUSH_TEXTURE_MASK_H
#define KIS_BRUSH_TEXTURE_MASK_H

#include <kritapainting_export.h>
#include <QImage>

/** Frozen import settings for the brush's repeating paper resource. */
struct KisBrushTextureMaskSettings
{
    qreal scale = 1;
    qreal brightness = 0;
    qreal contrast = 1;
    qreal neutralPoint = .5;
    int cutoffLeft = 0, cutoffRight = 255, cutoffPolicy = 0;
    bool invert = false;
    bool preserveAlpha = false;
};

/** Native pattern import, producing detached Alpha8 or gray ARGB32 pixels.
 * The caller supplies effective scale and alpha preservation from its resource
 * settings. Null reports an unavailable input or allocation.
 */
KRITAPAINTING_EXPORT QImage prepareBrushTextureMask(const QImage &pattern, const KisBrushTextureMaskSettings &settings);

#endif
