/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_EXPORT_RASTER_ROWS_H
#define KIS_EXPORT_RASTER_ROWS_H

#include "KisImportExportErrorCode.h"
#include <kritaimpex_export.h>
#include <QByteArray>
#include <QSharedPointer>
#include <QSize>
#include <functional>

class KoColorSpace;

/** Fixed raster input for one format conversion. Rows use the color space's
 * native pixel layout and zero-based coordinates. The borrowed color space
 * remains alive through conversion. One serialization thread calls readRow
 * sequentially; success supplies exactly width * pixelSize bytes. The producer
 * defines source lifetime; the reader returns errors before exposing partial rows.
 */
struct KRITAIMPEX_EXPORT KisExportRasterRows
{
    const KoColorSpace *colorSpace = nullptr;
    QSize size;
    std::function<KisImportExportErrorCode(int, QByteArray &)> readRow;
};
using KisExportRasterRowsSP = QSharedPointer<const KisExportRasterRows>;

#endif
