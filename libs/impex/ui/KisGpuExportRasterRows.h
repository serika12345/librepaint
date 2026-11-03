/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_EXPORT_RASTER_ROWS_H
#define KIS_GPU_EXPORT_RASTER_ROWS_H

#include <KisExportRasterRows.h>
#include <kis_types.h>
#include <kritaui_export.h>
#include <QObject>
#include <memory>

/** Fixed GPU projection for a format writer. Construction, destruction and
 * GPU transfers use the image thread. Construction checks the working-memory
 * budget, pins the version and determines transparency through the event loop.
 * Rows are read on one serialization thread while the image event loop runs.
 * Destruction cancels pending and subsequent reads; returned bytes remain valid.
 * Invalid input throws std::invalid_argument, insufficient budget throws
 * std::length_error, and GPU failures throw std::runtime_error.
 */
class KRITAUI_EXPORT KisGpuExportRasterRows : public QObject
{
    Q_OBJECT
public:
    KisGpuExportRasterRows(KisImageSP image, qsizetype memoryBudgetBytes);
    ~KisGpuExportRasterRows() override;
    KisExportRasterRowsSP rows() const;
    bool containsTransparency() const;
private:
    struct Private;
    std::unique_ptr<Private> m_d;
};

#endif
