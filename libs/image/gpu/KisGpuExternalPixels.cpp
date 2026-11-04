/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPaintImage_p.h"
#include "KisGpuPaintGeometry_p.h"
#include "KisGpuAutoBrush_p.h"
#include "KisGpuMarkerBrush_p.h"
#include <QHash>
#include <QMap>
#include <QRegion>
#include <tuple>
#include <algorithm>
#include <cstring>
#include <new>

namespace LibrePaint::Image::Gpu {
namespace {
Result readFailure(ExecutionCode code)
{
    if (code == ExecutionCode::CapacityExceeded) return Result::CapacityExceeded;
    if (code == ExecutionCode::Busy) return Result::Busy;
    return Result::DeviceFailure;
}
}

PaintImageSP PaintImage::cloneRasterLayer(const Snapshot &version, LayerId layer, QRect region,
                                        const QUuid &destinationUuid, qsizetype maximumWorkingBytes,
                                        const SelectionMaskSP &mask, bool invertMask) const
{
    if (!onThread() || !m_d->handle || !version || m_d->document->hasActiveStroke() ||
        version->documentId() != m_d->document->snapshot()->documentId() || destinationUuid.isNull() ||
        maximumWorkingBytes < 0) return {};
    constexpr int limit = RgbaTile::CoordinateLimit, size = RgbaTile::Size;
    if (region.isEmpty() || region.width() > limit || region.height() > limit ||
        region.x() < -limit || region.y() < -limit || qint64(region.x()) + region.width() > limit ||
        qint64(region.y()) + region.height() > limit) return {};
    const qint64 columns = (qint64(region.width()) + size - 1) / size;
    const qint64 rows = (qint64(region.height()) + size - 1) / size;
    const int pixelSize = pixelByteCount(version->format);
    const int bytesPerTile = RgbaTile::byteCount(version->format);
    const qint64 pixelBytes = qint64(region.width()) * region.height() * pixelSize;
    const qint64 tileBytes = columns * rows * bytesPerTile;
    if (pixelBytes + 2 * tileBytes > maximumWorkingBytes) return {};
    if (!layer) {
        const auto direct = version->directProjectionLayer();
        QUuid uuid;
        for (const auto &node : version->layers) if (node.id == direct) { uuid = node.uuid; break; }
        const bool filtered = std::any_of(version->layers.cbegin(), version->layers.cend(), [&](const auto &node) {
            return node.kind == LayerKind::FilterMask && node.parentUuid == uuid && node.visible;
        });
        if (!filtered) layer = direct;
    }
    auto pixels = m_d->execution.readRgbaPixels(m_d->handle, version, region, layer, pixelBytes, mask, invertMask);
    if (pixels.code != ExecutionCode::Success) return {};
    RasterLayer properties{QStringLiteral("Projection"), true, destinationUuid, {}};
    RasterImage input;
    input.size = region.size();
    input.format = version->format;
    input.colorProfile = version->colorProfile;
    input.layers = {properties};
    qsizetype retainedBytes = 0;
    for (int y = 0; y < rows; ++y) for (int x = 0; x < columns; ++x) {
        QByteArray tile(bytesPerTile, '\0');
        const int width = std::min(size, region.width() - x * size);
        const int height = std::min(size, region.height() - y * size);
        for (int row = 0; row < height; ++row) {
            const auto source = pixels.bytes.constData() + (qint64(y * size + row) * region.width() + x * size) * pixelSize;
            std::memcpy(tile.data() + row * size * pixelSize, source, width * pixelSize);
        }
        if (std::none_of(tile.cbegin(), tile.cend(), [](char value) { return value != 0; })) continue;
        if (bytesPerTile > m_d->limits.rasterBytes - retainedBytes) return {};
        retainedBytes += bytesPerTile;
        input.layers.first().tiles.append({{x, y}, std::move(tile), version->format});
    }
    OperationDocument prepared(input, m_d->limits);
    pixels.bytes.clear();
    input.layers.clear();
    return cloneLayerForest({{prepared.snapshot(), prepared.activeLayer(), properties}}, region.size());
}

Result PaintImage::writeRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes)
{ return applyRgbaPixels(layer, region, bytes, false); }

Result PaintImage::writeRgbaPixels(const LayerPixelReferenceSP &pixels, QRect region, const QByteArray &bytes)
{
    if (!onThread() || !pixels || m_d->layerPixels.value(pixels->m_uuid).toStrongRef() != pixels)
        return Result::InvalidInput;
    if (pixels->m_version->documentId() != m_d->document->snapshot()->documentId()) return Result::StaleVersion;
    return applyRgbaPixels(pixels->m_layer, region, bytes, false, &pixels->m_version);
}

