/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../../gpu/KisGpuPaintDocument_p.h"
#include <kra/KisKraRasterCodec.h>
#include <QFile>
#include <QSaveFile>
#include <stdexcept>

namespace LibrePaint::Document::Gpu {
using namespace Image::Gpu;
namespace Kra = ImportExport::Kra;
namespace {
FileCode fileCode(Kra::Status code)
{
    switch (code) {
    case Kra::Status::Success: return FileCode::Success;
    case Kra::Status::InvalidData: return FileCode::InvalidData;
    case Kra::Status::UnsupportedData: return FileCode::UnsupportedData;
    case Kra::Status::CapacityExceeded: return FileCode::CapacityExceeded;
    case Kra::Status::IoFailure: return FileCode::IoFailure;
    }
    return FileCode::InvalidData;
}
FileCode fileCode(ExecutionCode code)
{
    switch (code) {
    case ExecutionCode::Success: return FileCode::Success;
    case ExecutionCode::InvalidRequest: return FileCode::InvalidData;
    case ExecutionCode::CapacityExceeded: return FileCode::CapacityExceeded;
    case ExecutionCode::Busy: return FileCode::Busy;
    case ExecutionCode::DeviceFailure:
    case ExecutionCode::SurfaceOutOfDate: return FileCode::DeviceFailure;
    }
    return FileCode::DeviceFailure;
}

RasterLayer rasterLayerDescription(const Layer &layer)
{
    RasterLayer value{layer.name, layer.visible, layer.uuid, {}, layer.defaultPixel, layer.opacity,
                      layer.userLocked, layer.pinnedToTimeline, layer.channelWriteMask};
    value.position = layer.position;
    value.kind = layer.kind;
    value.parentUuid = layer.parentUuid;
    value.colorLabelIndex = layer.colorLabelIndex;
    value.projectionChannelMask = layer.projectionChannelMask;
    value.passThrough = layer.passThrough;
    value.blendMode = layer.blendMode;
    return value;
}
}

FileOpenOutcome PaintDocument::openKra(Execution &execution, const QString &path, Limits limits)
{ return openKraImpl(execution, {}, path, limits); }

FileOpenOutcome PaintDocument::openKra(std::shared_ptr<Execution> execution, const QString &path, Limits limits)
{
    if (!execution) return {FileCode::InvalidData, {}, QStringLiteral("Opening requires a GPU execution owner")};
    return openKraImpl(*execution, execution, path, limits);
}

FileOpenOutcome PaintDocument::openKra(std::shared_ptr<Execution> execution, const Kra::Document &document, Limits limits)
{
    if (!execution) return {FileCode::InvalidData, {}, QStringLiteral("Opening requires a GPU execution owner")};
    return adoptKraImpl(*execution, execution, document, {}, limits);
}

FileOpenOutcome PaintDocument::openKraImpl(Execution &execution, std::shared_ptr<Execution> retainedExecution,
                                          const QString &path, Limits limits)
{
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly)) return {FileCode::IoFailure, {}, input.errorString()};
    const auto loaded = Kra::RasterCodec::read(input, Private::formatLimits(limits));
    if (loaded.code != Kra::Status::Success) return {fileCode(loaded.code), {}, loaded.diagnostic};
    return adoptKraImpl(execution, retainedExecution, loaded.document, path, limits);
}

FileOpenOutcome PaintDocument::adoptKraImpl(Execution &execution, std::shared_ptr<Execution> retainedExecution,
                                           const Kra::Document &loaded, const QString &path, Limits limits)
{
    const auto metadata = Kra::RasterCodec::validateMetadata(loaded.description, loaded.files, Private::formatLimits(limits));
    if (metadata.code != Kra::Status::Success) return {fileCode(metadata.code), {}, metadata.diagnostic};
    try {
        const auto image = retainedExecution ? PaintImageSP::create(retainedExecution, loaded.image, limits) :
                                              PaintImageSP::create(execution, loaded.image, limits);
        auto document = std::make_unique<PaintDocument>(image, limits);
        document->m_d->path = path;
        document->m_d->description = loaded.description;
        document->m_d->files = loaded.files;
        document->m_d->compressRaster = loaded.compressRaster;
        return {FileCode::Success, std::move(document), {}};
    } catch (const std::length_error &error) {
        return {FileCode::CapacityExceeded, {}, QString::fromUtf8(error.what())};
    } catch (const std::invalid_argument &error) {
        return {FileCode::InvalidData, {}, QString::fromUtf8(error.what())};
    }
}

Kra::MetadataOutcome PaintDocument::kraImageMetadata() const
{
    if (!onThread()) return {Kra::Status::InvalidData, {}, QStringLiteral("Metadata reads require the document thread"), {}};
    return Kra::RasterCodec::readImageMetadata(m_d->description, m_d->files, Private::formatLimits(m_d->limits));
}

