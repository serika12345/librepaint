/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include "KisGpuOperationValues_p.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QHash>
#include <QIODevice>
#include <QSet>
#include <limits>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
constexpr quint32 Magic = 0x4c504f52; // LPOR
constexpr quint16 FormatVersion = 30;
constexpr quint32 NoOperation = std::numeric_limits<quint32>::max();
constexpr qsizetype DigestSize = 32;

void configure(QDataStream &stream)
{
    stream.setVersion(QDataStream::Qt_5_15);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
}
QByteArray digest(const QByteArray &data, qsizetype length)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(QByteArrayView(data.constData(), length));
    return hash.result();
}
bool readableCount(QDataStream &stream, quint32 count, qsizetype limit, qsizetype minimumBytes)
{
    return stream.status() == QDataStream::Ok && count <= quint64(limit) &&
        count <= quint64(stream.device()->bytesAvailable() / minimumBytes);
}
void writeSample(QDataStream &stream, const InputSample &sample)
{
    stream << sample.position.x() << sample.position.y() << sample.pressure
           << sample.xTilt << sample.yTilt << sample.rotation << sample.timestamp << sample.speed;
}
InputSample readSample(QDataStream &stream, quint16 format)
{
    InputSample sample;
    qreal x = 0, y = 0;
    stream >> x >> y >> sample.pressure >> sample.xTilt >> sample.yTilt
           >> sample.rotation >> sample.timestamp;
    sample.position = {x, y};
    if (format >= 21) stream >> sample.speed;
    return sample;
}
}

