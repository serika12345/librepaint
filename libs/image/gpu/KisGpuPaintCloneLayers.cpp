/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPaintImage_p.h"
#include "KisGpuCloneProjection_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <QSet>
#include <new>

namespace LibrePaint::Image::Gpu {
LayerId PaintImage::addCloneLayer(const RasterLayer &node, qsizetype index)
{
    if (!onThread()) return 0;
    const auto id = m_d->document->addCloneLayer(node, index);
    if (id) publish(Result::Success);
    return id;
}
Result PaintImage::setCloneSource(LayerId layer, const QUuid &source, bool fromOriginal)
{ return onThread() ? publish(m_d->document->setCloneSource(layer, source, fromOriginal)) : Result::InvalidInput; }
Result PaintImage::writeCloneSource(LayerId layer, const QUuid &source, bool fromOriginal)
{ return onThread() ? requestAfterWrite(m_d->document->writeCloneSource(layer, source, fromOriginal)) : Result::InvalidInput; }

OperationId PaintImage::beginCloneSourceEdit(const QList<LayerId> &layers)
{
    if (!onThread()) return {};
    const auto id = m_d->document->beginCloneSourceEdit(layers);
    if (!id.isNull()) publish(Result::Success);
    return id;
}
Result PaintImage::updateCloneSourceEdit(OperationId id, const QUuid &source)
{ return onThread() ? publish(m_d->document->updateCloneSourceEdit(id, source)) : Result::InvalidInput; }
Result PaintImage::commitCloneSourceEdit(OperationId id)
{ return onThread() ? publish(m_d->document->commitCloneSourceEdit(id)) : Result::InvalidInput; }
Result PaintImage::cancelCloneSourceEdit(OperationId id)
{ return onThread() ? publish(m_d->document->cancelCloneSourceEdit(id)) : Result::InvalidInput; }

Result PaintImage::removeLayer(LayerId layer)
{ return removeLayers({layer}); }

Result PaintImage::removeLayers(const QList<LayerId> &layers, const QString &replacementName)
{
    if (!onThread()) return Result::InvalidInput;
    const auto status = m_d->document->checkMutation();
    if (status != Result::Success) return status;
    try {
        const auto before = m_d->document->snapshot();
        QSet<LayerId> removed(layers.cbegin(), layers.cend());
        QSet<QUuid> removedIdentities;
        for (const auto &layer : before->layers) if (removed.contains(layer.id)) removedIdentities.insert(layer.uuid);
        bool expanded = true;
        while (expanded) {
            expanded = false;
            for (const auto &layer : before->layers) {
                if (!removed.contains(layer.id) && removedIdentities.contains(layer.parentUuid)) {
                    removed.insert(layer.id);
                    removedIdentities.insert(layer.uuid);
                    expanded = true;
                }
            }
        }
        QHash<LayerId, Layer> detached;
        for (const auto &layer : before->layers) {
            if (layer.kind != LayerKind::Clone || removed.contains(layer.id) ||
                !removedIdentities.contains(layer.cloneSourceUuid)) continue;
            const auto resolved = Detail::CloneLayerProjection::resolve(before, layer, Detail::PaintGeometryElementLimit);
            if (!resolved) return Result::CapacityExceeded;
            detached.insert(layer.id, *resolved);
        }
        return publish(m_d->document->removeLayers(layers, replacementName, detached));
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}
}
