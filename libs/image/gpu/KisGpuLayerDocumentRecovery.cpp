/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerDocument.h"
#include "KisGpuEditReplay_p.h"
#include <set>
#include <stdexcept>

KisGpuLayerDocument::KisGpuLayerDocument(std::shared_ptr<KisGpuTileStore> store, qsizetype retained,
                                       Recovery request, quint64 maximum)
    : KisGpuLayerDocument(std::move(store), retained, maximum)
{
    if (!validRecovery(request, m_retainedEdits, maximum)) throw std::invalid_argument("Invalid GPU layer document recovery history or CPU payload budget");
    m_restoring.emplace();
    m_restoring->request = std::move(request);
    m_restoring->records = m_restoring->request.history[m_restoring->request.cursor];
}

quint64 KisGpuLayerDocument::recoveryBytes(const Recovery &request)
{
    quint64 bytes = quint64(request.history.size()) * sizeof(Frame);
    std::set<const RecoveryLayer *> counted;
    std::set<const KisGpuEditSession::Recovery *> rasters;
    for (const auto &frame : request.history) {
        bytes += quint64(frame.size()) * sizeof(Record);
        for (const auto &record : frame) {
            if (counted.insert(record.get()).second) {
                bytes += sizeof(LayerState) + quint64(record->name.size()) * sizeof(QChar);
                for (const auto &raster : {record->pixels, record->mask}) {
                    if (raster && rasters.insert(raster.get()).second)
                        bytes += sizeof(KisGpuEditSession::Recovery) + KisGpuEditSession::ReplayState::recoveryBytes(*raster);
                }
            }
        }
    }
    if (request.working) bytes += sizeof(KisGpuEditSession::Recovery) + KisGpuEditSession::ReplayState::recoveryBytes(request.working->data);
    return bytes;
}

std::optional<quint64> KisGpuLayerDocument::workingMaximum(const Recovery &request, quint64 maximum)
{
    const auto &work = *request.working;
    const auto &base = *work.base;
    const auto working = sizeof(KisGpuEditSession::Recovery) + KisGpuEditSession::ReplayState::recoveryBytes(work.data);
    const auto existing = recoveryBytes(request) - working;
    const auto extra = sizeof(LayerState) + sizeof(KisGpuEditSession::Recovery) + quint64(base.name.size()) * sizeof(QChar)
        + sizeof(Frame) + quint64(request.history[request.cursor].size()) * sizeof(Record);
    return existing > maximum || extra > maximum - existing
        ? std::nullopt : std::optional<quint64>(maximum - existing - extra);
}

bool KisGpuLayerDocument::validRecovery(const Recovery &request, qsizetype retained, quint64 maximum)
{
    using Session = KisGpuEditSession;
    if (request.history.isEmpty() || request.history.size() - 1 > qMax(qsizetype(0), retained)
        || request.cursor < 0 || request.cursor >= request.history.size() || !request.nextId) return false;
    const auto completed = [&](const Session::Recovery &data) {
        return Session::ReplayState::validRecovery(data, 0, maximum)
            && data.cursor == data.edits.size() && !data.working;
    };
    LayerId highest = 0;
    std::set<const RecoveryLayer *> checked;
    for (const auto &frame : request.history) {
        std::set<LayerId> ids;
        for (const auto &record : frame) {
            if (!record || !record->id || !ids.insert(record->id).second) return false;
            highest = qMax(highest, record->id);
            if (checked.insert(record.get()).second) {
                if ((record->operation != KisGpuTileStore::CompositeOp::Over && record->operation != KisGpuTileStore::CompositeOp::Erase)
                    || !record->pixels || !completed(*record->pixels) || (record->mask && !completed(*record->mask))) return false;
            }
        }
    }
    if (request.nextId <= highest) return false;
    if (request.working) {
        const auto &work = *request.working;
        const auto &frame = request.history[request.cursor];
        if (!work.base || !frame.contains(work.base) || (work.target != Target::Pixels && work.target != Target::Mask)
            || !Session::ReplayState::validRecovery(work.data, 0, maximum)
            || work.data.cursor != work.data.edits.size()
            || (work.completed ? !completed(work.data) : !work.data.working.has_value())
            || !workingMaximum(request, maximum)
            || Session::ReplayState::recoveryBytes(work.data) > *workingMaximum(request, maximum)) return false;
    }
    return recoveryBytes(request) <= maximum;
}

