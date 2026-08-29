/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_convex_hull.h"

#include "kis_convex_hull_p.h"

namespace KisConvexHull
{

QPolygon findConvexHull(KisPaintDeviceSP device)
{
    return findConvexHull(
        KisConvexHullPrivate::retrieveBoundaryPoints(device, KisConvexHullPrivate::BoundaryMode::OpaquePixels));
}

QPolygon findConvexHullSelectionLike(KisPaintDeviceSP device)
{
    return findConvexHull(
        KisConvexHullPrivate::retrieveBoundaryPoints(device, KisConvexHullPrivate::BoundaryMode::SelectionLike));
}

} // namespace KisConvexHull
