/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_BRUSH_DABS_P_H
#define KIS_GPU_BRUSH_DABS_P_H

#include "KisGpuOperationDocument.h"
#include <functional>

namespace LibrePaint::Image::Gpu::Detail {
qreal pressureValue(const PressureResponse &response, qreal pressure);

/** Ordered geometry consumed synchronously by the current native brush packers. */
struct BrushDab
{
    QPointF position;
    qreal pressure;
    qreal scale;
    QSizeF size;
};
/** Calls visit in native dab order; false stops unsupported or over-budget geometry. */
bool forEachBrushDab(const PaintOperation &operation, bool pending,
                     const std::function<bool(const BrushDab &)> &visit);

}
#endif
