/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_NONLINEAR_RGBA8_P_H
#define KIS_GPU_NONLINEAR_RGBA8_P_H

#include "KisGpuOperationDocument.h"
#include <QByteArray>

namespace LibrePaint::Image::Gpu::Detail {
constexpr int NonlinearRgba8ModeCount = 28;
constexpr int NonlinearRgba8Layers = (NonlinearRgba8ModeCount + 3) / 4;
constexpr qsizetype NonlinearRgba8Bytes = NonlinearRgba8Layers * 256 * 256 * 4;
constexpr bool usesNonlinearRgba8(LayerBlendMode mode)
{ return mode >= LayerBlendMode::GeometricMean && mode <= LayerBlendMode::AdditiveSubtractive; }

/** Immutable process-lifetime channel values shared by sparse defaults and the
 * GPU's integer texture. An invalid embedded resource throws std::logic_error.
 */
KRITAIMAGEGPUOPERATIONS_EXPORT const QByteArray &nonlinearRgba8Values();

inline quint8 nonlinearRgba8Channel(LayerBlendMode mode, quint8 source, quint8 destination)
{
    const int index = int(mode) - int(LayerBlendMode::GeometricMean);
    return quint8(nonlinearRgba8Values()[((index / 4) * 65536 + source * 256 + destination) * 4 + index % 4]);
}
}
#endif
