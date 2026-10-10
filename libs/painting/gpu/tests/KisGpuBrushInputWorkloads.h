/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_BRUSH_INPUT_WORKLOADS_H
#define KIS_GPU_BRUSH_INPUT_WORKLOADS_H
#include "KisGpuBrushStroke.h"
#include <cmath>

/** Shared image contract and input/display measurement values. */
struct KisGpuBrushInputWorkload {
    QRect bounds, occupied;
    int layerCount;
    explicit KisGpuBrushInputWorkload(bool high)
        : bounds(0, 0, high ? 4096 : 128, high ? 4096 : 128),
          occupied(high ? QRect(1536, 1536, 1024, 1024) : bounds), layerCount(high ? 24 : 1) {}
    KisGpuTileStore::DabCommand seed(int index) const {
        KisGpuTileStore::DabCommand result;
        result.center = occupied.center();
        result.diameter = QSizeF(occupied.width() * 4, occupied.height() * 4);
        result.rgba = (quint32(index ? 128 : 255) << 24) | quint32((17 + index * 13) % 256)
            | (quint32((34 + index * 7) % 256) << 8) | (quint32((51 + index * 3) % 256) << 16);
        return result;
    }
    KisGpuBrushStroke::Settings settings() const {
        KisGpuBrushStroke::Settings result;
        result.diameter = layerCount > 1 ? QSizeF(512, 512) : QSizeF(16, 16);
        result.fade = {.8, .8};
        result.rgba = 0xCC3366EE;
        return result;
    }
    QVector<KisGpuBrushStroke::Sample> samples(int iteration, bool replace) const {
        QVector<KisGpuBrushStroke::Sample> result;
        const double pi = std::acos(-1.0);
        const double radius = occupied.width() * (replace ? .22 : .4);
        const QPointF center = occupied.center();
        for (int k = 0; k < (replace ? 9 : 1); ++k) {
            const double angle = replace ? k * pi / 4 + iteration * .03 : iteration * pi / 8;
            result.push_back({center + QPointF(std::cos(angle) * radius, std::sin(angle) * radius), 1});
        }
        return result;
    }
};
#endif
