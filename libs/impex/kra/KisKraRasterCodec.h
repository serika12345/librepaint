/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_KRA_RASTER_CODEC_H
#define KIS_KRA_RASTER_CODEC_H

#include <gpu/KisGpuOperationDocument.h>
#include <QMap>
#include <kritaimpexkra_export.h>
#include <functional>
#include <optional>

class QIODevice;
class QImage;

namespace LibrePaint::ImportExport::Kra {
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
    /** Current save setting for layer checkpoints and trailing metadata. */
    bool compressRaster = false;
};
struct Annotation
{
    QString type;
    QString description;
    QByteArray bytes;
};
struct ImageMetadata
{
    QPointF resolution{72, 72};
    /** Saving application version; empty retains the existing producer value. */
    QString producerVersion;
    bool compressRaster = false;
    QByteArray documentInfo;
    QByteArray iccProfile;
    QList<Annotation> annotations;
    /** Serialized MirrorAxis settings; their values remain owned by the UI configuration. */
    QByteArray mirrorAxis;
    /** Serialized ColorHistory and GlobalAssistantsColor settings, owned by the document.
     * Empty values remove their respective settings from the saved image.
     */
    QByteArray colorHistory;
    QByteArray assistantsGlobalColor;
    /** Inline legacy timing and the current animation-metadata sidecar.
     * Nonempty values replace the corresponding representation; empty values
     * preserve retained settings for callers that update other metadata.
     */
    QByteArray animation;
    QByteArray animationMetadata;
    /** Nonempty values replace the saved profile label; empty retains its current label. */
    QString profileName;
    /** Root and public-layer XMP packets. Missing values preserve unowned data;
     * present empty values clear the corresponding owned packets. Updating
     * layerXmp uses the current structure in Document::image, without encoding
     * its raster tiles. Keys identify the current paint layers and groups.
     */
    std::optional<QByteArray> rootLayerXmp;
    std::optional<QMap<QUuid, QByteArray>> layerXmp;
    /** Public node expansion state, independent of painting history. Missing
     * preserves retained values; present values update the current structure,
     * with omitted identities expanded. Keys require current layer UUIDs.
     */
    std::optional<QMap<QUuid, bool>> layerCollapsed;
};
struct Outcome
{
    Status code = Status::InvalidData;
    QString diagnostic;
};
struct MetadataOutcome
{
    Status code = Status::InvalidData;
    ImageMetadata metadata;
    QString diagnostic;
};
struct LoadOutcome
{
    Status code = Status::InvalidData;
    Document document;
    QString diagnostic;
};

/** RGBA8/sRGB and default RGBA16/linear-sRGB KRA values, including hidden and off-canvas layer pixels.
 * Devices are borrowed, open, seekable, and remain open after the operation.
 * Read publishes one complete value after validation. The write device is empty.
 * Write validates and encodes
 * before touching the device; its caller owns atomic replacement of the file.
 * The caller supplies the GPU projection of the same pinned document version.
 */
class KRITAIMPEXKRA_EXPORT RasterCodec
{
public:
    static LoadOutcome read(QIODevice &source, Limits limits = {});
    /** Validates retained XML and associated files without allocating raster pixels. */
    static Outcome validateMetadata(const QByteArray &description, const QMap<QString, QByteArray> &files, Limits limits = {});
    /** Returns validated image metadata;
     * failure returns no partial values.
     */
    static MetadataOutcome readImageMetadata(const QByteArray &description, const QMap<QString, QByteArray> &files, Limits limits = {});
    /** Validates and adopts current image metadata while preserving other format data. */
    static Outcome updateImageMetadata(Document &document, const ImageMetadata &metadata, Limits limits = {});
    /** Canonical 256-pixel preview from fixed straight RGBA8/sRGB rows.
     * Reads synchronously and retains one source row plus the preview and weights.
     * File encoding supplies profile tagging and physical resolution.
     * Dimensions use the raster coordinate limit. At most height + 512 rows are
     * requested. Invalid dimensions or incomplete rows return a null image;
     * reader exceptions propagate to the caller.
     */
    static QImage createPreview(QSize sourceSize, const std::function<QByteArray(int)> &readRgbaRow);
    static Outcome write(QIODevice &destination, const Document &document, const QImage &projection, Limits limits = {});
    /** RGBA16 additionally supplies the native display preview of this fixed version. */
    static Outcome write(QIODevice &destination, const Document &document, const QImage &projection, Limits limits, const QImage &preview);
};
}
#endif
