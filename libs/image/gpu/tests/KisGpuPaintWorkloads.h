/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_WORKLOADS_H
#define KIS_GPU_PAINT_WORKLOADS_H

#include "KisGpuTileStore.h"
#include <QString>

namespace KisGpuPaintMeasurements {
inline constexpr quint64 BudgetBytes = 256 * 1024 * 1024;
inline constexpr quint32 BaseColor = 0xC0102030;

inline quint32 layerColor(int index)
{
    return 0x80000000 | (quint32(index * 123457) & 0x00FFFFFF);
}

struct Workload {
    QString name;
    QRect bounds;
    QVector<KisGpuTileStore::PaintCommand> commands;
    int layerCount = 0;
    QRect layerBounds = {};
};

inline QVector<Workload> workloads()
{
    using Op = KisGpuTileStore::CompositeOp;
    QVector<Workload> result {
        {QStringLiteral("small"), QRect(0, 0, 128, 128), {{QRect(5, 7, 8, 8), 0x800000FF}}},
        {QStringLiteral("overlapping"), QRect(0, 0, 128, 128), {}},
        {QStringLiteral("scattered"), QRect(0, 0, 2048, 1024), {}}
    };
    for (int i = 0; i < 128; ++i) {
        const quint32 color = 0x80000000 | (quint32(i * 123457) & 0x00FFFFFF);
        const Op operation = i % 7 == 0 ? Op::Erase : Op::Over;
        result[1].commands.push_back({QRect(5 + i * 7 % 24, 7 + i * 11 % 24, 32, 32), color, operation, 192, 128});
        result[2].commands.push_back({QRect(i % 16 * 128 + 5, i / 16 * 128 + 7, 64, 64), color, operation, 192, 128});
    }
    result.push_back({QStringLiteral("layer-projection"), QRect(0, 0, 4096, 4096), {}, 24, QRect(1536, 1536, 1024, 1024)});
    return result;
}

}

#endif
