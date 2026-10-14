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
constexpr quint16 FormatVersion = 9;
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
           << sample.xTilt << sample.yTilt << sample.rotation << sample.timestamp;
}
InputSample readSample(QDataStream &stream)
{
    InputSample sample;
    qreal x = 0, y = 0;
    stream >> x >> y >> sample.pressure >> sample.xTilt >> sample.yTilt
           >> sample.rotation >> sample.timestamp;
    sample.position = {x, y};
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
        stream << quint8(paint ? 0 : 1) << operationId(value).sequence << operationVersion(value) << operationLayer(value);
        if (paint) {
            const auto &operation = **paint;
            for (const auto component : operation.brush.rgba) stream << component;
            stream << operation.brush.diameter << operation.brush.spacing << operation.brush.opacity
                   << quint8(operation.brush.erase) << operation.randomSeed << quint8(operation.shape.kind)
                   << operation.shape.first.x() << operation.shape.first.y()
                   << operation.shape.second.x() << operation.shape.second.y() << operation.shape.rotation
                   << quint32(operation.sampleCount);
            if (!withinLimit()) return {};
            for (const auto &block : operation.sampleBlocks) {
                if (block->size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 56) return {};
                for (const auto &sample : *block) writeSample(stream, sample);
            }
        } else {
            const auto &write = *std::get<TileWriteSnapshot>(value);
            stream << quint32(write.tiles.size());
            if (!withinLimit() || write.tiles.size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 4) return {};
            for (const auto &tile : write.tiles) stream << tileIndices.value(tile.data());
            stream << write.writeGroup << quint32(write.writtenAreas.size());
            if (!withinLimit() || write.writtenAreas.size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 16) return {};
            for (const auto &area : write.writtenAreas)
                stream << qint32(area.x()) << qint32(area.y()) << qint32(area.width()) << qint32(area.height());
        }
        if (!withinLimit()) return {};
    }
    if (m_propertyChanges.size() != m_history.size()) return {};
    stream << quint32(m_history.size()) << quint32(m_index);
    for (qsizetype stateIndex = 0; stateIndex < m_history.size(); ++stateIndex) {
        const auto &state = m_history[stateIndex];
        for (const auto component : state->projectionBackground) stream << component;
        stream << state->revision() << state->m_modifiedRevision << state->activeLayer << quint32(state->layers.size());
        if (!withinLimit()) return {};
        for (const auto &layer : state->layers) {
            const auto name = layer.name.toUtf8();
            if (name.size() > m_limits.recoveryBytes - DigestSize - bytes.size() - 40) return {};
            stream << layer.id << quint32(name.size());
            stream.writeRawData(name.constData(), int(name.size()));
            stream << quint8(layer.visible) << layer.uuid;
            for (const auto component : layer.defaultPixel) stream << component;
            stream << layer.opacity << quint8(layer.userLocked) << quint8(layer.pinnedToTimeline);
            stream << quint32(layer.operations.size());
            if (!withinLimit() || layer.operations.size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 4) return {};
            for (const auto &operation : layer.operations) stream << indices.value(operationPointer(operation));
        }
        const auto &change = m_propertyChanges[stateIndex];
        stream << quint8(change.property);
        if (change.property != LayerProperty::None) {
            stream << change.layer;
            if (change.property == LayerProperty::Name) {
                for (const auto &value : {change.before, change.after}) {
                    const auto name = value.toString().toUtf8();
                    if (name.size() > m_limits.recoveryBytes - DigestSize - bytes.size() - 4) return {};
                    stream << quint32(name.size());
                    stream.writeRawData(name.constData(), int(name.size()));
                }
            } else if (change.property == LayerProperty::Opacity) {
                stream << quint8(change.before.toUInt()) << quint8(change.after.toUInt());
            } else {
                stream << quint8(change.before.toBool()) << quint8(change.after.toBool());
            }
        }
        if (!withinLimit()) return {};
    }
    stream << (m_edit ? indices.value(m_edit.data()) : NoOperation)
           << (m_preview ? m_preview->revision() : quint64(0));
    if (!withinLimit()) return {};
    bytes.append(digest(bytes, bytes.size()));
    return bytes;
}

