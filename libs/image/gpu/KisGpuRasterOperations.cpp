/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include <QSet>
#include <limits>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
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
        layer.opacity = source.opacity;
        layer.userLocked = source.userLocked;
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
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (tiles.isEmpty()) return Result::EmptyOperation;
    if (tiles.size() > (m_limits.rasterBytes - snapshot()->rasterBytes) / RgbaTile::ByteCount ||
        m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
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
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->layers[index].operations.append(TileWriteSnapshot(operation));
    version->rasterBytes += tiles.size() * RgbaTile::ByteCount;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitVersion(version);
    return Result::Success;
}

Result OperationDocument::applyPixelVersions(const QList<VersionPixelWrite> &writes, bool recordHistory)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (writes.isEmpty() || writes.size() > m_history.size()) return Result::InvalidInput;
    if (recordHistory && (writes.size() != 1 || writes.first().before != snapshot())) return Result::StaleVersion;
    if (quint64(writes.size()) > std::numeric_limits<quint64>::max() - m_nextOperation ||
        quint64(writes.size()) > std::numeric_limits<quint64>::max() - m_nextRevision)
        return Result::CapacityExceeded;
    auto history = m_history;
    auto nextOperation = m_nextOperation;
    auto nextRevision = m_nextRevision;
    const auto group = nextOperation + 1;
    QSet<const DocumentVersion *> updated;
    for (const auto &write : writes) {
        const auto index = m_history.indexOf(write.before);
        if (index < 0 || updated.contains(write.before.data())) return Result::StaleVersion;
        if (write.layer != writes.first().layer || write.tiles.isEmpty()) return Result::InvalidInput;
        qsizetype layerIndex = -1;
        for (qsizetype i = 0; i < write.before->layers.size(); ++i)
            if (write.before->layers[i].id == write.layer) layerIndex = i;
        if (layerIndex < 0) return Result::LayerUnavailable;
        if (write.tiles.size() > (m_limits.rasterBytes - write.before->rasterBytes) / RgbaTile::ByteCount)
            return Result::CapacityExceeded;
        auto operation = QSharedPointer<TileWriteOperation>::create();
        operation->id = {m_identity, ++nextOperation};
        operation->version = 1;
        operation->layer = write.layer;
        operation->writeGroup = group;
        operation->writtenAreas = write.areas;
        QSet<QPoint> coordinates;
        for (const auto &tile : write.tiles) {
            if (!tile || coordinates.contains(tile->coordinate())) return Result::InvalidInput;
            coordinates.insert(tile->coordinate());
            operation->tiles.append(tile);
        }
        if (!Detail::validWrittenAreas(*operation)) return Result::InvalidInput;
        QSharedPointer<DocumentVersion> version(new DocumentVersion(*write.before));
        version->layers[layerIndex].operations.append(TileWriteSnapshot(operation));
        version->rasterBytes += write.tiles.size() * RgbaTile::ByteCount;
        version->m_revision = ++nextRevision;
        if (recordHistory) version->m_modifiedRevision = version->m_revision;
        history[index] = version;
        updated.insert(write.before.data());
    }
    if (recordHistory) commitVersion(history[m_index]);
    else m_history.swap(history);
    m_nextOperation = nextOperation;
    m_nextRevision = nextRevision;
    return Result::Success;
}
}
