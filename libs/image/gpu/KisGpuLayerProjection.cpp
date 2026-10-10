/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerProjection.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

KisGpuLayerProjection::KisGpuLayerProjection(KisGpuTileStore &store, QRect bounds)
    : m_store(store), m_bounds(bounds)
{
    if (bounds.isEmpty()
        || qint64(bounds.x()) + bounds.width() - 1 > std::numeric_limits<int>::max()
        || qint64(bounds.y()) + bounds.height() - 1 > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("GPU layer projection requires representable nonempty canvas bounds");
    }
    m_published.pixels = store.emptyVersion();
    m_requested = m_published;
    m_completion = store.project(m_published.pixels, {}, {}).completion;
}

QRect KisGpuLayerProjection::damage(const QVector<KisGpuTileStore::Layer> &before,
                                  const QVector<KisGpuTileStore::Layer> &after) const
{
    QRect result;
    const auto include = [&](QPoint coordinate) {
        const qint64 x = qint64(coordinate.x()) * 64, y = qint64(coordinate.y()) * 64;
        const qint64 left = std::max(x, qint64(m_bounds.x())), top = std::max(y, qint64(m_bounds.y()));
        const qint64 right = std::min(x + 64, qint64(m_bounds.x()) + m_bounds.width());
        const qint64 bottom = std::min(y + 64, qint64(m_bounds.y()) + m_bounds.height());
        if (left < right && top < bottom) {
            result = result.united(QRect(int(left), int(top), int(right - left), int(bottom - top)));
        }
    };
    for (qsizetype i = 0; i < std::max(before.size(), after.size()); ++i) {
        const auto *oldLayer = i < before.size() ? &before[i] : nullptr;
        const auto *newLayer = i < after.size() ? &after[i] : nullptr;
        if (oldLayer && newLayer && oldLayer->opacity == newLayer->opacity
            && oldLayer->operation == newLayer->operation && oldLayer->mask == newLayer->mask) {
            if (!newLayer->opacity || oldLayer->pixels == newLayer->pixels) continue;
            const auto changed = [&](QPoint coordinate) {
                const auto oldTile = oldLayer->pixels.tile(coordinate), newTile = newLayer->pixels.tile(coordinate);
                if (oldTile.buffer != newTile.buffer || oldTile.offset != newTile.offset) include(coordinate);
            };
            for (const auto coordinate : oldLayer->pixels.tileCoordinates()) changed(coordinate);
            for (const auto coordinate : newLayer->pixels.tileCoordinates()) changed(coordinate);
        } else {
            if (oldLayer && oldLayer->opacity) {
                for (const auto coordinate : oldLayer->pixels.tileCoordinates()) include(coordinate);
            }
            if (newLayer && newLayer->opacity) {
                for (const auto coordinate : newLayer->pixels.tileCoordinates()) include(coordinate);
            }
        }
    }
    return result;
}

KisGpuTileStore::Error KisGpuLayerProjection::setLayers(const QVector<KisGpuTileStore::Layer> &layers)
{
    const auto &base = status() == KisGpuTileStore::Status::Failed ? m_published : m_requested;
    const auto edit = m_store.project(base.pixels, layers, damage(base.layers, layers));
    if (edit.error != KisGpuTileStore::Error::None) return edit.error;
    m_requested = {layers, edit.version};
    m_completion = edit.completion;
    return KisGpuTileStore::Error::None;
}

void KisGpuLayerProjection::poll()
{
    m_store.poll();
    if (status() == KisGpuTileStore::Status::Succeeded) m_published = m_requested;
}