Result OperationDocument::restoreRecovery(const QByteArray &data)
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
    if (magic != Magic || format != FormatVersion || width <= 0 || height <= 0 ||
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
    const quint64 payloadSamples = quint64(payload.size() / 56);
    const quint64 retainedSampleLimit = qMin(quint64(m_limits.samplesPerDocument), payloadSamples) * (HistoryLimit + 1) +
                                        qMin(quint64(m_limits.samplesPerStroke), payloadSamples);
    for (quint32 i = 0; i < operationCount; ++i) {
        quint8 kind = 0;
        quint64 sequence = 0, version = 0;
        LayerId layer = 0;
        stream >> kind >> sequence >> version >> layer;
        if (kind > 1 || sequence == 0 || sequence > nextOperation || operationIndices.contains(sequence) ||
            layer == 0 || layer > nextLayer || version > nextRevision) return Result::InvalidRecovery;
        operationIndices.insert(sequence, quint32(operations.size()));
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
            if (!readableCount(stream, areaCount, quint64(count) * 16, 16)) return Result::InvalidRecovery;
            for (quint32 j = 0; j < areaCount; ++j) {
                qint32 x = 0, y = 0, width = 0, height = 0;
                stream >> x >> y >> width >> height;
                if (stream.status() != QDataStream::Ok || !validPixelWriteArea(x, y, width, height)) return Result::InvalidRecovery;
                operation->writtenAreas.append({x, y, width, height});
            }
            if (!validWrittenAreas(*operation)) return Result::InvalidRecovery;
            operations.append(TileWriteSnapshot(operation));
            continue;
        }
        auto operation = QSharedPointer<PaintOperation>::create();
        operation->id = {identity, sequence};
        operation->version = version;
        operation->layer = layer;
        for (auto &component : operation->brush.rgba) stream >> component;
        quint8 erase = 0, shape = 0;
        qreal x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        quint32 sampleCount = 0;
        stream >> operation->brush.diameter >> operation->brush.spacing >> operation->brush.opacity >> erase
               >> operation->randomSeed >> shape >> x1 >> y1 >> x2 >> y2 >> operation->shape.rotation >> sampleCount;
        operation->brush.erase = erase;
        operation->shape.kind = static_cast<ShapeKind>(shape);
        operation->shape.first = {x1, y1};
        operation->shape.second = {x2, y2};
        if (erase > 1 || !validBrush(operation->brush) || !validShape(operation->shape) ||
            (sampleCount == 0) != (version == 0) || !readableCount(stream, sampleCount, m_limits.samplesPerStroke, 56)) return Result::InvalidRecovery;
        if (quint64(retainedSamples) + sampleCount > retainedSampleLimit) return Result::CapacityExceeded;
        retainedSamples += sampleCount;
        auto samples = QSharedPointer<QList<InputSample>>::create();
        samples->reserve(sampleCount);
        quint64 time = 0;
        for (quint32 j = 0; j < sampleCount; ++j) {
            const auto sample = readSample(stream);
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
    QList<PropertyChange> changes;
    QSet<quint32> referencedOperations;
    QHash<LayerId, QUuid> layerIdentities;
    QHash<QUuid, LayerId> identityOwners;
    quint64 previousRevision = 0;
    quint64 previousModifiedRevision = 0;
    QSet<quint64> stateRevisions;
    for (quint32 i = 0; i < stateCount; ++i) {
        QSharedPointer<DocumentVersion> state(new DocumentVersion);
        state->m_document = identity;
        state->m_committed = true;
        state->size = {width, height};
        quint32 layerCount = 0;
        for (auto &component : state->projectionBackground) stream >> component;
        stream >> state->m_revision >> state->m_modifiedRevision >> state->activeLayer >> layerCount;
        if (state->revision() > nextRevision || state->m_modifiedRevision > state->revision() ||
            stateRevisions.contains(state->revision()) ||
            (i > 0 && state->m_modifiedRevision <= previousModifiedRevision) ||
            !readableCount(stream, layerCount, m_limits.layerCount, 40)) return Result::InvalidRecovery;
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
            stream >> count;
            if (visible > 1 || locked > 1 || pinned > 1 || layer.uuid.isNull() ||
                (layerIdentities.contains(layer.id) && layerIdentities.value(layer.id) != layer.uuid) ||
                (identityOwners.contains(layer.uuid) && identityOwners.value(layer.uuid) != layer.id) ||
                !readableCount(stream, count, operationCount, 4)) return Result::InvalidRecovery;
            layerIdentities.insert(layer.id, layer.uuid);
            identityOwners.insert(layer.uuid, layer.id);
            layer.visible = visible;
            layer.userLocked = locked;
            layer.pinnedToTimeline = pinned;
            quint64 previousOperation = 0;
            for (quint32 k = 0; k < count; ++k) {
                quint32 index = NoOperation;
                stream >> index;
                if (index >= operationCount) return Result::InvalidRecovery;
                const auto &operation = operations[index];
                const auto samples = operationSamples(operation);
                const auto pixels = operationRasterBytes(operation);
                if (operationLayer(operation) != layer.id || (std::holds_alternative<PaintSnapshot>(operation) && samples == 0) ||
                    operationId(operation).sequence <= previousOperation) return Result::InvalidRecovery;
                if (samples > m_limits.samplesPerDocument - state->sampleCount || pixels > m_limits.rasterBytes - state->rasterBytes) return Result::CapacityExceeded;
                previousOperation = operationId(operation).sequence;
                state->sampleCount += samples;
                state->rasterBytes += pixels;
                layer.operations.append(operation);
                referencedOperations.insert(index);
            }
            layers.insert(layer.id);
            state->layers.append(layer);
        }
        if (layers.isEmpty() ? state->activeLayer != 0 : !layers.contains(state->activeLayer)) return Result::InvalidRecovery;
        PropertyChange change;
        quint8 property = 0;
        stream >> property;
        if (property > quint8(LayerProperty::PinnedToTimeline)) return Result::InvalidRecovery;
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
            } else {
                quint8 before = 0, after = 0;
                stream >> before >> after;
                if (change.property == LayerProperty::Opacity) {
                    change.before = QVariant::fromValue(before);
                    change.after = QVariant::fromValue(after);
                } else {
                    if (before > 1 || after > 1) return Result::InvalidRecovery;
                    change.before = bool(before);
                    change.after = bool(after);
                }
            }
            if (change.before == change.after) return Result::InvalidRecovery;
        }
        changes.append(change);
        history.append(state);
    }
    quint32 pendingIndex = NoOperation;
    quint64 previewRevision = 0;
    stream >> pendingIndex >> previewRevision;
    if (stream.status() != QDataStream::Ok || !stream.atEnd()) return Result::InvalidRecovery;
    PaintSnapshot edit;
    Snapshot preview;
    if (pendingIndex != NoOperation) {
        if (pendingIndex >= operationCount || referencedOperations.contains(pendingIndex)) return Result::InvalidRecovery;
        const auto paint = std::get_if<PaintSnapshot>(&operations[pendingIndex]);
        if (!paint) return Result::InvalidRecovery;
        edit = *paint;
        const auto &base = history[currentIndex];
        qsizetype index = -1;
        for (qsizetype i = 0; i < base->layers.size(); ++i) {
            if (base->layers[i].id == edit->layer) index = i;
        }
        if (index < 0 || !base->layers[index].visible || base->layers[index].userLocked ||
            edit->sampleCount > m_limits.samplesPerDocument - base->sampleCount) return Result::InvalidRecovery;
        if (edit->sampleCount > 0) {
            if (previewRevision <= previousRevision || previewRevision > nextRevision || previewRevision == savedRevision) return Result::InvalidRecovery;
            QSharedPointer<DocumentVersion> value(new DocumentVersion(*base));
            value->m_committed = false;
            value->m_pendingOperation = edit->id;
            value->m_modifiedRevision = value->m_revision = previewRevision;
            value->layers[index].operations.append(edit);
            value->sampleCount += edit->sampleCount;
            preview = value;
        } else if (previewRevision != 0 || edit->shape.kind != ShapeKind::Freehand) return Result::InvalidRecovery;
        referencedOperations.insert(pendingIndex);
    } else if (previewRevision != 0) return Result::InvalidRecovery;
    if (referencedOperations.size() != operations.size()) return Result::InvalidRecovery;
    m_identity = identity;
    m_history = history;
    m_propertyChanges = changes;
    m_index = currentIndex;
    m_nextRevision = nextRevision;
    m_nextOperation = nextOperation;
    m_nextLayer = nextLayer;
    m_savedRevision = savedRevision;
    m_edit = edit;
    m_preview = preview;
    return Result::Success;
}
}
