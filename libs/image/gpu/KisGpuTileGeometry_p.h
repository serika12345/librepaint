/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_TILE_GEOMETRY_P_H
#define KIS_GPU_TILE_GEOMETRY_P_H
#include <QRect>
#include <limits>
#include <optional>

namespace KisGpuTileStorage {
inline std::optional<QRect> tileBounds(QPoint coordinate)
{
    const auto x = qint64(coordinate.x()) * 64, y = qint64(coordinate.y()) * 64;
    if (x < std::numeric_limits<int>::min() || x + 63 > std::numeric_limits<int>::max()
        || y < std::numeric_limits<int>::min() || y + 63 > std::numeric_limits<int>::max()) return {};
    return QRect(int(x), int(y), 64, 64);
}
}
#endif