Result PaintImage::replaceRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes, const QString &filterId)
{ return applyRgbaPixels(layer, region, bytes, true, nullptr, filterId); }

Result PaintImage::publishFilterResult(Result result, const Snapshot &before, const QString &filterId)
{
    auto &document = *m_d->document;
    if (result == Result::Success && document.m_pixelEdit && document.m_pixelEdit->updating) {
        document.m_pixelEdit->change.filterId = filterId;
        return result;
    }
    if (result == Result::Success && document.snapshot() != before)
        document.m_layerHistoryChanges[document.m_index].filterId = filterId;
    return publish(result);
}

Result PaintImage::replaceMaskPixels(LayerId layer, const QUuid &maskUuid, QRect region,
                                    const QByteArray &bytes, const QString &filterId)
{
    if (!onThread()) return Result::InvalidInput;
    auto &document = *m_d->document;
    const auto status = document.checkMutation();
    if (status != Result::Success) return status;
    const auto before = document.snapshot();
    SelectionMaskSP source;
    QPoint position;
    if (layer) {
        const auto found = std::find_if(before->layers.cbegin(), before->layers.cend(),
            [&](const auto &value) { return value.id == layer; });
        if (found == before->layers.cend() || maskUuid.isNull()) return Result::LayerUnavailable;
        source = editableMaskCoverage(*found, maskUuid);
        if (!source) return Result::LayerUnavailable;
        if (found->userLocked || (maskUuid != found->uuid && found->transparencyMaskLocked)) return Result::LayerUnavailable;
        position = maskUuid == found->uuid ? found->position : found->transparencyMaskPosition;
    } else {
        if (!maskUuid.isNull() || before->selection.isEmpty()) return Result::LayerUnavailable;
        const QRect canvas(QPoint(), before->size);
        source = before->selectionKind == SelectionKind::Freehand ? before->selectionMask :
            before->selectionKind == SelectionKind::Ellipse ? SelectionMask::fromEllipse(before->selection, canvas | before->selection) :
                SelectionMask::fromRectangle(before->selection, canvas | before->selection);
        if (before->selectionInverted && source) source = complementSelectionMask(source, canvas);
        if (!source) return Result::CapacityExceeded;
    }
    // A detached write prepares new coverage without touching the current
    // document version. Adoption follows only after every transfer succeeds.
    const auto result = applyMaskPixels(0, {}, region, bytes, &source, before->size, position);
    if (result != Result::Success) return result;
    if (document.snapshot() != before) return Result::StaleVersion;
    const auto adopted = layer ? document.replaceTransparencyMask(layer, source, false, false, maskUuid) :
        source->isEmpty() ? document.clearSelection() : document.setSelectionMask(source);
    return publishFilterResult(adopted, before, filterId);
}

