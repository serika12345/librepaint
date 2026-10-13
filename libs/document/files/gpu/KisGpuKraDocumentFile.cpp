/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../../gpu/KisGpuPaintDocument_p.h"
#include <kra/KisKraRasterCodec.h>
#include <QColorSpace>
#include <QFile>
#include <QImage>
#include <QSaveFile>
#include <cstring>
#include <stdexcept>

namespace Krita::Document::Gpu {
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
}

FileOpenOutcome PaintDocument::openKra(Execution &execution, const QString &path, Limits limits)
{
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly)) return {FileCode::IoFailure, {}, input.errorString()};
    const auto loaded = Kra::RasterCodec::read(input, Private::formatLimits(limits));
    if (loaded.code != Kra::Status::Success) return {fileCode(loaded.code), {}, loaded.diagnostic};
    try {
        auto document = std::make_unique<PaintDocument>(execution, loaded.document.image, limits);
        document->m_d->path = path;
        document->m_d->description = loaded.document.description;
        document->m_d->files = loaded.document.files;
        return {FileCode::Success, std::move(document), {}};
    } catch (const std::length_error &error) {
        return {FileCode::CapacityExceeded, {}, QString::fromUtf8(error.what())};
    } catch (const std::invalid_argument &error) {
        return {FileCode::InvalidData, {}, QString::fromUtf8(error.what())};
    }
}

FileOutcome PaintDocument::saveKra(const QString &path, const Snapshot &version)
{
    auto &d = *m_d;
    if (!onThread()) return {FileCode::InvalidData, {}, QStringLiteral("Saving requires the document thread")};
    if (!d.handle) return {FileCode::Closed, {}, QStringLiteral("The painting document is closed")};
    const auto pinned = version ? version : d.document->snapshot();
    if (!pinned || pinned->documentId() != d.document->snapshot()->documentId() || !pinned->isCommitted() || path.isEmpty())
        return {FileCode::InvalidData, {}, QStringLiteral("Saving requires a committed version of this document and a path")};
    const auto limits = Private::formatLimits(d.limits);
    if (qint64(pinned->size.width()) * pinned->size.height() * 4 > limits.archiveBytes)
        return {FileCode::CapacityExceeded, {}, QStringLiteral("KRA projection pixel capacity exceeded")};
    QList<LayerId> ids;
    for (const auto &layer : pinned->layers) ids.append(layer.id);
    constexpr int limit = RgbaTile::CoordinateLimit;
    const auto layers = d.execution.readPixels(d.handle, pinned, {-limit, -limit, 2 * limit, 2 * limit},
                                               ids, false, PixelReadArea::LayerCoordinates);
    if (layers.code != ExecutionCode::Success) return {fileCode(layers.code), {}, layers.diagnostic};
    const auto projected = d.execution.readPixels(d.handle, pinned, QRect(QPoint(0, 0), pinned->size));
    if (projected.code != ExecutionCode::Success) return {fileCode(projected.code), {}, projected.diagnostic};
    Kra::Document output;
    output.description = d.description;
    output.files = d.files;
    output.image.size = pinned->size;
    QHash<LayerId, qsizetype> indices;
    for (const auto &layer : pinned->layers) {
        indices.insert(layer.id, output.image.layers.size());
        if (pinned->activeLayer == layer.id) output.image.activeLayer = output.image.layers.size();
        output.image.layers.append({layer.name, layer.visible, layer.uuid, {}, layer.defaultPixel});
    }
    for (const auto &tile : layers.tiles) output.image.layers[indices.value(tile.layer)].tiles.append({tile.coordinate, tile.rgba});
    QImage projection(pinned->size, QImage::Format_RGBA8888);
    if (projection.isNull()) return {FileCode::CapacityExceeded, {}, QStringLiteral("KRA projection allocation failed")};
    projection.fill(Qt::transparent);
    projection.setColorSpace(QColorSpace::SRgb);
    const QRect canvas(QPoint(0, 0), pinned->size);
    for (const auto &tile : projected.tiles) {
        const auto origin = tile.coordinate * RgbaTile::Size;
        const auto region = QRect(origin, QSize(RgbaTile::Size, RgbaTile::Size)).intersected(canvas);
        for (int y = region.top(); y <= region.bottom(); ++y) {
            const auto source = tile.rgba.constData() + ((y - origin.y()) * RgbaTile::Size + region.x() - origin.x()) * 4;
            std::memcpy(projection.scanLine(y) + region.x() * 4, source, size_t(region.width()) * 4);
        }
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return {FileCode::IoFailure, {}, file.errorString()};
    const auto written = Kra::RasterCodec::write(file, output, projection, limits);
    if (written.code != Kra::Status::Success) { file.cancelWriting(); return {fileCode(written.code), {}, written.diagnostic}; }
    if (!file.commit()) return {FileCode::IoFailure, {}, file.errorString()};
    d.document->markSaved(pinned);
    d.path = path;
    Q_EMIT changed();
    return {FileCode::Success, pinned, {}};
}
}
