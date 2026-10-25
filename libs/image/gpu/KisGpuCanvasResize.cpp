/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include <limits>

namespace LibrePaint::Image::Gpu {
Result OperationDocument::resizeCanvas(const QRect &rect, bool cropLayers)
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
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    auto nextOperation = m_nextOperation;
    for (auto &layer : version->layers) {
        if (layer.kind == LayerKind::Group) continue;
        const qint64 x = qint64(layer.position.x()) + offset.x(), y = qint64(layer.position.y()) + offset.y();
        if (x < std::numeric_limits<qint32>::min() || x > std::numeric_limits<qint32>::max() ||
            y < std::numeric_limits<qint32>::min() || y > std::numeric_limits<qint32>::max()) return Result::InvalidInput;
        if (cropLayers) {
            auto crop = QSharedPointer<CropOperation>::create();
            crop->id = {m_identity, ++nextOperation};
            crop->layer = layer.id;
            crop->rect = rect;
            layer.operations.append(CropSnapshot(crop));
        }
        if (layer.transparencyMask) {
            layer.transparencyMask = layer.transparencyMask->translated(offset, QRect(QPoint(), rect.size()));
        }
        if (!offset.isNull()) {
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
        layer.position = {int(x), int(y)};
    }
    version->size = rect.size();
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.canvasResize = CanvasResize{rect, cropLayers};
    m_nextOperation = nextOperation;
    commitVersion(version, change);
    return Result::Success;
}
}