QByteArray OperationDocument::recoveryData() const
{
    QList<LayerOperation> operations;
    QHash<const void *, quint32> indices;
    QList<QSharedPointer<const RasterTile>> tiles;
    QHash<const RasterTile *, quint32> tileIndices;
    const auto registerOperation = [&](const LayerOperation &operation) {
        const auto pointer = operationPointer(operation);
        if (indices.contains(pointer)) return;
        indices.insert(pointer, quint32(operations.size()));
        operations.append(operation);
        if (const auto write = std::get_if<TileWriteSnapshot>(&operation)) {
            for (const auto &tile : (*write)->tiles) {
                if (!tileIndices.contains(tile.data())) {
                    tileIndices.insert(tile.data(), quint32(tiles.size()));
                    tiles.append(tile);
                }
            }
        }
    };
    for (const auto &state : m_history) {
        for (const auto &layer : state->layers) {
            for (const auto &operation : layer.operations) registerOperation(operation);
        }
    }
    if (m_edit) registerOperation(m_edit);
    if (m_translation) registerOperation(m_translation);
    if (quint64(operations.size()) >= NoOperation || quint64(tiles.size()) >= NoOperation) return {};
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    configure(stream);
    const auto withinLimit = [&] {
        return stream.status() == QDataStream::Ok && bytes.size() <= m_limits.recoveryBytes - DigestSize;
    };
    stream << Magic << FormatVersion << qint32(snapshot()->size.width()) << qint32(snapshot()->size.height())
           << m_nextRevision << m_nextOperation << m_nextLayer << m_savedRevision << quint32(tiles.size());
    if (!withinLimit()) return {};
    for (const auto &tile : tiles) {
        const auto data = tile->compressedData();
        if (data.size() > m_limits.recoveryBytes - DigestSize - bytes.size() - 12) return {};
        stream << qint32(tile->coordinate().x()) << qint32(tile->coordinate().y()) << quint32(data.size());
        stream.writeRawData(data.constData(), int(data.size()));
    }
    stream << quint32(operations.size());
    if (!withinLimit()) return {};
    for (const auto &value : operations) {
        const auto paint = std::get_if<PaintSnapshot>(&value);
        stream << quint8(paint ? 0 : std::holds_alternative<TileWriteSnapshot>(value) ? 1 :
                        std::holds_alternative<TranslationSnapshot>(value) ? 2 : 3)
               << operationId(value).sequence << operationVersion(value) << operationLayer(value);
        if (paint) {
            const auto &operation = **paint;
            for (const auto component : operation.brush.rgba) stream << component;
            stream << operation.brush.diameter << operation.brush.spacing << operation.brush.opacity
                   << quint8(quint8(operation.brush.erase) | (quint8(operation.brush.alphaLocked) << 1) |
                             ((15u ^ operation.brush.channelWriteMask) << 2) |
                             (quint8(operation.brush.mirrorHorizontal) << 6) | (quint8(operation.brush.mirrorVertical) << 7));
            if (operation.brush.mirrorHorizontal || operation.brush.mirrorVertical)
                stream << qint32(operation.brush.mirrorCenter.x()) << qint32(operation.brush.mirrorCenter.y());
            stream << operation.randomSeed << quint8(operation.shape.kind)
                   << operation.shape.first.x() << operation.shape.first.y()
                   << operation.shape.second.x() << operation.shape.second.y() << operation.shape.rotation;
            if (operation.shape.kind == ShapeKind::RoundedRectangle)
                stream << operation.shape.cornerRadii.width() << operation.shape.cornerRadii.height();
            if (operation.shape.kind == ShapeKind::Path) {
                if (operation.shape.pathCommands.size() > m_limits.recoveryBytes - DigestSize - bytes.size() - 5) return {};
                stream << quint8(operation.shape.windingFill) << quint32(operation.shape.pathCommands.size());
                for (const auto command : operation.shape.pathCommands) stream << quint8(command);
            }
            stream << quint8(quint8(operation.figureStyle.outline) | (operation.figureStyle.fill ? 2 : 0));
            if (operation.figureStyle.fill)
                for (const auto component : *operation.figureStyle.fill) stream << component;
            stream << quint8(quint8(operation.brush.pixelPerfect) | (quint8(operation.brush.basicSmoothing) << 1) |
                              (quint8(operation.smoothingFinished) << 2) | (operation.brush.weightedSmoothing ? 8 : 0));
            if (operation.brush.weightedSmoothing) {
                const auto &value = *operation.brush.weightedSmoothing;
                stream << value.distanceMin << value.distanceMax << value.tailAggressiveness << quint8(value.smoothPressure);
            }
            if (operation.brush.basicSmoothing) {
                const auto remaining = m_limits.recoveryBytes - DigestSize - bytes.size();
                if (remaining < 4 || quint64(operation.sampleCount) > std::numeric_limits<quint32>::max() ||
                    quint64(operation.smoothingFlushes.size()) > std::numeric_limits<quint32>::max() ||
                    operation.smoothingFlushes.size() > (remaining - 4) / 4) return {};
                stream << quint32(operation.smoothingFlushes.size());
                for (const auto count : operation.smoothingFlushes) stream << quint32(count);
            }
            stream << quint32(operation.sampleCount);
            if (!withinLimit()) return {};
            for (const auto &block : operation.sampleBlocks) {
                if (block->size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 64) return {};
                for (const auto &sample : *block) writeSample(stream, sample);
            }
        } else if (const auto tileWrite = std::get_if<TileWriteSnapshot>(&value)) {
            const auto &write = **tileWrite;
            stream << quint32(write.tiles.size());
            if (!withinLimit() || write.tiles.size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 4) return {};
            for (const auto &tile : write.tiles) stream << tileIndices.value(tile.data());
            stream << write.writeGroup << quint32(write.writtenAreas.size());
            if (!withinLimit() || write.writtenAreas.size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 16) return {};
            for (const auto &area : write.writtenAreas)
                stream << qint32(area.x()) << qint32(area.y()) << qint32(area.width()) << qint32(area.height());
        } else if (const auto crop = std::get_if<CropSnapshot>(&value)) {
            const auto &rect = (*crop)->rect;
            stream << qint32(rect.x()) << qint32(rect.y()) << qint32(rect.width()) << qint32(rect.height());
        } else {
            const auto &translation = *std::get<TranslationSnapshot>(value);
            stream << qint32(translation.offset.x()) << qint32(translation.offset.y());
        }
        if (!withinLimit()) return {};
    }
    if (m_layerHistoryChanges.size() != m_history.size()) return {};
    stream << quint32(m_history.size()) << quint32(m_index);
    for (qsizetype stateIndex = 0; stateIndex < m_history.size(); ++stateIndex) {
        const auto &state = m_history[stateIndex];
        stream << qint32(state->size.width()) << qint32(state->size.height());
        for (const auto component : state->projectionBackground) stream << component;
        const bool explicitSelection = state->selectedLayers.size() > 1 ||
            (!state->selectedLayers.isEmpty() && state->selectedLayers.first() != state->activeLayer);
        stream << state->revision() << state->m_modifiedRevision << state->activeLayer
               << (quint32(state->layers.size()) | (explicitSelection ? 0x80000000u : 0u));
        if (explicitSelection) {
            stream << quint32(state->selectedLayers.size());
            for (const auto layer : state->selectedLayers) stream << layer;
        }
        if (!withinLimit()) return {};
        for (const auto &layer : state->layers) {
            const auto name = layer.name.toUtf8();
            if (name.size() > m_limits.recoveryBytes - DigestSize - bytes.size() - 69) return {};
            stream << layer.id << quint32(name.size());
            stream.writeRawData(name.constData(), int(name.size()));
            stream << quint8(layer.visible) << layer.uuid;
            for (const auto component : layer.defaultPixel) stream << component;
            stream << layer.opacity << quint8(layer.userLocked) << quint8(quint8(layer.pinnedToTimeline) | ((15u ^ layer.channelWriteMask) << 1));
            stream << qint32(layer.position.x()) << qint32(layer.position.y());
            stream << quint8(layer.kind) << layer.parentUuid;
            stream << qint32(layer.colorLabelIndex);
            stream << quint32(layer.operations.size());
            if (!withinLimit() || layer.operations.size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 4) return {};
            for (const auto &operation : layer.operations) stream << indices.value(operationPointer(operation));
        }
        const auto &change = m_layerHistoryChanges[stateIndex];
        stream << quint8(quint8(change.property) | (change.duplicatedLayers ? 128u : 0u) |
                         (change.clearedLayers ? 64u : 0u) | (change.filledLayer ? 32u : 0u) |
                         (change.crop ? 16u : 0u) | (change.canvasResize ? 8u : 0u));
        if (change.canvasResize) {
            const auto &resize = *change.canvasResize;
            stream << qint32(resize.rect.x()) << qint32(resize.rect.y())
                   << qint32(resize.rect.width()) << qint32(resize.rect.height()) << quint8(resize.cropLayers);
        }
        if (change.crop) {
            if (!indices.contains(change.crop.data())) return {};
            stream << indices.value(change.crop.data());
        }
        if (change.property != LayerProperty::None) {
            stream << change.layer;
            if (change.property == LayerProperty::Name) {
                for (const auto &value : {change.before, change.after}) {
                    const auto name = value.toString().toUtf8();
                    if (name.size() > m_limits.recoveryBytes - DigestSize - bytes.size() - 4) return {};
                    stream << quint32(name.size());
                    stream.writeRawData(name.constData(), int(name.size()));
                }
            } else if (change.property == LayerProperty::Position) {
                for (const auto &value : {change.before, change.after})
                    stream << qint32(value.toPoint().x()) << qint32(value.toPoint().y());
            } else if (change.property == LayerProperty::Opacity || change.property == LayerProperty::ChannelMask) {
                stream << quint8(change.before.toUInt()) << quint8(change.after.toUInt());
            } else {
                stream << quint8(change.before.toBool()) << quint8(change.after.toBool());
            }
        }
        if (!withinLimit()) return {};
    }
    stream << (m_edit ? indices.value(m_edit.data()) : m_translation ? indices.value(m_translation.data()) : NoOperation)
           << (m_preview ? m_preview->revision() : quint64(0));
    if (!withinLimit()) return {};
    bytes.append(digest(bytes, bytes.size()));
    return bytes;
}

