/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include <QSet>
#include <limits>
#include <stdexcept>

namespace Krita::Image::Gpu {
OperationDocument::OperationDocument(const RasterImage &image, Limits limits)
    : m_limits(limits), m_identity(QUuid::createUuid())
{
    if (image.size.width() <= 0 || image.size.height() <= 0 ||
        image.size.width() > CoordinateLimit || image.size.height() > CoordinateLimit ||
        limits.samplesPerStroke <= 0 || limits.samplesPerDocument <= 0 || limits.layerCount <= 0 ||
        limits.recoveryBytes <= 0 || limits.recoveryBytes > std::numeric_limits<int>::max() ||
        limits.rasterBytes <= 0 || limits.rasterBytes > std::numeric_limits<int>::max() ||
        image.layers.size() > limits.layerCount || image.activeLayer < 0 ||
        (image.layers.isEmpty() ? image.activeLayer != 0 : image.activeLayer >= image.layers.size())) {
        throw std::invalid_argument("invalid raster document dimensions, layers or limits");
    }
    QSharedPointer<DocumentVersion> initial(new DocumentVersion);
    initial->m_document = m_identity;
    initial->m_committed = true;
    initial->size = image.size;
    QSet<QUuid> identities;
    for (const auto &source : image.layers) {
        const auto uuid = source.uuid.isNull() ? QUuid::createUuid() : source.uuid;
        if (identities.contains(uuid) || !Detail::validLayerName(source.name) ||
            source.tiles.size() > (limits.rasterBytes - initial->rasterBytes) / RgbaTile::ByteCount) {
            throw std::invalid_argument("invalid raster layer identity, name or pixel capacity");
        }
        Layer layer;
        layer.id = initial->layers.size() + 1;
        layer.name = source.name;
        layer.visible = source.visible;
        layer.uuid = uuid;
        layer.defaultPixel = source.defaultPixel;
        identities.insert(uuid);
        QSet<QPoint> coordinates;
        auto operation = QSharedPointer<TileWriteOperation>::create();
        for (const auto &pixels : source.tiles) {
            if (coordinates.contains(pixels.coordinate)) throw std::invalid_argument("repeated raster tile coordinate");
            const auto tile = RasterTile::fromPixels(pixels);
            if (!tile) throw std::invalid_argument("invalid raster tile bytes or coordinates");
            coordinates.insert(pixels.coordinate);
            operation->tiles.append(tile);
        }
        if (!operation->tiles.isEmpty()) {
            operation->id = {m_identity, ++m_nextOperation};
            operation->layer = layer.id;
            layer.operations.append(TileWriteSnapshot(operation));
            initial->rasterBytes += operation->tiles.size() * RgbaTile::ByteCount;
        }
        initial->layers.append(layer);
    }
    m_nextLayer = initial->layers.size();
    initial->activeLayer = image.layers.isEmpty() ? 0 : image.activeLayer + 1;
    m_history.append(initial);
}

Result OperationDocument::replaceTiles(LayerId layer, const QList<RgbaTile> &tiles)
{ return applyTiles(layer, tiles, true); }

Result OperationDocument::writeTiles(LayerId layer, const QList<RgbaTile> &tiles)
{ return applyTiles(layer, tiles, false); }

Result OperationDocument::applyTiles(LayerId layer, const QList<RgbaTile> &tiles, bool recordHistory)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (tiles.isEmpty()) return Result::EmptyOperation;
    if (tiles.size() > (m_limits.rasterBytes - snapshot()->rasterBytes) / RgbaTile::ByteCount ||
        m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QList<QPair<int, int>> affected;
    if (!recordHistory) {
        const auto &current = snapshot()->layers[index];
        for (int i = 0; i < m_history.size(); ++i) {
            const auto &state = m_history[i];
            for (int j = 0; j < state->layers.size(); ++j) {
                const auto &candidate = state->layers[j];
                if (candidate.id == layer && candidate.operations == current.operations &&
                    candidate.defaultPixel == current.defaultPixel) {
                    if (tiles.size() > (m_limits.rasterBytes - state->rasterBytes) / RgbaTile::ByteCount)
                        return Result::CapacityExceeded;
                    affected.append({i, j});
                }
            }
        }
        if (quint64(affected.size()) > std::numeric_limits<quint64>::max() - m_nextRevision)
            return Result::CapacityExceeded;
    }
    QSet<QPoint> coordinates;
    auto operation = QSharedPointer<TileWriteOperation>::create();
    for (const auto &pixels : tiles) {
        if (coordinates.contains(pixels.coordinate)) return Result::InvalidInput;
        const auto tile = RasterTile::fromPixels(pixels);
        if (!tile) return Result::InvalidInput;
        coordinates.insert(pixels.coordinate);
        operation->tiles.append(tile);
    }
    operation->id = {m_identity, ++m_nextOperation};
    operation->version = 1;
    operation->layer = layer;
    if (recordHistory) {
        QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
        version->layers[index].operations.append(TileWriteSnapshot(operation));
        version->rasterBytes += tiles.size() * RgbaTile::ByteCount;
        version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
        commitVersion(version);
    } else {
        for (const auto &[historyIndex, layerIndex] : affected) {
            QSharedPointer<DocumentVersion> version(new DocumentVersion(*m_history[historyIndex]));
            version->layers[layerIndex].operations.append(TileWriteSnapshot(operation));
            version->rasterBytes += tiles.size() * RgbaTile::ByteCount;
            version->m_revision = ++m_nextRevision;
            m_history[historyIndex] = version;
        }
    }
    return Result::Success;
}
}
