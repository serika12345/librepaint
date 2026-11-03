/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_BASIC_CURVE_CONTROLS_H
#define KIS_GPU_BASIC_CURVE_CONTROLS_H

#include <kritagpuimage_export.h>
#include <QPointF>
#include <array>
#include <optional>

namespace LibrePaint::Image::Gpu {
/**
 * Unquantized Basic controls for validated painting coordinates and finite tangents.
 * A zero tangent returns std::nullopt.
 */
KRITAGPUIMAGE_EXPORT std::optional<std::array<QPointF, 2>>
basicCurveControls(QPointF first, QPointF last, QPointF tangent1, QPointF tangent2);
}

#endif