Result OperationDocument::restoreRecovery(const QByteArray &data, const QHash<QUuid, int> &legacyColorLabels)
{
    if (m_closed) return Result::Closed;
    if (data.size() > m_limits.recoveryBytes) return Result::CapacityExceeded;
    if (data.size() <= DigestSize || data.right(DigestSize) != digest(data, data.size() - DigestSize)) return Result::InvalidRecovery;
    const auto payload = QByteArray::fromRawData(data.constData(), data.size() - DigestSize);
    QDataStream stream(payload);
    configure(stream);
    quint32 magic = 0, tileCount = 0;
    quint16 format = 0;
    qint32 width = 0, height = 0;
    quint64 nextRevision = 0, nextOperation = 0, nextLayer = 0, savedRevision = 0;
    stream >> magic >> format >> width >> height >> nextRevision >> nextOperation >> nextLayer >> savedRevision >> tileCount;
    if (magic != Magic || format < 9 || format > FormatVersion || width <= 0 || height <= 0 ||
        width > CoordinateLimit || height > CoordinateLimit || savedRevision > nextRevision ||
        !readableCount(stream, tileCount, payload.size(), 12)) return Result::InvalidRecovery;
    const quint64 retainedTileLimit = quint64(m_limits.rasterBytes / RgbaTile::ByteCount) * (HistoryLimit + 1);
    if (tileCount > retainedTileLimit) return Result::CapacityExceeded;
    QList<QSharedPointer<const RasterTile>> tiles;
    for (quint32 i = 0; i < tileCount; ++i) {
        qint32 x = 0, y = 0;
        quint32 size = 0;
        stream >> x >> y >> size;
        if (!readableCount(stream, size, RasterTile::maximumCompressedBytes(), 1)) return Result::InvalidRecovery;
        QByteArray compressed(size, Qt::Uninitialized);
        if (stream.readRawData(compressed.data(), size) != int(size)) return Result::InvalidRecovery;
        const auto tile = RasterTile::fromCompressed({x, y}, compressed);
        if (!tile) return Result::InvalidRecovery;
        tiles.append(tile);
    }
    quint32 operationCount = 0;
    stream >> operationCount;
    if (!readableCount(stream, operationCount, payload.size(), 29)) return Result::InvalidRecovery;
    const QUuid identity = QUuid::createUuid();
    QList<LayerOperation> operations;
    QHash<quint64, quint32> operationIndices;
    QSet<quint32> referencedTiles;
    qsizetype retainedSamples = 0;
    const quint64 payloadSamples = quint64(payload.size() / (format >= 21 ? 64 : 56));
    const quint64 retainedSampleLimit = qMin(quint64(m_limits.samplesPerDocument), payloadSamples) * (HistoryLimit + 1) +
                                        qMin(quint64(m_limits.samplesPerStroke), payloadSamples);
    for (quint32 i = 0; i < operationCount; ++i) {
        quint8 kind = 0;
        quint64 sequence = 0, version = 0;
        LayerId layer = 0;
        stream >> kind >> sequence >> version >> layer;
        if (kind > (format >= 26 ? 3 : format >= 25 ? 2 : 1) || sequence == 0 || sequence > nextOperation || operationIndices.contains(sequence) ||
            layer == 0 || layer > nextLayer || version > nextRevision) return Result::InvalidRecovery;
        operationIndices.insert(sequence, quint32(operations.size()));
        if (kind == 3) {
            qint32 x = 0, y = 0, width = 0, height = 0;
            stream >> x >> y >> width >> height;
            if (version != 1 || stream.status() != QDataStream::Ok || !validCropRect(x, y, width, height)) return Result::InvalidRecovery;
            auto operation = QSharedPointer<CropOperation>::create();
            operation->id = {identity, sequence};
            operation->version = version;
            operation->layer = layer;
            operation->rect = QRect(x, y, width, height);
            operations.append(CropSnapshot(operation));
            continue;
        }
        if (kind == 2) {
            auto operation = QSharedPointer<TranslationOperation>::create();
            operation->id = {identity, sequence};
            operation->version = version;
            operation->layer = layer;
            qint32 x = 0, y = 0;
            stream >> x >> y;
            constexpr qint64 limit = 2 * RgbaTile::CoordinateLimit;
            if (stream.status() != QDataStream::Ok || std::abs(qint64(x)) > limit || std::abs(qint64(y)) > limit ||
                (version == 0 && (x || y))) return Result::InvalidRecovery;
            operation->offset = {x, y};
            operations.append(TranslationSnapshot(operation));
            continue;
        }
        if (kind == 1) {
            auto operation = QSharedPointer<TileWriteOperation>::create();
            operation->id = {identity, sequence};
            operation->version = version;
            operation->layer = layer;
            quint32 count = 0;
            stream >> count;
            if (version > 1 || count == 0 || !readableCount(stream, count, tileCount, 4)) return Result::InvalidRecovery;
            QSet<QPoint> coordinates;
            for (quint32 j = 0; j < count; ++j) {
                quint32 index = 0;
                stream >> index;
                if (index >= tileCount || coordinates.contains(tiles[index]->coordinate())) return Result::InvalidRecovery;
                operation->tiles.append(tiles[index]);
                coordinates.insert(tiles[index]->coordinate());
                referencedTiles.insert(index);
            }
            quint32 areaCount = 0;
            stream >> operation->writeGroup >> areaCount;
            const auto maximumAreas = format < 28 ? 16 : PixelWriteAreasPerTile;
            if (!readableCount(stream, areaCount, quint64(count) * maximumAreas, 16)) return Result::InvalidRecovery;
            for (quint32 j = 0; j < areaCount; ++j) {
                qint32 x = 0, y = 0, width = 0, height = 0;
                stream >> x >> y >> width >> height;
                if (stream.status() != QDataStream::Ok || !validPixelWriteArea(x, y, width, height, format < 28)) return Result::InvalidRecovery;
                operation->writtenAreas.append({x, y, width, height});
            }
            if (!validWrittenAreas(*operation, format < 28)) return Result::InvalidRecovery;
            operations.append(TileWriteSnapshot(operation));
            continue;
        }
        auto operation = QSharedPointer<PaintOperation>::create();
        operation->id = {identity, sequence};
        operation->version = version;
        operation->layer = layer;
        for (auto &component : operation->brush.rgba) stream >> component;
        quint8 brushFlags = 0, shape = 0;
        qreal x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        quint32 sampleCount = 0;
        stream >> operation->brush.diameter >> operation->brush.spacing >> operation->brush.opacity >> brushFlags;
        if (format >= 14 && (brushFlags & 192)) {
            qint32 x = 0, y = 0;
            stream >> x >> y;
            operation->brush.mirrorCenter = {x, y};
            operation->brush.mirrorHorizontal = brushFlags & 64;
            operation->brush.mirrorVertical = brushFlags & 128;
        }
        stream >> operation->randomSeed >> shape >> x1 >> y1 >> x2 >> y2 >> operation->shape.rotation;
        if (shape >= quint8(ShapeKind::RoundedRectangle) && format < 11) return Result::InvalidRecovery;
        if (shape >= quint8(ShapeKind::Path) && format < 18) return Result::InvalidRecovery;
        if (static_cast<ShapeKind>(shape) == ShapeKind::RoundedRectangle) {
            qreal radiusX = 0, radiusY = 0;
            stream >> radiusX >> radiusY;
            operation->shape.cornerRadii = {radiusX, radiusY};
        }
        if (static_cast<ShapeKind>(shape) == ShapeKind::Path) {
            quint8 winding = 0;
            quint32 commandCount = 0;
            stream >> winding >> commandCount;
            if (winding > 1 || !readableCount(stream, commandCount, m_limits.samplesPerStroke, 1)) return Result::InvalidRecovery;
            operation->shape.windingFill = winding;
            operation->shape.pathCommands.reserve(commandCount);
            for (quint32 i = 0; i < commandCount; ++i) {
                quint8 command = 0;
                stream >> command;
                operation->shape.pathCommands.append(static_cast<PathCommand>(command));
            }
        }
        if (format >= 17) {
            quint8 figureFlags = 0;
            stream >> figureFlags;
            if (figureFlags > 3) return Result::InvalidRecovery;
            operation->figureStyle.outline = figureFlags & 1;
            if (figureFlags & 2) {
                std::array<quint8, 4> fill;
                for (auto &component : fill) stream >> component;
                operation->figureStyle.fill = fill;
            }
        }
        if (format >= 19) {
            quint8 processing = 0;
            stream >> processing;
            if (processing > (format >= 21 ? 15 : format >= 20 ? 7 : 1) ||
                ((processing & 12) && !(processing & 2))) return Result::InvalidRecovery;
            operation->brush.pixelPerfect = processing & 1;
            operation->brush.basicSmoothing = processing & 2;
            operation->smoothingFinished = processing & 4;
            if (processing & 8) {
                WeightedSmoothing value;
                quint8 pressure = 0;
                stream >> value.distanceMin >> value.distanceMax >> value.tailAggressiveness >> pressure;
                if (pressure > 1) return Result::InvalidRecovery;
                value.smoothPressure = pressure;
                operation->brush.weightedSmoothing = value;
            }
            if (operation->brush.basicSmoothing) {
                quint32 flushes = 0;
                stream >> flushes;
                if (!readableCount(stream, flushes, m_limits.samplesPerStroke, 4)) return Result::InvalidRecovery;
                operation->smoothingFlushes.reserve(flushes);
                qsizetype previous = 1;
                for (quint32 index = 0; index < flushes; ++index) {
                    quint32 count = 0;
                    stream >> count;
                    if (count <= quint64(previous)) return Result::InvalidRecovery;
                    operation->smoothingFlushes.append(count);
                    previous = count;
                }
            }
        }
        stream >> sampleCount;
        operation->brush.erase = brushFlags & 1;
        operation->brush.alphaLocked = brushFlags & 2;
        operation->brush.channelWriteMask = format >= 13 ? quint8(15u ^ ((brushFlags >> 2) & 15u)) : quint8(15);
        operation->shape.kind = static_cast<ShapeKind>(shape);
        operation->shape.first = {x1, y1};
        operation->shape.second = {x2, y2};
        if (brushFlags > (format < 12 ? 1 : format < 13 ? 3 : format < 14 ? 63 : 255) || !validBrush(operation->brush) || !validShape(operation->shape) ||
            !validFigureStyle(operation->shape, operation->figureStyle) ||
            (operation->brush.basicSmoothing && (operation->shape.kind != ShapeKind::Freehand ||
             (!operation->smoothingFlushes.isEmpty() && operation->smoothingFlushes.last() > sampleCount))) ||
            !validShapeSampleCount(operation->shape, sampleCount) ||
            (version == 0 && (sampleCount != 0 || operation->shape.kind != ShapeKind::Freehand)) ||
            (version > 0 && sampleCount == 0 && operation->shape.kind != ShapeKind::Path) ||
            !readableCount(stream, sampleCount, m_limits.samplesPerStroke, format >= 21 ? 64 : 56)) return Result::InvalidRecovery;
        if (quint64(retainedSamples) + sampleCount > retainedSampleLimit) return Result::CapacityExceeded;
        retainedSamples += sampleCount;
        auto samples = QSharedPointer<QList<InputSample>>::create();
        samples->reserve(sampleCount);
        quint64 time = 0;
        for (quint32 j = 0; j < sampleCount; ++j) {
            const auto sample = readSample(stream, format);
            if (!validSample(sample) || sample.timestamp < time) return Result::InvalidRecovery;
            time = sample.timestamp;
            samples->append(sample);
        }
        if (sampleCount) operation->sampleBlocks.append(samples);
        operation->sampleCount = sampleCount;
        operations.append(PaintSnapshot(operation));
    }
    for (const auto &value : operations) {
        const auto write = std::get_if<TileWriteSnapshot>(&value);
        if (!write || !(*write)->writeGroup) continue;
        const auto root = operationIndices.constFind((*write)->writeGroup);
        if (root == operationIndices.cend()) continue;
        const auto first = std::get_if<TileWriteSnapshot>(&operations[*root]);
        if (!first || (*first)->layer != (*write)->layer || (*first)->writeGroup != (*write)->writeGroup)
            return Result::InvalidRecovery;
    }
    if (referencedTiles.size() != tiles.size()) return Result::InvalidRecovery;
    quint32 stateCount = 0, currentIndex = 0;
    stream >> stateCount >> currentIndex;
    if (stateCount == 0 || currentIndex >= stateCount || !readableCount(stream, stateCount, HistoryLimit + 1, 33)) return Result::InvalidRecovery;
    QList<Snapshot> history;
    QList<LayerHistoryChange> changes;
    QSet<quint32> referencedOperations;
    QHash<LayerId, QUuid> layerIdentities;
    QHash<LayerId, LayerKind> layerKinds;
    QHash<QUuid, LayerId> identityOwners;
    quint64 previousRevision = 0;
    quint64 previousModifiedRevision = 0;
    QSet<quint64> stateRevisions;
    for (quint32 i = 0; i < stateCount; ++i) {
        QSharedPointer<DocumentVersion> state(new DocumentVersion);
        state->m_document = identity;
        state->m_committed = true;
        state->size = {width, height};
        if (format >= 27) {
            qint32 stateWidth = 0, stateHeight = 0;
            stream >> stateWidth >> stateHeight;
            if (stateWidth <= 0 || stateHeight <= 0 || stateWidth > CoordinateLimit || stateHeight > CoordinateLimit)
                return Result::InvalidRecovery;
            state->size = {stateWidth, stateHeight};
        }
        quint32 layerCount = 0;
        for (auto &component : state->projectionBackground) stream >> component;
        stream >> state->m_revision >> state->m_modifiedRevision >> state->activeLayer >> layerCount;
        const bool explicitSelection = format >= 15 && (layerCount & 0x80000000u);
        if (explicitSelection) {
            layerCount &= 0x7fffffffu;
            quint32 selectedCount = 0;
            stream >> selectedCount;
            if (selectedCount < (format >= 16 ? 1u : 2u) ||
                !readableCount(stream, selectedCount, qMin<quint64>(layerCount, m_limits.layerCount), 8)) return Result::InvalidRecovery;
            for (quint32 selected = 0; selected < selectedCount; ++selected) {
                LayerId layer = 0;
                stream >> layer;
                state->selectedLayers.append(layer);
            }
        } else if (state->activeLayer) state->selectedLayers = {state->activeLayer};
        if (state->revision() > nextRevision || state->m_modifiedRevision > state->revision() ||
            stateRevisions.contains(state->revision()) ||
            (i > 0 && state->m_modifiedRevision <= previousModifiedRevision) ||
            !readableCount(stream, layerCount, m_limits.layerCount, format >= 30 ? 69 : format >= 29 ? 65 : format >= 24 ? 48 : 40)) return Result::InvalidRecovery;
        stateRevisions.insert(state->revision());
        previousRevision = qMax(previousRevision, state->revision());
        previousModifiedRevision = state->m_modifiedRevision;
        QSet<LayerId> layers;
        for (quint32 j = 0; j < layerCount; ++j) {
            Layer layer;
            quint32 nameSize = 0, count = 0;
            quint8 visible = 0, locked = 0, pinned = 0;
            stream >> layer.id >> nameSize;
            if (layer.id == 0 || layer.id > nextLayer || layers.contains(layer.id) ||
                !readableCount(stream, nameSize, LayerNameByteLimit, 1)) return Result::InvalidRecovery;
            QByteArray name(nameSize, Qt::Uninitialized);
            if (stream.readRawData(name.data(), nameSize) != int(nameSize)) return Result::InvalidRecovery;
            layer.name = QString::fromUtf8(name);
            if (layer.name.toUtf8() != name) return Result::InvalidRecovery;
            stream >> visible >> layer.uuid;
            for (auto &component : layer.defaultPixel) stream >> component;
            stream >> layer.opacity >> locked >> pinned;
            if (format >= 24) {
                qint32 x = 0, y = 0;
                stream >> x >> y;
                layer.position = {x, y};
            }
            if (format >= 29) {
                quint8 kind = 0;
                stream >> kind >> layer.parentUuid;
                if (kind > quint8(LayerKind::Group)) return Result::InvalidRecovery;
                layer.kind = LayerKind(kind);
            }
            if (format >= 30) {
                qint32 label = 0;
                stream >> label;
                layer.colorLabelIndex = label;
            } else layer.colorLabelIndex = legacyColorLabels.value(layer.uuid);
            stream >> count;
            if (visible > 1 || locked > 1 || pinned > (format < 13 ? 1 : 31) || layer.uuid.isNull() ||
                (layerIdentities.contains(layer.id) && layerIdentities.value(layer.id) != layer.uuid) ||
                (layerKinds.contains(layer.id) && layerKinds.value(layer.id) != layer.kind) ||
                (identityOwners.contains(layer.uuid) && identityOwners.value(layer.uuid) != layer.id) ||
                !readableCount(stream, count, operationCount, 4)) return Result::InvalidRecovery;
            layerIdentities.insert(layer.id, layer.uuid);
            layerKinds.insert(layer.id, layer.kind);
            identityOwners.insert(layer.uuid, layer.id);
            layer.visible = visible;
            layer.userLocked = locked;
            layer.pinnedToTimeline = pinned & 1;
            layer.channelWriteMask = format >= 13 ? quint8(15u ^ (pinned >> 1)) : quint8(15);
            quint64 previousOperation = 0;
            for (quint32 k = 0; k < count; ++k) {
                quint32 index = NoOperation;
                stream >> index;
                if (index >= operationCount) return Result::InvalidRecovery;
                const auto &operation = operations[index];
                const auto samples = operationSamples(operation);
                const auto pixels = operationRasterBytes(operation);
                const auto paint = std::get_if<PaintSnapshot>(&operation);
                const auto translation = std::get_if<TranslationSnapshot>(&operation);
                if ((paint && (*paint)->brush.basicSmoothing && !(*paint)->smoothingFinished) ||
                    (translation && (*translation)->version == 0) ||
                    operationLayer(operation) != layer.id || (paint && samples == 0 && (*paint)->shape.kind != ShapeKind::Path) ||
                    operationId(operation).sequence <= previousOperation) return Result::InvalidRecovery;
                if (samples > m_limits.samplesPerDocument - state->sampleCount || pixels > m_limits.rasterBytes - state->rasterBytes) return Result::CapacityExceeded;
                previousOperation = operationId(operation).sequence;
                state->sampleCount += samples;
                state->rasterBytes += pixels;
                layer.operations.append(operation);
                referencedOperations.insert(index);
            }
            if (layer.kind == LayerKind::Group && (!layer.operations.isEmpty() ||
                layer.defaultPixel != std::array<quint8, 4>{} || layer.channelWriteMask != 15)) return Result::InvalidRecovery;
            layers.insert(layer.id);
            state->layers.append(layer);
        }
        if (!validLayerHierarchy(state->layers) ||
            (layers.isEmpty() ? state->activeLayer != 0 : !layers.contains(state->activeLayer))) return Result::InvalidRecovery;
        QList<LayerId> selected;
        for (const auto &layer : state->layers) if (state->selectedLayers.contains(layer.id)) selected.append(layer.id);
        if (selected != state->selectedLayers || (!layers.isEmpty() && selected.isEmpty()) ||
            (format < 16 && !layers.isEmpty() && !selected.contains(state->activeLayer)))
            return Result::InvalidRecovery;
        LayerHistoryChange change;
        quint8 property = 0;
        stream >> property;
        if (format >= 27 && (property & 8u)) {
            property &= 247u;
            qint32 x = 0, y = 0, w = 0, h = 0;
            quint8 crop = 0;
            stream >> x >> y >> w >> h >> crop;
            if (i == 0 || property != quint8(LayerProperty::None) || crop > 1 ||
                !validCropRect(x, y, w, h) || !validCanvasRect(QRect(x, y, w, h)) ||
                state->size != QSize(w, h) || state->layers.size() != history.last()->layers.size())
                return Result::InvalidRecovery;
            for (qsizetype layer = 0; layer < state->layers.size(); ++layer)
                if (state->layers[layer].id != history.last()->layers[layer].id) return Result::InvalidRecovery;
            change.canvasResize = CanvasResize{QRect(x, y, w, h), bool(crop)};
        }
        if (format >= 26 && (property & 16u)) {
            property &= 239u;
            quint32 cropIndex = NoOperation;
            stream >> cropIndex;
            if (i == 0 || property != quint8(LayerProperty::None) || cropIndex >= operationCount ||
                !std::holds_alternative<CropSnapshot>(operations[cropIndex]) ||
                state->layers.size() != history.last()->layers.size()) return Result::InvalidRecovery;
            change.crop = std::get<CropSnapshot>(operations[cropIndex]);
            bool found = false;
            for (qsizetype layer = 0; layer < state->layers.size(); ++layer) {
                if (state->layers[layer].id != history.last()->layers[layer].id) return Result::InvalidRecovery;
                if (state->layers[layer].id != change.crop->layer) continue;
                const LayerOperation crop(change.crop);
                if (!state->layers[layer].operations.contains(crop) || history.last()->layers[layer].operations.contains(crop))
                    return Result::InvalidRecovery;
                found = true;
            }
            if (!found) return Result::InvalidRecovery;
        }
        change.filledLayer = format >= 23 && (property & 32u);
        if (change.filledLayer) {
            property &= 223u;
            if (i == 0 || property != quint8(LayerProperty::None) ||
                state->layers.size() != history.last()->layers.size()) return Result::InvalidRecovery;
            for (qsizetype layer = 0; layer < state->layers.size(); ++layer)
                if (state->layers[layer].id != history.last()->layers[layer].id) return Result::InvalidRecovery;
        }
        change.clearedLayers = format >= 22 && (property & 64u);
        if (change.clearedLayers) {
            property &= 191u;
            if (i == 0 || property != quint8(LayerProperty::None) ||
                state->layers.size() != history.last()->layers.size()) return Result::InvalidRecovery;
            for (qsizetype layer = 0; layer < state->layers.size(); ++layer)
                if (state->layers[layer].id != history.last()->layers[layer].id) return Result::InvalidRecovery;
        }
        change.duplicatedLayers = format >= 15 && (property & 128u);
        if (change.duplicatedLayers) {
            property &= 127u;
            if (i == 0 || property != quint8(LayerProperty::None) || state->layers.size() <= history.last()->layers.size())
                return Result::InvalidRecovery;
        }
        if (property > quint8(format >= 25 ? LayerProperty::Position :
                              format < 13 ? LayerProperty::PinnedToTimeline : LayerProperty::ChannelMask)) return Result::InvalidRecovery;
        change.property = LayerProperty(property);
        if (change.property != LayerProperty::None) {
            stream >> change.layer;
            if (i == 0 || !layers.contains(change.layer)) return Result::InvalidRecovery;
            bool existed = false;
            for (const auto &layer : history.last()->layers) existed |= layer.id == change.layer;
            if (!existed) return Result::InvalidRecovery;
            if (change.property == LayerProperty::Name) {
                for (auto *value : {&change.before, &change.after}) {
                    quint32 count = 0;
                    stream >> count;
                    if (!readableCount(stream, count, LayerNameByteLimit, 1)) return Result::InvalidRecovery;
                    QByteArray name(count, Qt::Uninitialized);
                    if (stream.readRawData(name.data(), int(count)) != int(count)) return Result::InvalidRecovery;
                    const auto decoded = QString::fromUtf8(name);
                    if (decoded.toUtf8() != name || !validLayerName(decoded)) return Result::InvalidRecovery;
                    *value = decoded;
                }
            } else if (change.property == LayerProperty::Position) {
                for (auto *value : {&change.before, &change.after}) {
                    qint32 x = 0, y = 0;
                    stream >> x >> y;
                    *value = QPoint(x, y);
                }
            } else {
                quint8 before = 0, after = 0;
                stream >> before >> after;
                if (change.property == LayerProperty::Opacity || change.property == LayerProperty::ChannelMask) {
                    if (change.property == LayerProperty::ChannelMask && (before > 15 || after > 15)) return Result::InvalidRecovery;
                    change.before = QVariant::fromValue(before);
                    change.after = QVariant::fromValue(after);
                } else {
                    if (before > 1 || after > 1) return Result::InvalidRecovery;
                    change.before = bool(before);
                    change.after = bool(after);
                }
            }
            if (change.before == change.after && (format < 10 || change.property != LayerProperty::Name))
                return Result::InvalidRecovery;
        }
        if (i > 0 && state->size != history.last()->size && !change.canvasResize) return Result::InvalidRecovery;
        changes.append(change);
        history.append(state);
    }
    quint32 pendingIndex = NoOperation;
    quint64 previewRevision = 0;
    stream >> pendingIndex >> previewRevision;
    if (stream.status() != QDataStream::Ok || !stream.atEnd()) return Result::InvalidRecovery;
    PaintSnapshot edit;
    TranslationSnapshot translation;
    Snapshot preview;
    if (pendingIndex != NoOperation) {
        if (pendingIndex >= operationCount || referencedOperations.contains(pendingIndex)) return Result::InvalidRecovery;
        const auto paint = std::get_if<PaintSnapshot>(&operations[pendingIndex]);
        if (paint) {
            edit = *paint;
            if (edit->smoothingFinished) return Result::InvalidRecovery;
            const auto &base = history[currentIndex];
            qsizetype index = -1;
            for (qsizetype i = 0; i < base->layers.size(); ++i) {
                if (base->layers[i].id == edit->layer) index = i;
            }
            if (index < 0 || base->layers[index].kind != LayerKind::Paint ||
                !base->layers[index].visible || base->layers[index].userLocked ||
                edit->sampleCount > m_limits.samplesPerDocument - base->sampleCount) return Result::InvalidRecovery;
            if (edit->sampleCount > 0 || edit->shape.kind == ShapeKind::Path) {
                if (previewRevision <= previousRevision || previewRevision > nextRevision || previewRevision == savedRevision) return Result::InvalidRecovery;
                QSharedPointer<DocumentVersion> value(new DocumentVersion(*base));
                value->m_committed = false;
                value->m_pendingOperation = edit->id;
                value->m_modifiedRevision = value->m_revision = previewRevision;
                value->layers[index].operations.append(edit);
                value->sampleCount += edit->sampleCount;
                preview = value;
            } else if (previewRevision != 0 || edit->shape.kind != ShapeKind::Freehand) return Result::InvalidRecovery;
        } else if (const auto value = std::get_if<TranslationSnapshot>(&operations[pendingIndex])) {
            translation = *value;
            const auto &base = history[currentIndex];
            qsizetype index = -1;
            for (qsizetype i = 0; i < base->layers.size(); ++i)
                if (base->layers[i].id == translation->layer) index = i;
            if (index < 0 || base->layers[index].kind != LayerKind::Paint ||
                !base->layers[index].visible || base->layers[index].userLocked) return Result::InvalidRecovery;
            if (previewRevision) {
                if (translation->version == 0 || previewRevision <= previousRevision || previewRevision > nextRevision ||
                    previewRevision == savedRevision) return Result::InvalidRecovery;
                auto value = QSharedPointer<DocumentVersion>(new DocumentVersion(*base));
                auto &layer = value->layers[index];
                const auto previous = layer.operations.isEmpty() ? nullptr :
                    std::get_if<TranslationSnapshot>(&layer.operations.last());
                const QPoint prior = previous ? (*previous)->offset : QPoint();
                const qint64 dx = qint64(translation->offset.x()) - prior.x();
                const qint64 dy = qint64(translation->offset.y()) - prior.y();
                const qint64 x = qint64(layer.position.x()) + dx, y = qint64(layer.position.y()) + dy;
                constexpr qint64 limit = 2 * RgbaTile::CoordinateLimit;
                if (std::abs(dx) > limit || std::abs(dy) > limit ||
                    x < std::numeric_limits<qint32>::min() || x > std::numeric_limits<qint32>::max() ||
                    y < std::numeric_limits<qint32>::min() || y > std::numeric_limits<qint32>::max()) return Result::InvalidRecovery;
                if (previous) layer.operations.removeLast();
                layer.operations.append(translation);
                layer.position = {int(x), int(y)};
                value->m_committed = false;
                value->m_pendingOperation = translation->id;
                value->m_modifiedRevision = value->m_revision = previewRevision;
                preview = value;
            } else if (translation->version != 0) return Result::InvalidRecovery;
        } else return Result::InvalidRecovery;
        referencedOperations.insert(pendingIndex);
    } else if (previewRevision != 0) return Result::InvalidRecovery;
    if (referencedOperations.size() != operations.size()) return Result::InvalidRecovery;
    if (history[currentIndex]->size != QSize(width, height)) return Result::InvalidRecovery;
    m_identity = identity;
    m_history = history;
    m_layerHistoryChanges = changes;
    m_index = currentIndex;
    m_nextRevision = nextRevision;
    m_nextOperation = nextOperation;
    m_nextLayer = nextLayer;
    m_savedRevision = savedRevision;
    m_edit = edit;
    m_translation = translation;
    m_preview = preview;
    return Result::Success;
}
}
