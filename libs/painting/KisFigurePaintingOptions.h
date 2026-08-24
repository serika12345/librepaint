/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_FIGURE_PAINTING_OPTIONS_H
#define KIS_FIGURE_PAINTING_OPTIONS_H

/**
 * Stable style values used when a figure is rendered into a paint device.
 *
 * The ordinal values are shared with the geometry option widgets and the
 * scripting style-name mapping.
 */
namespace KisFigurePaintingOptions
{

enum FillStyle {
    FillStyleNone,
    FillStyleForegroundColor,
    FillStyleBackgroundColor,
    FillStylePattern,
};

enum StrokeStyle {
    StrokeStyleNone,
    StrokeStyleForeground,
    StrokeStyleBackground
};

}

#endif