Result PaintImage::applyRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes, bool recordHistory,
                                 Snapshot *retainedVersion, const QString &filterId)
{
    if (!onThread()) return Result::InvalidInput;
    auto &d = *m_d;
    auto &document = *d.document;
    const auto status = document.checkMutation();
    if (status != Result::Success) return status;
    const auto current = retainedVersion ? *retainedVersion : document.snapshot();
    const auto selectedLayer = std::find_if(current->layers.cbegin(), current->layers.cend(),
        [layer](const auto &value) { return value.id == layer; });
    if (selectedLayer == current->layers.cend() || selectedLayer->kind != LayerKind::Paint) return Result::LayerUnavailable;
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    if (region.width() < 0 || region.height() < 0 || region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit)
        return Result::InvalidInput;
    const int pixelSize = pixelByteCount(current->format);
    const int bytesPerTile = RgbaTile::byteCount(current->format);
    const qint64 byteCount = qint64(region.width()) * region.height() * pixelSize;
    if (bytes.size() < byteCount) return Result::InvalidInput;
    if (byteCount > d.limits.rasterBytes) return Result::CapacityExceeded;
    if (region.isEmpty()) return Result::Success;
    const auto &selected = *selectedLayer;
    constexpr int size = RgbaTile::Size;
    const qint64 columns = Detail::pixelTileIndex(region.right(), size) - Detail::pixelTileIndex(region.left(), size) + 1;
    const qint64 rows = Detail::pixelTileIndex(region.bottom(), size) - Detail::pixelTileIndex(region.top(), size) + 1;
    if (columns * rows > d.limits.rasterBytes / bytesPerTile)
        return Result::CapacityExceeded;
    Result adopted;
    try {
        OperationDocument::VersionPixelWrite write;
        write.before = current; write.layer = layer;
        QMap<quint64, QList<QRect>> areas;
        constexpr int blockSize = Detail::PixelHistoryBlockSize;
        const auto blockIndex = [](qint64 value) { return value >= 0 ? value / blockSize : (value - blockSize + 1) / blockSize; };
        for (qint64 y = blockIndex(qint64(region.top()) - selected.position.y()); y <= blockIndex(qint64(region.bottom()) - selected.position.y()); ++y)
            for (qint64 x = blockIndex(qint64(region.left()) - selected.position.x()); x <= blockIndex(qint64(region.right()) - selected.position.x()); ++x) {
                const QRect block(int(x * blockSize + selected.position.x()), int(y * blockSize + selected.position.y()), blockSize, blockSize);
                const auto area = block.intersected(region);
                for (int ty = Detail::pixelTileIndex(area.top(), size); ty <= Detail::pixelTileIndex(area.bottom(), size); ++ty)
                    for (int tx = Detail::pixelTileIndex(area.left(), size); tx <= Detail::pixelTileIndex(area.right(), size); ++tx)
                        areas[Detail::key(tx, ty)].append(area.intersected(QRect(tx * size, ty * size, size, size)));
            }
        for (auto tile = areas.cbegin(); tile != areas.cend(); ++tile) {
            const auto coordinate = Detail::coordinate(tile.key());
            const QRect tileRegion(coordinate * size, QSize(size, size));
            auto pixels = d.execution.readRgbaPixels(d.handle, current, tileRegion, layer, bytesPerTile);
            if (pixels.code != ExecutionCode::Success) return readFailure(pixels.code);
            for (const auto &area : tile.value()) {
                for (int y = area.top(); y <= area.bottom(); ++y) {
                    const qsizetype source = (qsizetype(y - region.y()) * region.width() + area.x() - region.x()) * pixelSize;
                    const qsizetype destination = ((y - tileRegion.y()) * size + area.x() - tileRegion.x()) * pixelSize;
                    std::memcpy(pixels.bytes.data() + destination, bytes.constData() + source, size_t(area.width()) * pixelSize);
                }
                write.areas.append(area);
            }
            const auto upload = UploadTile::fromPixels({coordinate, pixels.bytes, current->format});
            if (!upload) return Result::CapacityExceeded;
            write.tiles.append(upload);
        }
        adopted = document.applyPixelWrite(write, recordHistory, retainedVersion);
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
    if (recordHistory) return publishFilterResult(adopted, current, filterId);
    if (adopted == Result::Success) requestCurrentVersion();
    return adopted;
}
Result PaintImage::writeMaskPosition(const QUuid &maskUuid, QPoint position,
                                    SelectionMaskSP *retainedCoverage, QPoint retainedPosition)
{ return applyMaskPosition(maskUuid, position, retainedCoverage, retainedPosition, true); }

Result PaintImage::applyMaskPosition(const QUuid &maskUuid, QPoint position,
                                    SelectionMaskSP *retainedCoverage, QPoint retainedPosition, bool request)
{
    if (!onThread() || maskUuid.isNull() || !retainedCoverage || !*retainedCoverage) return Result::InvalidInput;
    auto &document = *m_d->document;
    const auto status = document.checkMutation();
    if (status != Result::Success && status != Result::Closed) return status;
    if (position == retainedPosition) return Result::Success;
    const auto selected = *retainedCoverage;
    Snapshot source;
    LayerId layerId = 0;
    if (status != Result::Closed) {
        const auto current = document.snapshot();
        for (const auto &layer : current->layers) {
            if (editableMaskCoverage(layer, maskUuid) != selected) continue;
            source = current; layerId = layer.id; break;
        }
    }
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    const auto move = [&](const SelectionMaskSP &coverage, QPoint before) -> SelectionMaskSP {
        const qint64 x = qint64(position.x()) - before.x(), y = qint64(position.y()) - before.y();
        if (x < -2 * limit || x > 2 * limit || y < -2 * limit || y > 2 * limit) return {};
        const auto bounds = coverage->allocatedBounds() | coverage->bounds();
        if (!bounds.isEmpty() && (qint64(bounds.left()) + x < -limit || qint64(bounds.right()) + x >= limit ||
            qint64(bounds.top()) + y < -limit || qint64(bounds.bottom()) + y >= limit)) return {};
        return translatedSelectionMask(coverage, QPoint(int(x), int(y)), domain);
    };
    try {
        const auto output = move(selected, retainedPosition);
        if (!output) return Result::CapacityExceeded;
        const auto result = source ? document.applyMaskWrite({source, layerId, maskUuid, output, position}) : Result::Success;
        if (result == Result::Success) {
            *retainedCoverage = output;
            if (request && source) requestCurrentVersion();
        }
        return result;
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}

Result PaintImage::writeMaskPixels(LayerId layer, const QUuid &maskUuid, QRect region, const QByteArray &bytes,
                                   SelectionMaskSP *retainedCoverage, QSize retainedCanvasSize, QPoint retainedPosition)
{
    if (!onThread() || maskUuid.isNull()) return Result::InvalidInput;
    return applyMaskPixels(layer, maskUuid, region, bytes, retainedCoverage, retainedCanvasSize, retainedPosition);
}

Result PaintImage::applyMaskPixels(LayerId layer, const QUuid &maskUuid, QRect region, const QByteArray &bytes,
                                   SelectionMaskSP *retainedCoverage, QSize retainedCanvasSize, QPoint retainedPosition)
{
    auto &d = *m_d;
    auto &document = *d.document;
    const auto status = document.checkMutation();
    const bool closed = status == Result::Closed;
    if (status != Result::Success && !(closed && retainedCoverage && *retainedCoverage)) return status;
    const auto current = closed ? Snapshot{} : document.snapshot();
    const auto findTarget = [&](const Snapshot &version) -> QPair<LayerId, SelectionMaskSP> {
        if (!version || maskUuid.isNull()) return {};
        for (const auto &node : version->layers) {
            const auto coverage = editableMaskCoverage(node, maskUuid);
            if (coverage) return {node.id, coverage};
        }
        return {};
    };
    const auto maskPosition = [&](const Snapshot &version) {
        if (version) for (const auto &node : version->layers)
            if (editableMaskCoverage(node, maskUuid)) return node.uuid == maskUuid ? node.position : node.transparencyMaskPosition;
        return retainedPosition;
    };
    const auto currentTarget = findTarget(current);
    if (layer && currentTarget.first && currentTarget.first != layer) return Result::LayerUnavailable;
    const auto selected = retainedCoverage ? *retainedCoverage : currentTarget.second;
    if (!selected) return Result::LayerUnavailable;
    const auto canvas = !retainedCanvasSize.isEmpty() ? retainedCanvasSize : current ? current->size : QSize{};
    if (canvas.isEmpty() || canvas.width() > RgbaTile::CoordinateLimit || canvas.height() > RgbaTile::CoordinateLimit)
        return Result::InvalidInput;
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    const qint64 count = qint64(region.width()) * region.height();
    if (region.width() < 0 || region.height() < 0 || region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit || bytes.size() < count)
        return Result::InvalidInput;
    if (count > SelectionMask::MaximumBytes || count > d.limits.rasterBytes) return Result::CapacityExceeded;
    if (region.isEmpty()) return Result::Success;
    try {
        RasterImage contextInput;
        contextInput.size = canvas;
        OperationDocument context(contextInput, d.limits);
        const auto sourceContext = context.snapshot();
        QMap<std::tuple<int, int, int, int>, SelectionMaskSP> inputs;
        qsizetype retainedBytes = 0;
        std::optional<OperationDocument::VersionMaskWrite> write;
        constexpr int blockSize = Detail::PixelHistoryBlockSize;
        const auto blockIndex = [](qint64 value) { return value >= 0 ? value / blockSize : (value - blockSize + 1) / blockSize; };
        const auto selectedPosition = retainedCoverage ? retainedPosition : maskPosition(current);
        const auto origin = selectedPosition;
        // A direct write changes only the current version; a detached mask keeps its own copy.
        const bool selectedInHistory = !closed && current && findTarget(current).second == selected;
        const QList<Snapshot> versions{selectedInHistory ? current : sourceContext};
        SelectionMaskSP selectedOutput;
        for (qsizetype index = 0; index < versions.size(); ++index) {
            const auto &version = versions[index];
            const bool retainedOnly = !selectedInHistory && index == versions.size() - 1;
            const auto target = findTarget(version);
            const auto previous = retainedOnly ? selected : target.second;
            if (!previous) continue;
            const auto previousPosition = retainedOnly ? selectedPosition : maskPosition(version);
            const qint64 dx = qint64(previousPosition.x()) - selectedPosition.x(), dy = qint64(previousPosition.y()) - selectedPosition.y();
            if (qint64(region.left()) + dx < -limit || qint64(region.right()) + dx >= limit ||
                qint64(region.top()) + dy < -limit || qint64(region.bottom()) + dy >= limit) return Result::InvalidInput;
            const QPoint placement{int(dx), int(dy)};
            QRegion allowed;
            for (qint64 y = blockIndex(qint64(region.top()) - origin.y()); y <= blockIndex(qint64(region.bottom()) - origin.y()); ++y)
                for (qint64 x = blockIndex(qint64(region.left()) - origin.x()); x <= blockIndex(qint64(region.right()) - origin.x()); ++x) {
                    const auto key = Detail::key(int(x), int(y));
                    const QRect block(int(x * blockSize + origin.x()), int(y * blockSize + origin.y()), blockSize, blockSize);
                    allowed += block.intersected(region).translated(placement);
                }
            if (allowed.isEmpty()) continue;
            const auto image = createMaskImage(sourceContext, previous);
            if (!image) return readFailure(d.lastRequest.code);
            const auto sourceLayer = image->operations().activeLayer();
            if (image->clearSelection() != Result::Success) return Result::CapacityExceeded;
            for (const auto &area : allowed) {
                const auto key = std::make_tuple(area.x(), area.y(), area.width(), area.height());
                auto imported = inputs.constFind(key);
                if (imported == inputs.cend()) {
                    QByteArray alpha(qsizetype(area.width()) * area.height(), Qt::Uninitialized);
                    for (int y = 0; y < area.height(); ++y)
                        std::memcpy(alpha.data() + qsizetype(y) * area.width(),
                            bytes.constData() + qsizetype(area.y() + y - placement.y() - region.y()) * region.width() + area.x() - placement.x() - region.x(), size_t(area.width()));
                    auto input = SelectionMask::fromAlphaCoverage(area, alpha, area);
                    if (!input) {
                        if (std::any_of(alpha.cbegin(), alpha.cend(), [](char value) { return value != 0; }))
                            return Result::CapacityExceeded;
                        input = SelectionMask::empty();
                    }
                    if (input->byteCount() > d.limits.rasterBytes - retainedBytes) return Result::CapacityExceeded;
                    retainedBytes += input->byteCount();
                    imported = inputs.insert(key, input);
                }
                const QPointF first(area.topLeft()), last(area.x() + area.width(), area.y() + area.height());
                const QList<InputSample> samples{{first, 1}, {last, 1}};
                const Shape rectangle{ShapeKind::Rectangle, first, last, 0};
                Brush copy; copy.blendMode = LayerBlendMode::Copy;
                FigureStyle clear; clear.outline = false; clear.fill = RgbaPixel{};
                clear.fillBlendMode = LayerBlendMode::Copy; clear.ignoreSelection = true;
                auto result = image->publish(image->m_d->document->paintFigureWithin(
                    sourceLayer, copy, samples, rectangle, clear, 0, area));
                if (result != Result::Success) return result;
                if (!(*imported)->isEmpty()) {
                    if (image->setSelectionMask(*imported) != Result::Success) return Result::CapacityExceeded;
                    Brush white; white.rgba = {255, 255, 255, 255};
                    FigureStyle fill; fill.outline = false; fill.fill = white.rgba;
                    result = image->publish(image->m_d->document->paintFigureWithin(
                        sourceLayer, white, samples, rectangle, fill, 0, area));
                    if (result != Result::Success) return result;
                }
            }
            const auto source = image->operations().snapshot();
            SelectionMaskSP output;
            const auto generated = d.execution.paintedSelectionCoverage(image->handle(), source, sourceLayer, &output);
            if (generated.code != ExecutionCode::Success) return readFailure(generated.code);
            if (!output || !document.selectionFitsInputs(output)) return Result::CapacityExceeded;
            if (previous == selected && previousPosition == selectedPosition) selectedOutput = output;
            if (!retainedOnly) write = OperationDocument::VersionMaskWrite{version, target.first, maskUuid, output};
        }
        if (!selectedOutput) return Result::LayerUnavailable;
        const auto adopted = write ? document.applyMaskWrite(*write) : Result::Success;
        if (adopted == Result::Success) {
            if (retainedCoverage) *retainedCoverage = selectedOutput;
            if (bool(write)) requestCurrentVersion();
        }
        return adopted;
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}

}
