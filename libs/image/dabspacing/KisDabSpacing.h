/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_DAB_SPACING_H
#define KIS_DAB_SPACING_H
#include <QPointF>

namespace KisDabSpacing {
/** Next interpolation factor, or -1 after accumulating the remaining segment.
 * Finite positions, nonnegative accumulated distance and finite spacing are required.
 * The 0.5px minimum and float segment length preserve CPU brush placement.
 */
qreal nextIsotropic(const QPointF &start, const QPointF &end, qreal spacing, qreal &accumulated);
}
#endif
