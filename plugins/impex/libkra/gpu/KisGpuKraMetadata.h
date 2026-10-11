/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_KRA_METADATA_H
#define KIS_GPU_KRA_METADATA_H
#include <QByteArray>
#include <QSize>
#include <QString>

/** Native raster document dimensions, resolution and independent document information. */
struct KisGpuKraMetadata {
    QSize canvasSize;
    QString name;
    double xResolution = 1, yResolution = 1;
    /** Optional native document-info XML; empty uses the writer's default information. */
    QByteArray documentInfoXml = {};
    /** Optional native grid, guides and mirror-axis values captured independently of pixels. */
    QByteArray documentSettingsXml = {};
};
#endif
