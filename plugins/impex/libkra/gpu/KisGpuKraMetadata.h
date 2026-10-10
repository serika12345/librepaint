/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_KRA_METADATA_H
#define KIS_GPU_KRA_METADATA_H
#include <QSize>
#include <QString>

/** Native raster document dimensions and resolution in pixels per point. */
struct KisGpuKraMetadata {
    QSize canvasSize;
    QString name;
    double xResolution = 1, yResolution = 1;
};
#endif
