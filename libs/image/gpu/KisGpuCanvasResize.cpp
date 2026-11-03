/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include <limits>

namespace LibrePaint::Image::Gpu {
Result OperationDocument::resizeCanvas(const QRect &rect, bool cropLayers)
{ return resizeCanvasWithMasks(rect, cropLayers, {}); }

Result OperationDocument::resizeCanvasWithMasks(const QRect &rect, bool cropLayers,
    const QHash<QUuid, SelectionMaskSP> &masks, const SelectionMaskSP &selection)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!Detail::validCanvasRect(rect)) return Result::InvalidInput;
    if (!cropLayers && rect == QRect(QPoint(), snapshot()->size)) return Result::Success;
    const QPoint offset = -rect.topLeft();
    quint64 paintLayers = 0;
    for (const auto &layer : snapshot()->layers) paintLayers += layer.kind == LayerKind::Paint;
    const quint64 count = paintLayers * (int(cropLayers) + int(!offset.isNull()));
    if (m_nextRevision == std::numeric_limits<quint64>::max() ||
        count > std::numeric_limits<quint64>::max() - m_nextOperation) return Result::CapacityExceeded;
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    const auto withinDomain = [&](QRect bounds) {
        return bounds.isEmpty() || (qint64(bounds.left()) + offset.x() >= -limit && qint64(bounds.top()) + offset.y() >= -limit &&
            qint64(bounds.right()) + offset.x() < limit && qint64(bounds.bottom()) + offset.y() < limit);
    };
    if (!cropLayers) {
        if (!withinDomain(snapshot()->selection)) return Result::InvalidInput;
        for (const auto &layer : snapshot()->layers) {
            for (const auto &mask : {layer.filterSelection, layer.transparencyMask})
                if (mask && !withinDomain(mask->bounds() | mask->allocatedBounds())) return Result::InvalidInput;
        }
    }
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    auto nextOperation = m_nextOperation;
    for (auto &layer : version->layers) {
        const qint64 x = qint64(layer.position.x()) + offset.x(), y = qint64(layer.position.y()) + offset.y();
        if (layer.kind != LayerKind::Group && (x < std::numeric_limits<qint32>::min() || x > std::numeric_limits<qint32>::max() ||
            y < std::numeric_limits<qint32>::min() || y > std::numeric_limits<qint32>::max())) return Result::InvalidInput;
        if (cropLayers && layer.kind == LayerKind::Paint) {
            auto crop = QSharedPointer<CropOperation>::create();
            crop->id = {m_identity, ++nextOperation};
            crop->layer = layer.id;
            crop->rect = rect;
            layer.operations.append(CropSnapshot(crop));
        }
        const auto moveMask = [&](const SelectionMaskSP &coverage, const QUuid &uuid) {
            const QRect clip = cropLayers ? QRect(QPoint(), rect.size()) :
                QRect(-RgbaTile::CoordinateLimit, -RgbaTile::CoordinateLimit, 2 * RgbaTile::CoordinateLimit, 2 * RgbaTile::CoordinateLimit);
            auto result = masks.value(uuid);
            if (!result && !coverage->isGpuGenerated()) result = coverage->translated(offset, clip);
            return result && result->byteCount() <= m_limits.rasterBytes && selectionFitsInputs(result) ? result : SelectionMaskSP{};
        };
        if (layer.filterSelection) {
            const auto moved = moveMask(layer.filterSelection, layer.uuid);
            if (!moved) return Result::CapacityExceeded;
            layer.filterSelection = moved;
        }
        if (layer.transparencyMask) {
            const qint64 maskX = qint64(layer.transparencyMaskPosition.x()) + offset.x();
            const qint64 maskY = qint64(layer.transparencyMaskPosition.y()) + offset.y();
            if (maskX < std::numeric_limits<qint32>::min() || maskX > std::numeric_limits<qint32>::max() ||
                maskY < std::numeric_limits<qint32>::min() || maskY > std::numeric_limits<qint32>::max()) return Result::InvalidInput;
            const auto moved = moveMask(layer.transparencyMask, layer.transparencyMaskUuid);
            if (!moved) return Result::CapacityExceeded;
            layer.transparencyMask = moved;
            layer.transparencyMaskPosition = {int(maskX), int(maskY)};
        }
        if (!offset.isNull() && layer.kind == LayerKind::Paint) {
            auto combined = offset;
            if (!layer.operations.isEmpty()) {
                if (const auto previous = std::get_if<TranslationSnapshot>(&layer.operations.last())) {
                    const qint64 dx = qint64((*previous)->offset.x()) + offset.x();
                    const qint64 dy = qint64((*previous)->offset.y()) + offset.y();
                    if (qAbs(dx) > 2 * qint64(RgbaTile::CoordinateLimit) ||
                        qAbs(dy) > 2 * qint64(RgbaTile::CoordinateLimit)) return Result::InvalidInput;
                    combined = {int(dx), int(dy)};
                    layer.operations.removeLast();
                }
            }
            if (!combined.isNull()) {
                auto translation = QSharedPointer<TranslationOperation>::create();
                translation->id = {m_identity, ++nextOperation};
                translation->version = 1;
                translation->layer = layer.id;
                translation->offset = combined;
                layer.operations.append(TranslationSnapshot(translation));
            }
        }
        if (layer.kind != LayerKind::Group) layer.position = {int(x), int(y)};
    }
    if (!version->selection.isEmpty()) {
        const auto oldRect = version->selection;
        const bool clippedEllipse = cropLayers && version->selectionKind == SelectionKind::Ellipse && !rect.contains(oldRect);
        if (version->selectionKind == SelectionKind::Freehand || clippedEllipse) {
            auto moved = selection;
            if (!moved) {
                auto source = version->selectionMask ? version->selectionMask : SelectionMask::fromEllipse(oldRect, rect);
                if (!source && clippedEllipse) source = SelectionMask::empty();
                const QRect domain(-RgbaTile::CoordinateLimit, -RgbaTile::CoordinateLimit, 2 * RgbaTile::CoordinateLimit, 2 * RgbaTile::CoordinateLimit);
                if (source && !source->isGpuGenerated()) moved = source->translated(offset, cropLayers ? QRect(QPoint(), rect.size()) : domain);
            }
            if (!moved || !selectionFitsInputs(moved)) return Result::CapacityExceeded;
            version->selectionMask = moved->isEmpty() ? SelectionMaskSP{} : moved;
            version->selection = moved->bounds();
            version->selectionKind = moved->isEmpty() ? SelectionKind::Rectangle : SelectionKind::Freehand;
        } else {
            const auto selected = cropLayers ? oldRect.intersected(rect) : oldRect;
            version->selection = selected.isEmpty() ? QRect{} : selected.translated(offset);
        }
    }
    version->size = rect.size();
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.canvasResize = CanvasResize{rect, cropLayers};
    change.changedSelection = version->selection != snapshot()->selection || version->selectionKind != snapshot()->selectionKind ||
        version->selectionMask != snapshot()->selectionMask;
    m_nextOperation = nextOperation;
    commitVersion(version, change);
    return Result::Success;
}
}
