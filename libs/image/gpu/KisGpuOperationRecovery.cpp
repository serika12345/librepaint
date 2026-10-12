/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QHash>
#include <QIODevice>
#include <QSet>
#include <limits>

namespace Krita::Image::Gpu {
namespace {
constexpr quint32 Magic = 0x4c504f52; // LPOR
constexpr quint16 FormatVersion = 1;
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
    QList<QSharedPointer<const PaintOperation>> operations;
    QHash<const PaintOperation *, quint32> indices;
    const auto registerOperation = [&](const QSharedPointer<const PaintOperation> &operation) {
        if (!indices.contains(operation.data())) {
            indices.insert(operation.data(), quint32(operations.size()));
            operations.append(operation);
        }
    };
    for (const auto &state : m_history) {
        for (const auto &layer : state->layers) {
            for (const auto &operation : layer.operations) registerOperation(operation);
        }
    }
    if (m_edit) registerOperation(m_edit);
    if (quint64(operations.size()) >= NoOperation) return {};

    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    configure(stream);
    const auto withinLimit = [&] {
        return stream.status() == QDataStream::Ok && bytes.size() <= m_limits.recoveryBytes - DigestSize;
    };
    stream << Magic << FormatVersion << qint32(snapshot()->size.width()) << qint32(snapshot()->size.height())
           << m_nextRevision << m_nextOperation << m_nextLayer << m_savedRevision
           << quint32(operations.size());
    if (!withinLimit()) return {};
    for (const auto &operation : operations) {
        stream << operation->id.sequence << operation->version << operation->layer;
        for (auto component : operation->brush.rgba) stream << component;
        stream << operation->brush.diameter << operation->brush.spacing << operation->brush.opacity
               << quint8(operation->brush.erase) << operation->randomSeed << quint8(operation->shape.kind)
               << operation->shape.first.x() << operation->shape.first.y()
               << operation->shape.second.x() << operation->shape.second.y() << operation->shape.rotation
               << quint32(operation->sampleCount);
        if (!withinLimit()) return {};
        for (const auto &block : operation->sampleBlocks) {
            if (block->size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 56) return {};
            for (const auto &sample : *block) writeSample(stream, sample);
        }
        if (!withinLimit()) return {};
    }
    stream << quint32(m_history.size()) << quint32(m_index);
    for (const auto &state : m_history) {
        stream << state->revision() << state->activeLayer << quint32(state->layers.size());
        if (!withinLimit()) return {};
        for (const auto &layer : state->layers) {
            const auto name = layer.name.toUtf8();
            if (name.size() > m_limits.recoveryBytes - DigestSize - bytes.size() - 17) return {};
            stream << layer.id << quint32(name.size());
            stream.writeRawData(name.constData(), int(name.size()));
            stream << quint8(layer.visible) << quint32(layer.operations.size());
            if (!withinLimit() ||
                layer.operations.size() > (m_limits.recoveryBytes - DigestSize - bytes.size()) / 4) return {};
            for (const auto &operation : layer.operations) stream << indices.value(operation.data());
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
    if (data.size() <= DigestSize || data.right(DigestSize) != digest(data, data.size() - DigestSize)) {
        return Result::InvalidRecovery;
    }
    const auto payload = QByteArray::fromRawData(data.constData(), data.size() - DigestSize);
    QDataStream stream(payload);
    configure(stream);
    quint32 magic = 0, operationCount = 0;
    quint16 format = 0;
    qint32 width = 0, height = 0;
    quint64 nextRevision = 0, nextOperation = 0, nextLayer = 0, savedRevision = 0;
    stream >> magic >> format >> width >> height >> nextRevision >> nextOperation >> nextLayer >> savedRevision
           >> operationCount;
    if (magic != Magic || format != FormatVersion || width <= 0 || height <= 0 ||
        width > CoordinateLimit || height > CoordinateLimit || savedRevision > nextRevision || nextLayer == 0 ||
        !readableCount(stream, operationCount, payload.size(), 99)) {
        return Result::InvalidRecovery;
    }

    const QUuid identity = QUuid::createUuid();
    QList<QSharedPointer<const PaintOperation>> operations;
    QSet<quint64> operationIds;
    qsizetype retainedSamples = 0;
    for (quint32 i = 0; i < operationCount; ++i) {
        QSharedPointer<PaintOperation> operation(new PaintOperation);
        operation->id.document = identity;
        stream >> operation->id.sequence >> operation->version >> operation->layer;
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
        if (operation->id.sequence == 0 || operation->id.sequence > nextOperation ||
            operationIds.contains(operation->id.sequence) || operation->layer == 0 || operation->layer > nextLayer ||
            operation->version > nextRevision || erase > 1 || !Detail::validBrush(operation->brush) ||
            !Detail::validShape(operation->shape) || (sampleCount == 0) != (operation->version == 0) ||
            !readableCount(stream, sampleCount, m_limits.samplesPerStroke, 56)) {
            return Result::InvalidRecovery;
        }
        // Current/redo versions share their prefix. A pending branch can add one stroke.
        const quint64 totalLimit = quint64(m_limits.samplesPerDocument) + quint64(m_limits.samplesPerStroke);
        if (quint64(retainedSamples) + sampleCount > totalLimit) return Result::CapacityExceeded;
        retainedSamples += sampleCount;
        QSharedPointer<QList<InputSample>> samples(new QList<InputSample>);
        samples->reserve(sampleCount);
        quint64 time = 0;
        for (quint32 j = 0; j < sampleCount; ++j) {
            const auto sample = readSample(stream);
            if (!Detail::validSample(sample) || sample.timestamp < time) return Result::InvalidRecovery;
            time = sample.timestamp;
            samples->append(sample);
        }
        if (sampleCount != 0) operation->sampleBlocks.append(samples);
        operation->sampleCount = sampleCount;
        operationIds.insert(operation->id.sequence);
        operations.append(operation);
    }

    quint32 stateCount = 0, currentIndex = 0;
    stream >> stateCount >> currentIndex;
    if (stateCount == 0 || currentIndex >= stateCount ||
        !readableCount(stream, stateCount, HistoryLimit + 1, 20)) return Result::InvalidRecovery;
    QList<Snapshot> history;
    QSet<quint32> referencedOperations;
    quint64 previousRevision = 0;
    for (quint32 i = 0; i < stateCount; ++i) {
        QSharedPointer<DocumentVersion> state(new DocumentVersion);
        state->m_document = identity;
        state->m_committed = true;
        state->size = {width, height};
        quint32 layerCount = 0;
        stream >> state->m_revision >> state->activeLayer >> layerCount;
        if (state->revision() > nextRevision || (i > 0 && state->revision() <= previousRevision) ||
            layerCount == 0 || !readableCount(stream, layerCount, m_limits.layerCount, 17)) return Result::InvalidRecovery;
        previousRevision = state->revision();
        QSet<LayerId> layers;
        for (quint32 j = 0; j < layerCount; ++j) {
            Layer layer;
            quint32 nameSize = 0, count = 0;
            quint8 visible = 0;
            stream >> layer.id >> nameSize;
            if (layer.id == 0 || layer.id > nextLayer || layers.contains(layer.id) ||
                !readableCount(stream, nameSize, Detail::LayerNameByteLimit, 1)) return Result::InvalidRecovery;
            QByteArray name(nameSize, Qt::Uninitialized);
            if (stream.readRawData(name.data(), nameSize) != int(nameSize)) return Result::InvalidRecovery;
            layer.name = QString::fromUtf8(name);
            if (layer.name.toUtf8() != name) return Result::InvalidRecovery;
            stream >> visible >> count;
            if (visible > 1 || !readableCount(stream, count, operationCount, 4)) return Result::InvalidRecovery;
            layer.visible = visible;
            quint64 previousOperation = 0;
            for (quint32 k = 0; k < count; ++k) {
                quint32 index = NoOperation;
                stream >> index;
                if (index >= operationCount) return Result::InvalidRecovery;
                const auto &operation = operations[index];
                if (operation->layer != layer.id || operation->sampleCount == 0 ||
                    operation->id.sequence <= previousOperation ||
                    operation->sampleCount > m_limits.samplesPerDocument - state->sampleCount) {
                    return Result::InvalidRecovery;
                }
                previousOperation = operation->id.sequence;
                state->sampleCount += operation->sampleCount;
                layer.operations.append(operation);
                referencedOperations.insert(index);
            }
            layers.insert(layer.id);
            state->layers.append(layer);
        }
        if (!layers.contains(state->activeLayer)) return Result::InvalidRecovery;
        history.append(state);
    }

    quint32 pendingIndex = NoOperation;
    quint64 previewRevision = 0;
    stream >> pendingIndex >> previewRevision;
    if (stream.status() != QDataStream::Ok || !stream.atEnd()) return Result::InvalidRecovery;
    QSharedPointer<const PaintOperation> edit;
    Snapshot preview;
    if (pendingIndex != NoOperation) {
        if (pendingIndex >= operationCount || referencedOperations.contains(pendingIndex)) return Result::InvalidRecovery;
        edit = operations[pendingIndex];
        const auto &base = history[currentIndex];
        qsizetype index = -1;
        for (qsizetype i = 0; i < base->layers.size(); ++i) {
            if (base->layers[i].id == edit->layer) index = i;
        }
        if (index < 0 || !base->layers[index].visible ||
            edit->sampleCount > m_limits.samplesPerDocument - base->sampleCount) return Result::InvalidRecovery;
        if (edit->sampleCount > 0) {
            if (previewRevision <= previousRevision || previewRevision > nextRevision || previewRevision == savedRevision) {
                return Result::InvalidRecovery;
            }
            QSharedPointer<DocumentVersion> value(new DocumentVersion(*base));
            value->m_committed = false;
            value->m_revision = previewRevision;
            value->layers[index].operations.append(edit);
            value->sampleCount += edit->sampleCount;
            preview = value;
        } else if (previewRevision != 0 || edit->shape.kind != ShapeKind::Freehand) {
            return Result::InvalidRecovery;
        }
        referencedOperations.insert(pendingIndex);
    } else if (previewRevision != 0) {
        return Result::InvalidRecovery;
    }
    if (referencedOperations.size() != operations.size()) return Result::InvalidRecovery;

    m_identity = identity;
    m_history = history;
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
