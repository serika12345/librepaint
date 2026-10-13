/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_DOCUMENT_P_H
#define KIS_GPU_PAINT_DOCUMENT_P_H
#include "KisGpuPaintDocument.h"
#include <gpu/KisGpuExecution.h>
#include <kra/KisKraRasterCodec.h>
#include <QMap>

namespace Krita::Document::Gpu {
struct PaintDocument::Private
{
    Paint::Execution &execution;
    Paint::PaintImageSP image;
    Paint::Limits limits;
    QString path;
    QByteArray description;
    QMap<QString, QByteArray> files;
    static ImportExport::Kra::Limits formatLimits(Paint::Limits limits)
    {
        ImportExport::Kra::Limits result;
        result.rasterBytes = limits.rasterBytes;
        result.layers = limits.layerCount;
        return result;
    }
};
}
#endif
