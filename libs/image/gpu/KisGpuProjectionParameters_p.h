/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PROJECTION_PARAMETERS_P_H
#define KIS_GPU_PROJECTION_PARAMETERS_P_H
#include <QtGlobal>
namespace LibrePaint::Image::Gpu::Detail {
struct CompositeParameters {
    quint32 opacityBits = 0x3f800000, flags = 0, channelMask = 15, blendModeOpacity = 0;
    qint32 sourceLeft = 0, sourceTop = 0, sourceRight = 0, sourceBottom = 0;
    qint32 maskLeft = 0, maskTop = 0, maskRight = 0, maskBottom = 0;
};
static_assert(sizeof(CompositeParameters) == 48);
}
#endif
