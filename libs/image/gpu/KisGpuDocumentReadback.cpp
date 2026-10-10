/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentReadback.h"
#include "KisGpuTileGeometry_p.h"
#include <limits>
#include <set>
#include <utility>

KisGpuDocumentReadback::KisGpuDocumentReadback(std::shared_ptr<KisGpuTileStore> store,
    QVector<KisGpuLayerDocument::Layer> layers, KisGpuLayerDocument::LayerId nextId, quint64 maximum)
    : m_store(std::move(store))
{
    using Document = KisGpuLayerDocument;
    using Session = KisGpuEditSession;
    if (!m_store || !nextId) { fail(Error::InvalidDocument); return; }
    if (!m_store->deviceAvailable()) { fail(Error::DeviceLost); return; }
    const auto reserve = [&](quint64 count, quint64 size) {
        if (count > std::numeric_limits<quint64>::max() / size || count * size > maximum - m_reservedBytes) {
            fail(Error::BudgetExceeded);
            return false;
        }
        m_reservedBytes += count * size;
        return true;
    };
    if (!reserve(1, sizeof(Document::RecoveryFrame))) return;
    QVector<std::pair<qsizetype, qsizetype>> references;
    std::set<Document::LayerId> ids;
    const auto sourceIndex = [&](const KisGpuTileStore::Version &version) {
        for (qsizetype i = 0; i < m_sources.size(); ++i) if (m_sources[i].version == version) return i;
        m_sources.push_back({version, {}, {}, 0});
        return m_sources.size() - 1;
    };
    for (const auto &layer : layers) {
        if (!layer.id || layer.id >= nextId || !ids.insert(layer.id).second
            || (layer.paint.operation != KisGpuTileStore::CompositeOp::Over
                && layer.paint.operation != KisGpuTileStore::CompositeOp::Erase)) {
            fail(Error::InvalidDocument); return;
        }
        if (!reserve(1, sizeof(std::shared_ptr<const Document::RecoveryLayer>) + sizeof(Document::RecoveryLayer)
                     + sizeof(std::pair<qsizetype, qsizetype>))
            || !reserve(quint64(layer.name.size()), sizeof(QChar))) return;
        const auto pixels = sourceIndex(layer.paint.pixels);
        const auto mask = layer.paint.mask == KisGpuTileStore::Version{} ? -1 : sourceIndex(layer.paint.mask);
        references.push_back({pixels, mask});
    }
    for (auto &source : m_sources) {
        if (!reserve(1, sizeof(Source) + sizeof(Session::Recovery))
            || !reserve(quint64(source.version.tileCount()), KisGpuTileStore::TileBytes + sizeof(Session::RecoveryTile) + sizeof(QPoint))) return;
        const auto checked = m_store->paint(source.version, QVector<KisGpuTileStore::PaintCommand>{});
        if (checked.error != KisGpuTileStore::Error::None || checked.completion.status() != Status::Succeeded) {
            fail(checked.error == KisGpuTileStore::Error::DeviceLost ? Error::DeviceLost : Error::InvalidVersion);
            return;
        }
        source.coordinates = source.version.tileCoordinates();
        for (const auto coordinate : source.coordinates) {
            if (!KisGpuTileStorage::tileBounds(coordinate)) { fail(Error::InvalidVersion); return; }
        }
    }
    for (auto &source : m_sources) source.checkpoint = std::make_shared<Session::Recovery>();
    m_staging.history = {Document::RecoveryFrame{}};
    m_staging.nextId = nextId;
    auto &frame = m_staging.history[0];
    frame.reserve(layers.size());
    for (qsizetype i = 0; i < layers.size(); ++i) {
        const auto &layer = layers[i];
        const auto [pixels, mask] = references[i];
        frame.push_back(std::make_shared<const Document::RecoveryLayer>(Document::RecoveryLayer{
            layer.id, layer.name, layer.paint.opacity, layer.paint.operation,
            m_sources[pixels].checkpoint, mask < 0 ? Document::RasterRecovery{} : m_sources[mask].checkpoint}));
    }
}

void KisGpuDocumentReadback::fail(Error error)
{
    m_error = error;
    m_status = Status::Failed;
    m_latest = {};
    m_sources.clear();
    m_staging = {};
    m_reservedBytes = 0;
}

void KisGpuDocumentReadback::poll()
{
    if (m_status != Status::Pending) return;
    m_store->poll();
    if (!m_store->deviceAvailable()) { fail(Error::DeviceLost); return; }
    if (m_waiting) {
        const auto status = m_latest.completion.status();
        if (status == Status::Pending) return;
        const auto bytes = m_latest.bytes();
        if (status != Status::Succeeded || bytes.size() != KisGpuTileStore::TileBytes) { fail(Error::ReadFailed); return; }
        auto &source = m_sources[m_index];
        source.checkpoint->tiles.push_back({source.coordinates[source.cursor++], bytes});
        m_latest = {};
        m_waiting = false;
    }
    while (m_index < m_sources.size()) {
        auto &source = m_sources[m_index];
        if (source.cursor == source.coordinates.size()) { ++m_index; continue; }
        m_latest = m_store->readback(source.version, *KisGpuTileStorage::tileBounds(source.coordinates[source.cursor]));
        if (m_latest.error == KisGpuTileStore::Error::QueueFull) return;
        if (m_latest.error != KisGpuTileStore::Error::None) {
            fail(m_latest.error == KisGpuTileStore::Error::BudgetExceeded ? Error::BudgetExceeded
                 : m_latest.error == KisGpuTileStore::Error::DeviceLost ? Error::DeviceLost : Error::ReadFailed);
            return;
        }
        m_waiting = true;
        return;
    }
    m_result = std::move(m_staging);
    m_staging = {};
    m_sources.clear();
    m_status = Status::Succeeded;
}