FileOutcome PaintDocument::saveKra(const QString &path, const Snapshot &version)
{ return writeKra(path, version, true); }

FileOutcome PaintDocument::setKraImageMetadata(const Kra::ImageMetadata &metadata)
{
    if (!onThread()) return {FileCode::InvalidData, {}, QStringLiteral("Metadata updates require the document thread")};
    if (!m_d->image->handle()) return {FileCode::Closed, {}, QStringLiteral("The painting document is closed")};
    Kra::Document value;
    value.description = m_d->description;
    value.files = m_d->files;
    if (metadata.layerXmp || metadata.layerCollapsed) {
        const auto version = m_d->image->operations().snapshot();
        value.image.size = version->size;
        value.image.projectionBackground = version->projectionBackground;
        for (const auto &layer : version->layers) {
            if (layer.id == version->activeLayer) value.image.activeLayer = value.image.layers.size();
            value.image.layers.append(rasterLayerDescription(layer));
        }
    }
    const auto outcome = Kra::RasterCodec::updateImageMetadata(value, metadata, Private::formatLimits(m_d->limits));
    if (outcome.code != Kra::Status::Success) return {fileCode(outcome.code), {}, outcome.diagnostic};
    m_d->description = std::move(value.description);
    m_d->files = std::move(value.files);
    m_d->compressRaster = value.compressRaster;
    return {FileCode::Success, m_d->image->operations().snapshot(), {}};
}

FileOutcome PaintDocument::exportKra(const QString &path, const Snapshot &version)
{ return writeKra(path, version, false); }

FileOutcome PaintDocument::writeKra(const QString &path, const Snapshot &version, bool adoptFile)
{
    if (!onThread()) return {FileCode::InvalidData, {}, QStringLiteral("Saving requires the document thread")};
    if (!m_d->image->handle()) return {FileCode::Closed, {}, QStringLiteral("The painting document is closed")};
    if (path.isEmpty()) return {FileCode::InvalidData, {}, QStringLiteral("Saving requires a path")};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return {FileCode::IoFailure, {}, file.errorString()};
    const auto written = exportKra(file, version);
    if (written.code != FileCode::Success) { file.cancelWriting(); return written; }
    if (!file.commit()) return {FileCode::IoFailure, {}, file.errorString()};
    if (adoptFile) return adoptSavedFile(path, written.version);
    return written;
}

FileOutcome PaintDocument::exportKra(QIODevice &destination, const Snapshot &version)
{
    auto &d = *m_d;
    if (!onThread()) return {FileCode::InvalidData, {}, QStringLiteral("Saving requires the document thread")};
    if (!d.image->handle()) return {FileCode::Closed, {}, QStringLiteral("The painting document is closed")};
    const auto pinned = version ? version : d.image->operations().snapshot();
    if (!pinned || pinned->documentId() != d.image->operations().snapshot()->documentId() || !pinned->isCommitted())
        return {FileCode::InvalidData, {}, QStringLiteral("Saving requires a committed version of this document")};
    const auto limits = Private::formatLimits(d.limits);
    if (qint64(pinned->size.width()) * pinned->size.height() * 4 > limits.archiveBytes)
        return {FileCode::CapacityExceeded, {}, QStringLiteral("KRA projection pixel capacity exceeded")};
    Kra::Document output;
    output.description = d.description;
    output.files = d.files;
    output.compressRaster = d.compressRaster;
    output.image.size = pinned->size;
    output.image.projectionBackground = pinned->projectionBackground;
    qsizetype rasterBytes = 0;
    for (const auto &layer : pinned->layers) {
        if (pinned->activeLayer == layer.id) output.image.activeLayer = output.image.layers.size();
        RasterLayer saved = rasterLayerDescription(layer);
        if (layer.kind == LayerKind::Paint) {
            const auto pixels = d.execution.readLayerPixels(d.image->handle(), pinned, layer.id, limits.rasterBytes - rasterBytes);
            if (pixels.code != ExecutionCode::Success) return {fileCode(pixels.code), {}, pixels.diagnostic};
            saved.writtenAreas = pixels.writtenAreas;
            for (const auto &tile : pixels.tiles) {
                saved.tiles.append({tile.coordinate, tile.rgba});
                rasterBytes += tile.rgba.size();
            }
        }
        output.image.layers.append(std::move(saved));
    }
    const auto projection = d.execution.readProjectionImage(d.image->handle(), pinned, limits.archiveBytes);
    if (projection.code != ExecutionCode::Success) return {fileCode(projection.code), {}, projection.diagnostic};
    const auto written = Kra::RasterCodec::write(destination, output, projection.image, limits);
    if (written.code != Kra::Status::Success) return {fileCode(written.code), {}, written.diagnostic};
    return {FileCode::Success, pinned, {}};
}
}
