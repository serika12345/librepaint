/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_INPUT_VALIDATION_P_H
#define KIS_GPU_PAINT_INPUT_VALIDATION_P_H
#include "KisGpuTileStore.h"
#include <cmath>
#include <limits>

namespace KisGpuTileStorage {
inline bool validDabCommand(const KisGpuTileStore::DabCommand &command)
{
    const double cx = command.center.x(), cy = command.center.y();
    const double dx = command.diameter.width(), dy = command.diameter.height();
    const double fx = command.fade.width(), fy = command.fade.height();
    return std::isfinite(cx) && std::isfinite(cy) && std::isfinite(dx) && std::isfinite(dy)
        && std::isfinite(fx) && std::isfinite(fy) && dx > 0 && dy > 0 && fx > 0 && fy > 0
        && fx <= 1 && fy <= 1 && std::abs(cx) <= std::numeric_limits<int>::max()
        && std::abs(cy) <= std::numeric_limits<int>::max()
        && dx <= std::numeric_limits<int>::max() && dy <= std::numeric_limits<int>::max()
        && std::isfinite(float(2 / dx / fx)) && std::isfinite(float(2 / dy / fy))
        && (!command.pickup || command.operation == KisGpuTileStore::CompositeOp::Over)
        && (command.operation == KisGpuTileStore::CompositeOp::Over || command.operation == KisGpuTileStore::CompositeOp::Erase);
}
inline bool validBrushTexture(QSize size, const QByteArray &alpha)
{
    return size.width() > 0 && size.height() > 0 && qint64(size.width()) * size.height() == alpha.size();
}
inline bool validDabBatch(const QVector<KisGpuTileStore::DabCommand> &commands, bool textured = false)
{
    bool mixing = false;
    for (const auto &command : commands) {
        if (!validDabCommand(command)) return false;
        mixing = mixing || command.pickup.has_value();
    }
    return !mixing || (!textured && commands.size() <= 16);
}
}
#endif
