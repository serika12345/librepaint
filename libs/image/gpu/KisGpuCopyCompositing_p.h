/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_COPY_COMPOSITING_P_H
#define KIS_GPU_COPY_COMPOSITING_P_H
#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
/** Uniform-pixel Copy with native opacity, protected channels and integer rounding. */
RgbaPixel copyPixel(RgbaPixel destination, const RgbaPixel &source,
                   float opacity, quint8 writeMask, PixelFormat format);
}
#endif
