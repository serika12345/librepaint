/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_GAUSSIAN_BRUSH_FIXTURE_P_H
#define KIS_GPU_GAUSSIAN_BRUSH_FIXTURE_P_H

#include <QByteArray>

namespace LibrePaint::Image::Gpu::Test {
// Normal GPU brush capture at 2d57883297, recovery format 39, 128x128 RGBA8.
// Basic-5 Size Opacity in Krita_4_Default_Resources.bundle, preset SHA-256
// bd0610171db6d3eca006e7b12697d7baf3868ef34cbf3b1e792592d2f0957939.
// Color (17,34,51), flow .5, canvas opacity .57; full pressure tables retained.
// The uncompressed recovery SHA-256 is:
// f86a7048d2c2cf49fef07dd695cfea7eef444db88b000cec1377460c8f847387.
inline QByteArray defaultGaussianBrushRecoveryV39()
{
    return qUncompress(QByteArray::fromBase64(
        "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"
    ));
}
}
#endif
