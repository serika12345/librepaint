/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerDocument.h"
#include "KisGpuEditReplay_p.h"
#include <set>
#include <stdexcept>

KisGpuLayerDocument::Result KisGpuLayerDocument::compactRecovery(LayerId id, Target target)
{
    using Session = KisGpuEditSession;
    const auto ready = readiness();
    if (ready != Result::Accepted) return ready;
    const auto index = indexOf(id);
    if (index < 0) return Result::InvalidLayer;
    if (target != Target::Pixels && target != Target::Mask) return Result::InvalidInput;
    const auto &layer = *m_history[m_cursor][index];
    const auto source = target == Target::Pixels ? layer.pixels : layer.mask;
    if (!source) return Result::InvalidInput;
    if (source->edits.isEmpty()) {
        m_compactionStatus = KisGpuTileStore::Status::Succeeded;
        m_compactionError = KisGpuTileStore::Error::None;
        return Result::Accepted;
    }
    const auto version = rasterVersion(source);
    const auto checkpoint = quint64(version.tileCount()) * (KisGpuTileStore::TileBytes + sizeof(Session::RecoveryTile));
    const auto payload = Session::ReplayState::recoveryBytes(*source);
    // Reserve both immutable histories and the session's source/pending checkpoint.
    quint64 metadata = 2 * sizeof(Session::Recovery) + quint64(m_history.size()) * sizeof(Frame);
    std::set<const LayerState *> counted;
    for (const auto &frame : m_history) {
        metadata += quint64(frame.size()) * sizeof(Record);
        for (const auto &record : frame) {
            if ((record->pixels == source || record->mask == source) && counted.insert(record.get()).second)
                metadata += recordMetadataBytes(*record);
        }
    }
    const auto remaining = m_maximumRecoveryBytes - historyBytes(m_history);
    if (metadata > remaining || payload > remaining - metadata
        || checkpoint > remaining - metadata - payload) return Result::RecoveryBudgetExceeded;
    std::shared_ptr<Session> session;
    try {
        session = std::shared_ptr<Session>(new Session(*m_store, version, *source, remaining - metadata),
                                          [store = m_store](Session *value) { delete value; });
    } catch (const std::invalid_argument &) {
        if (!m_store->deviceAvailable()) return Result::GpuRejected;
        throw;
    }
    const auto result = session->compactRecovery();
    if (result != Session::Result::Accepted) {
        return result == Session::Result::RecoveryBudgetExceeded ? Result::RecoveryBudgetExceeded : Result::GpuRejected;
    }
    m_compacting = Compaction{source, std::move(session), metadata + payload + checkpoint};
    m_compactionStatus = KisGpuTileStore::Status::Pending;
    m_compactionError = KisGpuTileStore::Error::None;
    return Result::Accepted;
}

std::optional<KisGpuTileStore::Status> KisGpuLayerDocument::recoveryCompactionStatus() const
{
    return m_compactionStatus;
}

KisGpuTileStore::Error KisGpuLayerDocument::recoveryCompactionError() const
{
    return m_compactionError;
}

void KisGpuLayerDocument::pollRecoveryCompaction()
{
    auto &job = *m_compacting;
    job.session->poll();
    const auto status = job.session->recoveryCompactionStatus();
    if (status == KisGpuTileStore::Status::Pending) return;
    m_compactionStatus = status;
    m_compactionError = job.session->lastGpuError();
    if (status == KisGpuTileStore::Status::Succeeded) {
        const auto checkpoint = std::make_shared<const KisGpuEditSession::Recovery>(*job.session->recovery());
        auto history = m_history;
        std::map<const LayerState *, Record> replacements;
        for (auto &frame : history) for (auto &record : frame) {
            if (record->pixels != job.source && record->mask != job.source) continue;
            auto found = replacements.find(record.get());
            if (found == replacements.end()) {
                auto replacement = std::make_shared<LayerState>(*record);
                if (replacement->pixels == job.source) replacement->pixels = checkpoint;
                if (replacement->mask == job.source) replacement->mask = checkpoint;
                found = replacements.emplace(record.get(), std::move(replacement)).first;
            }
            record = found->second;
        }
        m_rasterVersions[checkpoint.get()] = rasterVersion(job.source);
        m_history = std::move(history);
        pruneRasterVersions();
    }
    m_compacting.reset();
}
