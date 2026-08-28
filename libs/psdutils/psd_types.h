/*
 * SPDX-FileCopyrightText: 2010 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PSD_TYPES_H
#define PSD_TYPES_H

#include <QtGlobal>

#include <cstdint>

const int MAX_CHANNELS = 56;

const int MAX_PSD_SIZE = 30000;

typedef qint32 Fixed; /* Represents a fixed point implied decimal */

enum class psd_byte_order : std::uint_least8_t {
    psdBigEndian = 0,
    psdLittleEndian,
    psdInvalidByteOrder = 255,
};

enum psd_compression_type : std::uint16_t {
    Uncompressed = 0,
    RLE,
    ZIP,
    ZIPWithPrediction,
    Unknown = 255,
};

/**
 * Image color/depth modes
 */
enum psd_color_mode {
    Bitmap = 0,
    Grayscale = 1,
    Indexed = 2,
    RGB = 3,
    CMYK = 4,
    MultiChannel = 7,
    DuoTone = 8,
    Lab = 9,
    Gray16,
    RGB48,
    Lab48,
    CMYK64,
    DeepMultichannel,
    Duotone16,
    COLORMODE_UNKNOWN = 9000
};

namespace psd_color_sampler
{
/**
 * Color samplers, apparently distinct from PSDColormode
 */
enum PSDColorSamplers {
    RGB,
    HSB,
    CMYK,
    PANTONE, // LAB
    FOCOLTONE, // CMYK
    TRUMATCH, // CMYK
    TOYO, // LAB
    LAB,
    GRAYSCALE,
    HKS, // CMYK
    DIC, // LAB
    TOTAL_INK,
    MONITOR_RGB,
    DUOTONE,
    OPACITY,
    ANPA = 3000 // LAB
};
}

// EFFECTS
enum psd_gradient_style {
    psd_gradient_style_linear, // 'Lnr '
    psd_gradient_style_radial, // 'Rdl '
    psd_gradient_style_angle, // 'Angl'
    psd_gradient_style_reflected, // 'Rflc'
    psd_gradient_style_diamond // 'Dmnd'
};

enum psd_color_stop_type {
    psd_color_stop_type_foreground_color, // 'FrgC'
    psd_color_stop_type_background_Color, // 'BckC'
    psd_color_stop_type_user_stop // 'UsrS'
};

enum psd_technique_type {
    psd_technique_softer,
    psd_technique_precise,
    psd_technique_slope_limit,
};

enum psd_stroke_position { psd_stroke_outside, psd_stroke_inside, psd_stroke_center };

enum psd_fill_type {
    psd_fill_solid_color,
    psd_fill_gradient,
    psd_fill_pattern,
};

enum psd_glow_source {
    psd_glow_center,
    psd_glow_edge,
};

enum psd_bevel_style {
    psd_bevel_outer_bevel,
    psd_bevel_inner_bevel,
    psd_bevel_emboss,
    psd_bevel_pillow_emboss,
    psd_bevel_stroke_emboss,
};

enum psd_direction { psd_direction_up, psd_direction_down };

enum psd_section_type { psd_other = 0, psd_open_folder, psd_closed_folder, psd_bounding_divider };

#endif // PSD_TYPES_H
