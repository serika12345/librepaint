/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_CONVEX_HULL_P_H
#define KIS_CONVEX_HULL_P_H

#include "kis_types.h"

#include <QPoint>
#include <QVector>

namespace KisConvexHullPrivate
{

enum class BoundaryMode {
    OpaquePixels,
    SelectionLike,
};

QVector<QPoint> retrieveBoundaryPoints(const KisPaintDeviceSP &device, BoundaryMode mode);

} // namespace KisConvexHullPrivate

#endif // KIS_CONVEX_HULL_P_H