KisGpuLayerDocument::Recovery KisGpuLayerDocument::recovery() const
{
    if (m_restoring) return m_restoring->request;
    Recovery result;
    result.cursor = m_cursor;
    result.nextId = m_nextId;
    result.history = m_history;
    if (m_active) {
        const auto &record = m_history[m_cursor][indexOf(m_active->id)];
        auto data = *m_session->recovery();
        const auto before = rasterVersion(m_active->target == Target::Pixels ? record->pixels : record->mask);
        const bool completed = !(m_session->head() == before)
            && (m_active->target == Target::Pixels || m_session->head().tileCount() || record->mask);
        if (completed) { data.working.reset(); data.commitRequested = false; }
        if (completed || data.working) result.working = RecoveryWorking{
            record, m_active->target, std::move(data), completed};
    }
    return result;
}

KisGpuLayerDocument::Editing KisGpuLayerDocument::currentEditing() const
{
    return m_active ? Editing{Result::Accepted, m_session, m_active->token} : Editing{};
}

KisGpuTileStore::Error KisGpuLayerDocument::recoveryError() const
{
    if (!m_restoring) return KisGpuTileStore::Error::None;
    return !m_store->deviceAvailable() ? KisGpuTileStore::Error::DeviceLost : m_restoring->error;
}

void KisGpuLayerDocument::pollRecovery()
{
    using Session = KisGpuEditSession;
    auto &restore = *m_restoring;
    if (restore.failed) return;
    const auto fail = [&](KisGpuTileStore::Error error = KisGpuTileStore::Error::DeviceLost) {
        restore.failed = true;
        restore.error = error;
        restore.session.reset();
        restore.rasters.clear();
    };
    if (!m_store->deviceAvailable()) { fail(); return; }
    const auto replay = [&](const Session::Recovery &data) {
        try {
            return std::shared_ptr<Session>(new Session(*m_store, 0, data, m_maximumRecoveryBytes),
                                           [store = m_store](Session *value) { delete value; });
        } catch (const std::invalid_argument &) {
            if (!m_store->deviceAvailable()) { fail(); return std::shared_ptr<Session>{}; }
            throw;
        }
    };
    if (restore.index < restore.records.size()) {
        const auto &source = restore.records[restore.index];
        const auto &raster = restore.maskPhase ? source->mask : source->pixels;
        if (!restore.rasters.count(raster.get())) {
            if (!restore.session) restore.session = replay(*raster);
            if (!restore.session) return;
            restore.session->poll();
            if (restore.session->state() == Session::State::Failed) { fail(restore.session->lastGpuError()); return; }
            if (restore.session->state() != Session::State::Idle) return;
            restore.rasters[raster.get()] = restore.session->head();
            restore.session.reset();
        }
        if (!restore.maskPhase && source->mask) {
            restore.maskPhase = true;
            return;
        }
        restore.maskPhase = false;
        ++restore.index;
        return;
    }
    if (restore.request.working) {
        const auto &work = *restore.request.working;
        if (!restore.session) {
            const auto &base = *work.base;
            const auto version = work.target == Target::Pixels ? restore.rasters.at(base.pixels.get())
                : (base.mask ? restore.rasters.at(base.mask.get()) : m_store->emptyVersion());
            try {
                restore.session = std::shared_ptr<Session>(work.completed
                    ? new Session(*m_store, 0, work.data, *workingMaximum(restore.request, m_maximumRecoveryBytes))
                    : new Session(*m_store, version, work.data, *workingMaximum(restore.request, m_maximumRecoveryBytes)),
                    [store = m_store](Session *value) { delete value; });
            } catch (const std::invalid_argument &) {
                if (!m_store->deviceAvailable()) { fail(); return; }
                throw;
            }
        }
        restore.session->poll();
        const auto state = restore.session->state();
        if (state == Session::State::Failed) { fail(restore.session->lastGpuError()); return; }
        if (state == Session::State::Restoring) return;
        if (work.completed) {
            const auto token = restore.session->begin();
            if (!token || restore.session->commit(token) != Session::Result::Accepted) {
                fail();
                return;
            }
        }
        m_session = restore.session;
        m_active = Active{work.base->id, work.target, m_session->currentToken()};
    }
    if (restore.initial) {
        m_history = restore.request.history;
        m_rasterVersions = std::move(restore.rasters);
        m_nextId = restore.request.nextId;
    } else {
        m_rasterVersions.merge(restore.rasters);
    }
    m_cursor = restore.request.cursor;
    m_restoring.reset();
}
