/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_KRA_RASTER_CODEC_H
#define KIS_KRA_RASTER_CODEC_H

#include <gpu/KisGpuOperationDocument.h>
#include <QMap>
#include <kritaimpex_export.h>

class QIODevice;
class QImage;

namespace Krita::ImportExport::Kra {
enum class Status { Success, InvalidData, UnsupportedData, CapacityExceeded, IoFailure };
struct Limits
{
    qsizetype rasterBytes = 256 * 1024 * 1024;
    qsizetype archiveBytes = 512 * 1024 * 1024;
    qsizetype xmlBytes = 8 * 1024 * 1024;
    qsizetype entries = 16384;
    qsizetype layers = 4096;
};
struct Document
{
    Image::Gpu::RasterImage image;
    QByteArray description;
    QMap<QString, QByteArray> files;
};
struct Outcome
{
    Status code = Status::InvalidData;
    QString diagnostic;
};
struct LoadOutcome
{
    Status code = Status::InvalidData;
    Document document;
    QString diagnostic;
};

/** Flat RGBA8/sRGB KRA values, including hidden and off-canvas layer pixels.
 * Devices are borrowed, open, seekable, and remain open after the operation.
 * Read publishes one complete value after validation. The write device is empty.
 * Write validates and encodes
 * before touching the device; its caller owns atomic replacement of the file.
 * The caller supplies the GPU projection of the same pinned document version.
 */
class KRITAIMPEX_EXPORT RasterCodec
{
public:
    static LoadOutcome read(QIODevice &source, Limits limits = {});
    static Outcome write(QIODevice &destination, const Document &document, const QImage &projection, Limits limits = {});
};
}
#endif
