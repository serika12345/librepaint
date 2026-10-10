/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuEditReplay_p.h"

KisGpuEditSession::Result KisGpuEditSession::compactRecovery()
{
    if (m_replay->compacting || m_state == State::Restoring || m_state == State::Failed) return Result::Busy;
    if (!m_replay->data) { m_lastGpuError = KisGpuTileStore::Error::InvalidVersion; return Result::GpuRejected; }
    if (!m_store.deviceAvailable()) { m_lastGpuError = KisGpuTileStore::Error::DeviceLost; return Result::GpuRejected; }
    const auto bytes = quint64(m_history.front().tileCount()) * (KisGpuTileStore::TileBytes + sizeof(RecoveryTile));
    const auto existing = ReplayState::recoveryBytes(*m_replay->data, m_token ? &m_replay->working : nullptr);
    if (bytes > m_replay->maximumBytes || existing > m_replay->maximumBytes - bytes) return Result::RecoveryBudgetExceeded;
    ReplayState::Compaction job;
    job.source = m_history.front();
    job.coordinates = job.source.tileCoordinates();
    for (const auto coordinate : job.coordinates) {
        if (!ReplayState::tileBounds(coordinate)) { m_lastGpuError = KisGpuTileStore::Error::InvalidCommand; return Result::GpuRejected; }
    }
    job.tiles.reserve(job.coordinates.size());
    job.firstRetained = m_replay->data->firstRetained;
    job.reservedBytes = bytes;
    m_replay->compacting = std::move(job);
    m_lastGpuError = KisGpuTileStore::Error::None;
    m_replay->compactionStatus = KisGpuTileStore::Status::Pending;
    return Result::Accepted;
}

std::optional<KisGpuTileStore::Status> KisGpuEditSession::recoveryCompactionStatus() const
{
    return m_replay->compactionStatus;
}

void KisGpuEditSession::pollRecoveryCompaction()
{
    auto &job = *m_replay->compacting;
    const auto fail = [&] {
        m_replay->compactionStatus = KisGpuTileStore::Status::Failed;
        m_replay->compacting.reset();
    };
    if (!m_store.deviceAvailable()) { m_lastGpuError = KisGpuTileStore::Error::DeviceLost; fail(); return; }
    if (job.waiting) {
        const auto status = job.latest.completion.status();
        if (status == KisGpuTileStore::Status::Pending) return;
        if (status == KisGpuTileStore::Status::Failed) { fail(); return; }
        job.tiles.push_back({job.coordinates[job.tiles.size()], job.latest.bytes()});
        job.latest = {};
        job.waiting = false;
    }
    if (job.tiles.size() < job.coordinates.size()) {
        const auto coordinate = job.coordinates[job.tiles.size()];
        job.latest = m_store.readback(job.source, *ReplayState::tileBounds(coordinate));
        if (job.latest.error == KisGpuTileStore::Error::QueueFull) return;
        if (job.latest.error != KisGpuTileStore::Error::None) { m_lastGpuError = job.latest.error; fail(); return; }
        job.waiting = true;
        return;
    }
    auto &data = *m_replay->data;
    data.bounds = {};
    data.pixels.clear();
    data.tiles = std::move(job.tiles);
    data.edits.remove(0, job.firstRetained);
    data.cursor -= job.firstRetained;
    data.firstRetained -= job.firstRetained;
    m_replay->compactionStatus = KisGpuTileStore::Status::Succeeded;
    m_replay->compacting.reset();
}
